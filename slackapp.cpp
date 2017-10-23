
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
            //throw std::runtime_error("TODO");
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

                printf("message: %s\n", msg_str.c_str());
                // string_t msg_str_t = to_string_t(msg_str);
                // json::value json_val = json::value::parse(msg_str_t);
                // ServerClientPacket packet(json_val);
                // receive_message(packet);

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

pplx::task<void> slack_app::process_loop()
{
    pplx::task<void> at_verified
    {
        _access_ok ? pplx::task_from_result() : verify_access_tokens()
    };

    // the task will resolve on disconnect
    return at_verified.then([this] {
        _access_ok = true;
        printf("verified\n");

        pplx::task<web::uri> uri_task
        {
            _rtm_url.is_empty() ? get_ws_url() : pplx::task_from_result(_rtm_url)
        };

        return uri_task;
    }).then([this](web::uri wsurl) {

        printf("got ws url\n");
        _rtm_url = wsurl;

        return _rtm_client.connect(_rtm_url);
    }).then([this](pplx::task<void> task) {

        try
        {
            task.get();
            printf("connected.\n");

            _rtm_url = U("");
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