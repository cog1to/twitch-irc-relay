OUTPUT=twitch-bot

all: client

commands: FORCE
	$(MAKE) -C commands

.o: socket.c irc.c client.c
	gcc -c socket.c irc.c client.c

client: commands .o
	gcc -o $(OUTPUT) *.o commands/*.o

clean:
	rm -f **/*.o
	rm -f twitch-bot

FORCE:
