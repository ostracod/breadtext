
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define true 1
#define false 0

WINDOW *window;
int32_t windowWidth = -1;
int32_t windowHeight = -1;

void handleResize() {
    int32_t tempWidth;
    int32_t tempHeight;
    getmaxyx(window, tempHeight, tempWidth);
    if (tempWidth == windowWidth && tempHeight == windowHeight) {
        return;
    }
    windowWidth = tempWidth;
    windowHeight = tempHeight;
    
    // TODO: Redraw everything.
    clear();
    printw("%d %d", windowWidth, windowHeight);
}

int main(int argc, const char *argv[]) {
    
    window = initscr();
    handleResize();
    
    while (true) {
        int32_t tempKey = getch();
        if (tempKey == KEY_RESIZE) {
            handleResize();
        }
        if (tempKey == 'q') {
            break;
        }
    }
    
    endwin();
    
    return 0;
}
