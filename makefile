all: client

client: socket.c irc.c client.c
	gcc -o client socket.c irc.c client.c

clean:
	rm client

