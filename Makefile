
all: breadtext.c 
	gcc -g -Wall -o breadtext breadtext.c -lncurses

clean: 
	rm breadtext

