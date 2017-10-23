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
        _bot_access_token(bot_access_token)
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

    pplx::task<bool> test_access_token(const utility::string_t &token);
    pplx::task<void> verify_access_tokens();
    pplx::task<void> process_loop();
    pplx::task<void> run_internal();
    pplx::task<web::uri> get_ws_url();
    pplx::task<void> listen();

    inline static web::websockets::client::websocket_client_config get_rtm_client_config(utility::string_t access_token)
    {
        printf("configuring\n");
        web::websockets::client::websocket_client_config wsconfig;
        wsconfig.headers().add(U("Authorization"), U("Bearer ") + access_token);
        return wsconfig;
    }
};