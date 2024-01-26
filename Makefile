OUTPUT = twitch-bot
OBJDIR = obj
SOURCES = $(shell ls *.c)
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

all: client

commands: force
	$(MAKE) -C commands

$(OBJDIR)/%.o: %.c
	@mkdir -p obj
	gcc $< `pkg-config --cflags dbus-1` -c -o $@

client: commands $(OBJECTS)
	gcc -o $(OUTPUT) `pkg-config --libs dbus-1` obj/*.o

clean:
	rm -f *.o **/*.o
	rm -f twitch-bot

force:
