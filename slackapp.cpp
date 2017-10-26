
#include "pch.h"
#include "slackapp.h"
#include "utils.h"

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

//web::http::client::http_client::~http_client() noexcept {}

pplx::task<bool> slack_app::test_access_token(const string_t &access_token)
{
    printf("authing\n");
    auto path = U("auth.test?token=") + access_token;

    return _client.request(methods::POST, path)
        .then([=](http_response response) {
        return response.extract_json();
    })
        .then([=](json::value json) {
        return json.as_object()
            .at(U("ok"))
            .as_bool();
    });
}

pplx::task<utility::string_t> slack_app::get_userid(const string_t &access_token)
{
    auto path = U("auth.test?token=") + access_token;

    return _client.request(methods::POST, path)
        .then([=](http_response response) {
        return response.extract_json();
    })
        .then([=](json::value json) {
        return json.as_object()
            .at(U("user_id"))
            .as_string();
    });
}

pplx::task<web::uri> slack_app::get_ws_url()
{
    auto path = U("rtm.connect?token=") + _bot_access_token;

    return _client.request(methods::POST, path)
        .then([=](http_response response) {
        return response.extract_json();
    })
        .then([=](json::value json) {
        auto obj = json.as_object();
        auto isok = obj.at(U("ok")).as_bool();

        if (isok)
        {
            auto url = obj.at(U("url")).as_string();
            return web::uri{ url };
        }
        else
        {
            throw std::runtime_error(N(obj.at(U("error")).as_string()));
        }
    });
}

pplx::task<void> slack_app::verify_access_tokens()
{
    printf("verifying\n");
    std::array<pplx::task<bool>, 2> tasks
    {
        test_access_token(_access_token),
        test_access_token(_bot_access_token)
    };

    pplx::task<void> combined = pplx::when_all(std::begin(tasks), std::end(tasks))
        .then([](std::vector<bool> results) {
        printf("checking\n");
        auto at_ok = results[0];
        auto bat_ok = results[1];
        if (!at_ok || !bat_ok)
        {
            std::string messages("Could not authenticate");

            if (!at_ok)
            {
                messages += "\n  Problem with access token";
            }
            if (!bat_ok)
            {
                messages += "\n  Problem with bot access token";
            }

            throw std::runtime_error(messages);
        }
    });

    return combined;
}

pplx::task<void> slack_app::listen()
{
    pplx::task_completion_event<void> tce;
    pplx::task<void> completed(tce);

    return async_do_while([=]() {
        return _rtm_client.receive().then([=](web::websockets::client::websocket_incoming_message in_msg) {

            return in_msg.extract_string().then([=](std::string msg_str) {

                string_t msg_str_t = W(msg_str);
                json::value json_val = json::value::parse(msg_str_t);
                handle_message(json_val);
            });

        }).then([=](pplx::task<void> end_task) {
            try
            {
                end_task.get();
                return true;
            }
            catch (std::exception &e)
            {
                printf("ws error: %s\n", e.what());
            }
            catch (...)
            {
                printf("unkown error\n");
            }

            // We are here means we encountered some exception.
            // Return false to break the asynchronous loop.

            return false;
        });
    });
}

pplx::task<void> slack_app::handle_message(web::json::value message)
{
    printf("message:\n%s\n", N(message.serialize()).c_str());
    auto vtype = message[U("type")];
    auto vreply = message[U("reply_to")];

    if(vtype.is_string())
    {
        auto type = message[U("type")].as_string();

        if (type == U("hello"))
        {
            printf("received hello message\n");
        }
        else if (type == U("reconnect_url"))
        {
            auto new_url = message[U("url")].as_string();
            _rtm_url = new_url;

            printf("received new connect URL\n");
        }
        else if (type == U("message"))
        {
            auto text = message[U("text")];
            std::string stext;
            if(text.is_string() && !(stext = N(text.as_string())).empty())
            {
                auto channel = message[U("channel")].as_string();
                auto from = message[U("user")].as_string();

                printf("got message: %s from %s\n", stext.c_str(), N(from).c_str());

                if(from != _bot_userid)
                {
                    auto emojis = get_emojis_in_message(stext);

                    sequential_transform(emojis.begin(), emojis.end(),
                        [=] (std::string p) -> pplx::task<std::pair<std::string, std::string>>
                    {
                        return get_emoji_url(p).then([=](std::string url)
                        {
                            return std::make_pair(p, url);
                        });
                    })
                    .then([=] (std::vector<std::pair<std::string, std::string>> urls) {
                        std::string replymsg;

                        if (urls.size())
                        {
                            replymsg = "you specified these emojis: \n";
                            for (auto&pair : urls)
                            {
                                std::string url;
                                if(pair.second.empty())
                                {
                                    url = ":question:";
                                }
                                else
                                {
                                    url = pair.second;
                                }

                                replymsg += "  " + pair.first + " -> " + url + "\n";
                            }
                        }
                        else
                        {
                            replymsg = "you did not specify any emojis";
                        }

                        return send_message(W(replymsg), channel);
                    });
                }
            }
        }
        else if (type == U("presence_change") || type == U("desktop_notification"))
        {

        }
        else
        {

        }
    }
    else if(vreply.is_number())
    {
        
    }
  

    return pplx::task_from_result();
}

