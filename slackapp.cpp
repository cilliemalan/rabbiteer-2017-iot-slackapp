#include "pch.h"
#include "slackapp.h"
#include "utils.h"

using namespace utility; // Common utilities like string conversions
using namespace web; // Common features like URIs.
using namespace web::http; // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

//web::http::client::http_client::~http_client() noexcept {}

pplx::task<bool> slack_app::test_access_token(const string_t& access_token)
{
    printf("authing\n");
    auto path = U("auth.test?token=") + access_token;

    return _client.request(methods::POST, path)
                  .then([=](http_response response)
                  {
                      return response.extract_json();
                  })
                  .then([=](json::value json)
                  {
                      return json.as_object()
                                 .at(U("ok"))
                                 .as_bool();
                  });
}

pplx::task<utility::string_t> slack_app::get_userid(const string_t& access_token)
{
    auto path = U("auth.test?token=") + access_token;

    return _client
        .request(methods::POST, path)
        .then([=](http_response response)
        {
            return response.extract_json();
        })
        .then([=](json::value json)
        {
            return json.as_object()
                       .at(U("user_id"))
                       .as_string();
        });
}

pplx::task<web::uri> slack_app::get_ws_url()
{
    auto path = U("rtm.connect?token=") + _bot_access_token;

    return _client
        .request(methods::POST, path)
        .then([=](http_response response)
        {
            return response.extract_json();
        })
        .then([=](json::value json)
        {
            auto obj = json.as_object();
            auto isok = obj.at(U("ok")).as_bool();

            if (isok)
            {
                auto url = obj.at(U("url")).as_string();
                return web::uri{url};
            }
            else
            {
                throw std::runtime_error(N(obj.at(U("error")).as_string()));
            }
        });
}

pplx::task<bool> slack_app::verify_access_tokens()
{
    printf("verifying\n");
    std::array<pplx::task<bool>, 2> tasks
    {
        test_access_token(_access_token),
        test_access_token(_bot_access_token)
    };

    pplx::task<bool> combined = pplx::
        when_all(std::begin(tasks), std::end(tasks))
        .then([](std::vector<bool> results)
        {
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

                printf("%s\n", messages.c_str());
                return false;
            }
            else
            {
                return true;
            }
        });

    return combined;
}

