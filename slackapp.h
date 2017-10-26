#pragma once

#include "pch.h"
#include "emoji.h"

class slack_app
{
public:
    slack_app(const utility::string_t access_token,
        const utility::string_t bot_access_token,
        const web::uri api_url = U("https://slack.com/api")) :
        _client(api_url),
        _access_token(access_token),
        _bot_access_token(bot_access_token),
        _emojis(get_all_builtin_emoji())
    {
        printf("constructing\n");

        if(access_token.empty()) throw std::runtime_error("access_token cannot be empty");
        if(bot_access_token.empty()) throw std::runtime_error("bot_access_token cannot be empty");
    }

    void run();

private:
    web::http::client::http_client _client;
    std::unique_ptr<web::websockets::client::websocket_client> _rtm_client;
    const utility::string_t _access_token;
    const utility::string_t _bot_access_token;
    web::uri _rtm_url;
    bool _access_ok = false;
    std::atomic<int> _messageid;
    utility::string_t _bot_userid;
    std::map<std::string, std::string> _emojis;
    int _rapid_fail = 0;

    pplx::task<bool> test_access_token(const utility::string_t &token);
    pplx::task<utility::string_t> get_userid(const utility::string_t &token);
    pplx::task<bool> verify_access_tokens();
    pplx::task<bool> process_loop();
    pplx::task<web::uri> get_ws_url();
    pplx::task<void> listen();

    pplx::task<void> handle_message(web::json::value message);
    pplx::task<void> send_message(const utility::string_t &mesasge, const utility::string_t &channel);

    pplx::task<std::map<std::string, std::string>> get_custom_emojis();
    std::vector<std::string> get_emojis_in_message(const std::string &s) const;
    pplx::task<const void*> get_emoji_data(const std::string &emoji);
    pplx::task<std::string> get_emoji_url(const std::string &emoji);

    void reset_rtm_client()
    {
        // fire and forget
        if (_rtm_client)
        {
            try { _rtm_client->close(); }
            catch (...) {}
        }

        printf("configuring rtm client\n");
        web::websockets::client::websocket_client_config wsconfig;
        wsconfig.headers().add(U("Authorization"), U("Bearer ") + _bot_access_token);

        _rtm_client.reset(new web::websockets::client::websocket_client(wsconfig));
    }
};