pplx::task<void> slack_app::send_message(const utility::string_t &text, const utility::string_t &channel)
{
    auto obj = json::value::object();
    obj[U("id")] = json::value::number(++_messageid);
    obj[U("type")] = json::value::string(U("message"));
    obj[U("channel")] = json::value::string(channel);
    obj[U("text")] = json::value::string(text);

    websockets::client::websocket_outgoing_message message;
    message.set_utf8_message(N(obj.serialize()));

    return _rtm_client.send(message);
}

static std::regex rx_emoji(":([-a-zA-Z0-9_+]+):");
std::vector<std::string> slack_app::get_emojis_in_message(const std::string &s) const
{
    std::vector<std::string> result;
    auto emojis_begin = std::sregex_iterator(s.begin(), s.end(), rx_emoji);
    auto emojis_end = std::sregex_iterator();

    for (std::sregex_iterator i = emojis_begin; i != emojis_end; ++i)
    {
        std::smatch match = *i;
        std::string match_str = match.str(1);
        result.push_back(match_str);
    }

    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());

    return result;
}

pplx::task<std::map<std::string, std::string>> slack_app::get_custom_emojis()
{
    auto path = U("emoji.list?token=") + _access_token;

    return _client.request(methods::POST, path)
        .then([=](http_response response) {
        return response.extract_json();
    }).then([=](json::value json) {
        std::map<std::string, std::string> obj_emoji;
        auto emojis = json[U("emoji")].as_object();

        for(auto i = emojis.begin(); i != emojis.end(); ++i)
        {
            auto kvp = *i;
            auto key = N(kvp.first);
            auto val = N(kvp.second.as_string());

            obj_emoji[key] = val;
        }

        return obj_emoji;
    });
}

pplx::task<void> slack_app::process_loop()
{
    pplx::task<void> at_verified
    {
        _access_ok ? pplx::task_from_result() : verify_access_tokens()
    };

    // the task will resolve on disconnect
    return at_verified.then([=] {
        if(_bot_userid.empty())
        {
            return get_userid(_bot_access_token).then([=](string_t userid) {
                printf("my user id: %s\n", N(userid).c_str());
                _bot_userid = userid;
            });
        }
        else
        {
            return pplx::task_from_result();
        }
    }).then([=] {
        _access_ok = true;
        printf("verified\n");


        //get url
        if(_rtm_url.is_empty())
        {
            return get_ws_url().then([=](web::uri wsurl) {
                printf("got ws url\n");
                _rtm_url = wsurl;
            });
        }
        else
        {
            return pplx::task_from_result();
        }

    }).then([=] {

        //get emojis
        return get_custom_emojis().then([=](std::map<std::string, std::string> emojis) {
            for(auto &i : emojis)
            {
                _emojis.insert_or_assign(i.first, i.second);
            }
        });
    }).then([=] {

        //connect
        return _rtm_client.connect(_rtm_url);
    }).then([this](pplx::task<void> task) {

        try
        {
            task.get();
            printf("connected.\n");
        }
        catch (...)
        {
            printf("connection error.\n");

            // force re-getting url
            _rtm_url = U("");
            _access_ok = false;

            return pplx::task_from_result();
        }

        return listen();
    }).then([] {
        printf("listen over\n");
    });
}


pplx::task<const void*> slack_app::get_emoji_data(const std::string &emoji)
{
    return pplx::task_from_result<const void*>(nullptr);
}

pplx::task<std::string> slack_app::get_emoji_url(const std::string &emoji)
{
    //try cached emoji
    auto found = _emojis.find(emoji);
    if(found != _emojis.end())
    {
        return pplx::task_from_result(found->second);
    }
    else
    {
        //get emojis again
        return get_custom_emojis().then([this,emoji](std::map<std::string, std::string> emojis) {

            // cache new custom emojis
            for (auto &i : emojis)
            {
                _emojis.insert_or_assign(i.first, i.second);
            }

            // find from custom emojis
            auto found2 = _emojis.find(emoji);
            if (found2 != _emojis.end())
            {
                return found2->second;
            }
            else
            {
                return std::string();
            }
        });
    }
}

void slack_app::run()
{
    printf("enteing\n");

    auto task = async_do_while([=] {
        return process_loop().then([=] {
            return true;
        });
    });

    task.wait();

}