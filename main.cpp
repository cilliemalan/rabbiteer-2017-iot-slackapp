
#include <cstdio>
#include <cstdlib>

#include <string>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

web::http::client::http_client::~http_client() noexcept {}

std::string api_url = "https://slack.com/api";

pplx::task<bool> test_access_token(http_client client, const std::string &access_token)
{
    http_request message(methods::POST);
    message.set_request_uri("auth.test");
    message.set_body("", "application/x-www-form-urlencoded");
    message.headers().add("Authorization", "Bearer " + access_token);

    return client.request(message)
        .then([=](http_response response) {
            return response.extract_json();
        })
        .then([=](json::value json) {
            return json.as_object()
                .at("ok")
                .as_bool();
        });
}

void verify_access_tokens(http_client client, const std::string &access_token, const std::string &bot_access_token)
{
    if (access_token.empty())
    {
        throw std::runtime_error("Could not get access token. SLACK_BOT_ACCESS_TOKEN not set\n");
    }

    if (access_token.empty())
    {
        throw std::runtime_error("Could not get access token. SLACK_ACCESS_TOKEN not set\n");
    }

    bool token_result = test_access_token(client, access_token).get();
    if (!token_result)
    {
        throw std::runtime_error("could not authenticate. Please check access token");
    }

    bool bot_token_result = test_access_token(client, bot_access_token).get();
    if (!bot_token_result)
    {
        throw std::runtime_error("could not authenticate bot. Please check bot access token");
    }
}

int main()
{
    try
    {
        http_client client(api_url);

        const std::string access_token(std::getenv("SLACK_ACCESS_TOKEN"));
        const std::string bot_access_token(std::getenv("SLACK_BOT_ACCESS_TOKEN"));

        verify_access_tokens(client, access_token, bot_access_token);
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "Error:%s\n", e.what());
    }

    return 0;
}
