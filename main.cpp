
#include <cstdio>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

web::http::client::http_client::~http_client() { }

int main()
{
    // Create http_client to send the request.
    http_client client(U("https://www.bing.com/"));

    // Build request URI and start the request.
    uri_builder builder(U("/search"));
    builder.append_query(U("q"), U("cpprestsdk github"));
    auto requestTask = client.request(methods::GET, builder.to_string())
                    .then([=](http_response response) {
                        printf("Received response status code:%u\n", response.status_code());
                    });

    try
    {
        requestTask.wait();
    }
    catch (const std::exception &e)
    {
        printf("Error exception:%s\n", e.what());
    }

    return 0;
}
