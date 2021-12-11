OUTPUT=twitch-bot

all: client

commands: force
	$(MAKE) -C commands

.o: socket.c irc.c client.c dbus.c
	gcc `pkg-config --cflags dbus-1` -c socket.c irc.c client.c debug.c dbus.c

client: commands .o
	gcc -o $(OUTPUT) `pkg-config --libs dbus-1` *.o commands/*.o

clean:
	rm -f *.o **/*.o
	rm -f twitch-bot

force:
