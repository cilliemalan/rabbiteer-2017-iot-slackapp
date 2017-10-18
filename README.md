# Rabbiteer 2017 IOT Slack app

This is a small application that integrates with Slack and marshalls messages between
Slack and devices.

# Dependencies
Only Linux is supported (also [WSL](https://msdn.microsoft.com/en-us/commandline/wsl/about)).
You will need [cpprestsdk](https://github.com/Microsoft/cpprestsdk/wiki).
Install like this: `sudo apt install libcpprest-dev`

# Building
The usual:
```
$ make
```

## Trouble
If you get `make: root-config: Command not found`, install libroot like this:
`sudo apt install libroot-core-dev`