pplx::task<void> slack_app::listen()
{
    pplx::task_completion_event<void> tce;
    pplx::task<void> completed(tce);

    return async_do_while([=]()
    {
        return _rtm_client->receive()
            .then([=](web::websockets::client::websocket_incoming_message in_msg)
            {
                return in_msg.extract_string()
                             .then([=](std::string msg_str)
                             {
                                 string_t msg_str_t = W(msg_str);
                                 json::value json_val = json::value::parse(msg_str_t);
                                 handle_message(json_val);
                             });
            }).then([=](pplx::task<void> end_task)
            {
                try
                {
                    end_task.get();
                    return true;
                }
                catch (std::exception& e)
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

    if (vtype.is_string())
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
            if (text.is_string() && !(stext = N(text.as_string())).empty())
            {
                auto channel = message[U("channel")].as_string();
                auto from = message[U("user")].as_string();

                printf("got message: %s from %s\n", stext.c_str(), N(from).c_str());

                if (from != _bot_userid)
                {
                    auto emojis = get_emojis_in_message(stext);

                    sequential_transform(emojis.begin(), emojis.end(),
                                         [=](std::string p) -> pplx::task<std::pair<std::string, std::string>>
                                     {
                                         return get_emoji_url(p)
                                             .then([=](std::string url)
                                             {
                                                 return std::make_pair(p, url);
                                             });
                                     })
                        .then([=](std::vector<std::pair<std::string, std::string>> urls)
                        {
                            std::string replymsg;

                            if (urls.size())
                            {
                                replymsg = "you specified these emojis: \n";
                                for (auto& pair : urls)
                                {
                                    std::string url;
                                    if (pair.second.empty())
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
    else if (vreply.is_number())
    {
    }


    return pplx::task_from_result();
}

pplx::task<void> slack_app::send_message(const utility::string_t& text, const utility::string_t& channel)
{
    auto obj = json::value::object();
    obj[U("id")] = json::value::number(++_messageid);
    obj[U("type")] = json::value::string(U("message"));
    obj[U("channel")] = json::value::string(channel);
    obj[U("text")] = json::value::string(text);

    websockets::client::websocket_outgoing_message message;
    message.set_utf8_message(N(obj.serialize()));

    return _rtm_client->send(message);
}

static std::regex rx_emoji(":([-a-zA-Z0-9_+]+):");

std::vector<std::string> slack_app::get_emojis_in_message(const std::string& s) const
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

    return _client
        .request(methods::POST, path)
        .then([=](http_response response)
        {
            return response.extract_json();
        }).then([=](json::value json)
        {
            std::map<std::string, std::string> obj_emoji;
            auto emojis = json[U("emoji")].as_object();

            for (auto i = emojis.begin(); i != emojis.end(); ++i)
            {
                auto kvp = *i;
                auto key = N(kvp.first);
                auto val = N(kvp.second.as_string());

                obj_emoji[key] = val;
            }

            return obj_emoji;
        });
}

pplx::task<bool> slack_app::process_loop()
{
    pplx::task<void> at_root = pplx::task_from_result();

    if (_rapid_fail > 2)
    {
        _access_ok = false;
        _rtm_url = U("");
        at_root = task_delay(std::chrono::seconds(3));
    }

    return at_root.then([=]
    {
        return pplx::task<bool>
        {
            _access_ok
                ? pplx::task_from_result(true)
                : verify_access_tokens().then([=](bool verified)
                {
                    _access_ok = verified;
                    printf("verified: %s\n", verified ? "true" : "false");
                    return verified;
                })
        };
    }).then([=](bool verified)
    {
        if (!verified)
        {
            return pplx::task_from_result(false);
        }
        else
        {
            // get bot user id
            pplx::task<void> at_botuserid
            {
                _bot_userid.empty()
                    ? get_userid(_bot_access_token)
                    .then([=](string_t userid)
                    {
                        printf("my user id: %s\n", N(userid).c_str());
                        _bot_userid = userid;
                    })
                    : pplx::task_from_result()
            };

            // get url
            pplx::task<void> at_url
            {
                _rtm_url.is_empty()
                    ? get_ws_url().then([=](web::uri wsurl)
                    {
                        printf("got ws url\n");
                        _rtm_url = wsurl;
                    })
                    : pplx::task_from_result()
            };

            // get emojis
            pplx::task<void> at_emojis
            {
                _emojis.empty()
                    ? get_custom_emojis().then([=](std::map<std::string, std::string> emojis)
                    {
                        for (auto& i : emojis)
                        {
                            _emojis[i.first] = i.second;
                        }

                        printf("got custom emojis\n");
                    })
                    : pplx::task_from_result()
            };

            // wait while they complete
            std::vector<pplx::task<void>> tasks{at_botuserid, at_url, at_emojis};
            return pplx::when_all(tasks.begin(), tasks.end()).then([=]
            {
                // then connect

                reset_rtm_client();
                return _rtm_client->connect(_rtm_url).then([this]()
                {
                    printf("connected.\n");
                    _rapid_fail = 0;

                    return listen().then([=] () {
                        // will only get here if listening stops without error for some reason
                        return true; 
                    });
                });
            });
        }
    }).then([this](pplx::task<bool> listen_task)
    {
        try
        {
            // if we return true here listening has stopped but without error someohow.
            return listen_task.get();
        }
        catch (const std::exception& ex)
        {
            printf("listening stopped with error: %s\n", ex.what());
        }
        catch (...)
        {
            printf("listening stopped with unknown error\n");
        }

        ++_rapid_fail;
        return true;
    });
}


pplx::task<const void*> slack_app::get_emoji_data(const std::string& emoji)
{
    return pplx::task_from_result<const void*>(nullptr);
}

pplx::task<std::string> slack_app::get_emoji_url(const std::string& emoji)
{
    //try cached emoji
    auto found = _emojis.find(emoji);
    if (found != _emojis.end())
    {
        return pplx::task_from_result(found->second);
    }
    else
    {
        //get emojis again
        return get_custom_emojis()
            .then([this, emoji](std::map<std::string, std::string> emojis)
            {
                // cache new custom emojis
                for (auto& i : emojis)
                {
                    _emojis[i.first] = i.second;
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

    auto loop = std::bind(&slack_app::process_loop, this);

    auto task = async_do_while(loop);

    task.wait();
}
