# Basic Twitch IRC bot

Baseline Twitch IRC chat bot in C.

## Usage
```
./twitch-bot my_user_name "oauth:my_oauth_token" channel_name
```

## What it does

Connects to Twitch IRC, authenticates with given user data, and joins specified channel.

If everything is ok,
- it will start printing user messages to the STDOUT with the following format:
  `tags<TAB>user<TAB>command<TAB>message\n`
- Command part is optional.
- it also creats a FIFO at /tmp/irc-client that you can use to send raw IRC commands to the channel.

## TODO

Custom commands handling

## License

[WTFPL](http://www.wtfpl.net/)

## Acknowledgements

Inspired by [irc.c client](https://c9x.me/irc/).
