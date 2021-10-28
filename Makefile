CC = gcc
CFLAGS = -Wall -fcommon -c
LDFLAGS = -lncurses -lm
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

install:
	mv $(EXECUTABLE) /usr/local/bin/breadtext
	mkdir -p ~/.breadtextsyntax
	cp ./syntax/* ~/.breadtextsyntax

clean: 
	rm $(OBJECTS) $(EXECUTABLE)

