
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

web::http::client::http_client::~http_client() {}

std::string api_url = "https://slack.com/api";

pplx::task<bool> test_access(http_client client)
{
    return client.request(
                     methods::POST,
                     "auth.test",
                     "",
                     "application/x-www-form-urlencoded; charset=utf-8")
        .then([=](http_response response) {
            return response.extract_json();
        })
        .then([=](json::value json) {
            return json.as_object().at("ok").as_bool();
        });
}

int main()
{
    const char *access_token = std::getenv("SLACK_ACCESS_TOKEN");
    if (!access_token)
    {
        fprintf(stderr, "Could not get access token. SLACK_ACCESS_TOKEN not set\n");
        return -1;
    }

    try
    {
        // Create http_client to send the request.
        http_client client(api_url);

        bool result = test_access(client).wait();
        printf("%s\n", result ? "access okay" : "access not okay");
    }
    catch (const std::exception &e)
    {
        printf("Error exception:%s\n", e.what());
    }
    
    return 0;
}
