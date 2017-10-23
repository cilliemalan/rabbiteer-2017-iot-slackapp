#pragma once

#include "pch.h"

class slack_app
{
public:
    slack_app(const utility::string_t access_token,
        const utility::string_t bot_access_token,
        const web::uri api_url = U("https://slack.com/api")) :
        _client(api_url),
        _rtm_client(get_rtm_client_config(bot_access_token)),
        _access_token(access_token),
        _bot_access_token(bot_access_token),
        _access_ok(false)
    {
        printf("constructing\n");
        if(access_token.empty()) throw std::runtime_error("access_token cannot be empty");
        if(bot_access_token.empty()) throw std::runtime_error("bot_access_token cannot be empty");
    }

    void run();

private:
    web::http::client::http_client _client;
    web::websockets::client::websocket_client _rtm_client;
    const utility::string_t _access_token;
    const utility::string_t _bot_access_token;
    web::uri _rtm_url;
    bool _access_ok;
    std::atomic<int> _messageid;
    utility::string_t _bot_userid;
    std::map<std::string, std::string> _emojis;

    pplx::task<bool> test_access_token(const utility::string_t &token);
    pplx::task<utility::string_t> get_userid(const utility::string_t &token);
    pplx::task<void> verify_access_tokens();
    pplx::task<void> process_loop();
    pplx::task<web::uri> get_ws_url();
    pplx::task<void> listen();
    pplx::task<std::map<std::string, std::string>> get_emojis();

    pplx::task<void> handle_message(web::json::value message);

    pplx::task<void> send_message(const utility::string_t &mesasge, const utility::string_t &channel);

    std::vector<std::string> get_emojis_in_message(const std::string &s) const;

    inline static web::websockets::client::websocket_client_config get_rtm_client_config(utility::string_t access_token)
    {
        printf("configuring\n");
        web::websockets::client::websocket_client_config wsconfig;
        wsconfig.headers().add(U("Authorization"), U("Bearer ") + access_token);
        return wsconfig;
    }
};