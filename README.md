# Rabbiteer 2017 IOT Slack app

This is a small application that integrates with Slack and marshalls messages between
Slack and devices.

# Dependencies
You will need the [REST SDK](https://github.com/Microsoft/cpprestsdk/wiki) from microsoft.
Install like:
```
$ sudo apt install libcpprest-dev
```
on Linux, or on Windows use vcpkg:
```
> vcpkg install cpprestsdk:x64-windows
```

# Building
The usual:
```
$ make
```
or on windows just open the solution in VS.

# Running
It expects `SLACK_ACCESS_TOKEN` and `SLACK_BOT_ACCESS_TOKEN` to be set as environment
variables. Obtain these by [creating a slack app](https://api.slack.com/apps?new_app=1).

