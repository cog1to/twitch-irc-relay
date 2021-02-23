# Basic Twitch IRC bot

Baseline Twitch IRC chat bot in C.

## Usage
```
./twitch-bot my_user_name "oauth:my_oauth_token" channel_name
```

## What it does

Connects to Twitch IRC, authenticates with given user data, and joins specified channel.

If everything is ok, two FIFO pipes will be created at `/tmp/twitch-bot-in|out`. 
The `-in` one is observed to receive commands and send them to the channel. The 
`-out` one outputs whatever messages it receives from other users in the 
channel.

## Commands 

The program supports custom automated commands. See `commands` directory for basic 
directions. In short, adding a new command should include:

1. Creating a new `.c` file with two methods:

```
int xxx_match(irc_message_t *message);
void xxx_handle(irc_t *irc, irc_message_t *message)
```

The `_match` method determines if the command should be triggered by incoming channel 
message.

The `_handle` method should contain custom logic for the specific command 
you're implementing, like sending back another message.

2. Registering the command in the client.

See top of the `client.c` for an example. For convenience there are a couple of macros
you can use:

- `COMMAND(xxx)` does exporting command's match and handle functions.
- `REGISTER(xxx)` adds command's functions to a list of commands to check agains
when receiving a new channel message.

*Note:* this commands framework is just an example of adding custom logic to the client.
If you need an actual chatbot, I suggest writing it as a separate program that just 
interacts with in and out pipes.

## License

[WTFPL](http://www.wtfpl.net/)

## Acknowledgements

Inspired by [irc.c client](https://c9x.me/irc/).
