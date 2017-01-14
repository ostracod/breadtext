
all: breadtext.c 
	gcc -g -Wall -o breadtext breadtext.c

clean: 
	rm breadtext

