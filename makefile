OUTPUT=twitch-bot

all: client

client: socket.c irc.c client.c
	gcc -o $(OUTPUT) socket.c irc.c client.c

clean:
	rm -f twitch-bot

