
#include "pch.h"
#include "slackapp.h"
#include "utils.h"



int main()
{
    try
    {
		auto e_access = std::getenv("SLACK_ACCESS_TOKEN");
		auto e_bot = std::getenv("SLACK_BOT_ACCESS_TOKEN");

		if (!e_access) throw std::runtime_error("SLACK_ACCESS_TOKEN environment variable must be set.");
		if (!e_bot) throw std::runtime_error("SLACK_BOT_ACCESS_TOKEN environment variable must be set.");

        const std::string access_token(e_access);
        const std::string bot_access_token(e_bot);

        slack_app app(W(access_token), W(bot_access_token));

        app.run();
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "Error:%s\n", e.what());
    }

    return 0;
}
