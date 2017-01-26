CC = gcc
CFLAGS = -Wall -c
LDFLAGS = -lncurses
DIRBUILD = build
DIRSRC = src
SOURCES := $(shell ls $(DIRSRC)/*.c)
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = $(DIRBUILD)/breadtext

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(DIRBUILD)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm $(OBJECTS) $(EXECUTABLE)

