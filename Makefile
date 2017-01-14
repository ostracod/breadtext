
all: breadtext.c 
	gcc -g -Wall -o breadtext -lncurses breadtext.c

clean: 
	rm breadtext

