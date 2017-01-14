
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define true 1
#define false 0

WINDOW *window;
int32_t windowWidth;
int32_t windowHeight;

int main(int argc, const char *argv[]) {
    
    window = initscr();
    getmaxyx(window, windowWidth, windowHeight);
    
    endwin();
    
    printf("%d %d\n", windowWidth, windowHeight);
    
    return 0;
}
