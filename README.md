# Rabbiteer 2017 IOT Slack app

This is a small application that integrates with Slack and marshalls messages between
Slack and devices.

# Dependencies
You will need the [REST SDK](https://github.com/Microsoft/cpprestsdk/wiki) from microsoft,
giflib, and libpng. Install like:
```
$ sudo apt install libcpprest-dev libgif-dev libpng16-dev
```
Or on Windows use [vcpkg](https://github.com/Microsoft/vcpkg):
```
> vcpkg install cpprestsdk:x64-windows libpng:x64-windows giflib:x64-windows
```

# Building
The usual:
```
$ make
```
or on windows just open the solution in VS.

# Running
It expects `SLACK_ACCESS_TOKEN` and `SLACK_BOT_ACCESS_TOKEN` to be set as environment
variables. Obtain these by [creating a slack app](https://api.slack.com/apps?new_app=1)
and getting the tokens on the OAuth page:

![oauth page](https://i.imgur.com/r45xqrj.png)

Note: you will need to add scopes: `bot`, `emoji:read`, `chat:write:bot`, `channels:read`, `channels:history` (disclaimer: I'm not sure that you need all of them but these are the ones enabled for me).

