# Twitch IRC chat relay

Barebone Twitch IRC chat relay/bot in C.

There are a lot of these out there. This one is not better, and probably worse in most
aspects. But it's mine. :)

## Usage
```
./twitch-bot my_user_name "oauth:my_oauth_token" channel_name [-f|-s|-d]
```

## What it can do

Connects to Twitch IRC, authenticates with given user data, and joins specified channel.

If `-f` argument was provided, two FIFO pipes will be created at `/tmp/twitch-bot-in|out`.
The `-in` one is observed to receive commands and send them to the channel. The
`-out` one outputs whatever messages it receives from other users in the
channel.

If `-d` was provided, the client will try to connect to the DBus and send incoming IRC
messages as DBus signals. You can tweak the interface/type strings in `client.c`.

Otherwise the client will listen to `stdin` and output to `stdout`.

## Output

Output is in JSON format. The client filters out PING messages and sends everything else
to the selected output channel. Here's an example of a message:

```json
{"tags":"@badge-info=;badges=broadcaster/1;client-nonce=32bc0ef43873d1203001e148ec84ed03;color=#8A2BE2;display-name=cog1to;emotes=
;flags=;id=0ddb158f-44b1-4e9b-89a7-9b4d660fd9bb;mod=0;room-id=32319568;subscriber=0;tmi-sent-ts=1633040608904;turbo=0;user-id=3231
9568;user-type=","sender":":cog1to!cog1to@cog1to.tmi.twitch.tv","command":"PRIVMSG","message":"test"}
```

## Input

Input is just any string ending with a newline symbol. It is taken "as is" and is sent to the server without modifications.
For example, to send a message to a chat just type in `PRIVMSG #channel_name :hello!<newline>`.

## Commands

*Note:* this commands framework is just an example of adding custom logic to the client.
If you need an actual chatbot, I suggest writing it as a separate program that just
interacts with this program via std channels or pipes.

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

## TODO

- More friendly input command parsing. Typign out `PRIVMSG #mychannel :mymessage` is
annoying.
- Add Dbus interface along with pipes and standard in/out.
- Investigate and stabilize connection action sequence. Sometimes the connection hangs 
waiting for server's response.

## License

[GNU LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html).

## Acknowledgements

Inspired by [irc.c client](https://c9x.me/irc/).
