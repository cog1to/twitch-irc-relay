OBJDIR = ../obj
SOURCES = $(shell ls *.c)
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

all: $(OBJECTS)

$(OBJDIR)/%.o: %.c
	gcc -c $< -o $@
