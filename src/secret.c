
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textLineTest.h"
#include "display.h"
#include "secret.h"
#include "breadtext.h"

void sleepMilliseconds(int32_t milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int8_t *allocateScreenArray() {
    int64_t tempSize = (viewPortWidth + 1) * viewPortHeight;
    int8_t *output = malloc(tempSize);
    int32_t tempPosX = 0;
    int64_t index = 0;
    while (index < tempSize) {
        if (tempPosX < viewPortWidth) {
            output[index] = ' ';
        } else {
            output[index] = 0;
        }
        tempPosX += 1;
        if (tempPosX >= viewPortWidth + 1) {
            tempPosX = 0;
        }
        index += 1;
    }
    textLine_t *tempLine = topTextLine;
    int32_t tempPosY = 0;
    while (tempPosY < viewPortHeight && tempLine != NULL) {
        int64_t tempLength = tempLine->textAllocation.length;
        int32_t tempPosX = 0;
        int64_t index = 0;
        while (index < tempLength) {
            int8_t tempCharacter = tempLine->textAllocation.text[index];
            if (tempCharacter != '\t') {
                output[tempPosX + tempPosY * (viewPortWidth + 1)] = tempCharacter;
            }
            tempPosX += 1;
            if (tempPosX >= viewPortWidth) {
                tempPosX = 0;
                tempPosY += 1;
            }
            index += 1;
        }
        tempLine = getNextTextLine(tempLine);
        tempPosY += 1;
    }
    return output;
}

void displayScreenArray(int8_t *screenArray) {
    attron(COLOR_PAIR(primaryColorPair));
    int64_t tempPosY = 0;
    while (tempPosY < viewPortHeight) {
        mvprintw(tempPosY, 0, (char *)(screenArray + tempPosY * (viewPortWidth + 1)));
        tempPosY += 1;
    }
    attroff(COLOR_PAIR(primaryColorPair));
}

int8_t craneIntersects(int8_t *screenArray, int64_t posX, int64_t posY, int8_t character) {
    int64_t tempMaxPosY;
    if (character == ' ') {
        tempMaxPosY = posY - 1;
    } else {
        tempMaxPosY = posY;
    }
    int64_t tempPosY = 0;
    while (tempPosY <= tempMaxPosY) {
        int64_t index = posX + tempPosY * (viewPortWidth + 1);
        int8_t tempCharacter = screenArray[index];
        if (tempCharacter != ' ') {
            return true;
        }
        tempPosY += 1;
    }
    return false;
}

void craneSecret() {
    int8_t *tempScreenArray = allocateScreenArray();
    timeout(0);
    int64_t tempCranePosX = viewPortWidth / 2;
    int64_t tempCranePosY = 0;
    int8_t tempCraneCharacter = ' ';
    int8_t tempFallDelay = 0;
    while (true) {
        int32_t tempKey = getch();
        if (tempKey == KEY_RESIZE || tempKey == 27 || tempKey == 'q') {
            break;
        }
        if (tempKey == KEY_LEFT) {
            if (tempCranePosX > 0 && !craneIntersects(tempScreenArray, tempCranePosX - 1, tempCranePosY, tempCraneCharacter)) {
                tempCranePosX -= 1;
            }
        }
        if (tempKey == KEY_RIGHT) {
            if (tempCranePosX < viewPortWidth - 1 && !craneIntersects(tempScreenArray, tempCranePosX + 1, tempCranePosY, tempCraneCharacter)) {
                tempCranePosX += 1;
            }
        }
        if (tempKey == KEY_UP) {
            if (tempCranePosY > 0) {
                tempCranePosY -= 1;
            }
        }
        if (tempKey == KEY_DOWN && !craneIntersects(tempScreenArray, tempCranePosX, tempCranePosY + 1, tempCraneCharacter)) {
            if (tempCranePosY < viewPortHeight - 1) {
                tempCranePosY += 1;
            }
        }
        if (tempKey == ' ') {
            int64_t index = tempCranePosX + tempCranePosY * (viewPortWidth + 1);
            int8_t tempCharacter = tempCraneCharacter;
            tempCraneCharacter = tempScreenArray[index];
            tempScreenArray[index] = tempCharacter; 
        }
        while (true) {
            int32_t tempKey = getch();
            if (tempKey == ERR) {
                break;
            }
        }
        tempFallDelay += 1;
        if (tempFallDelay >= 2) {
            int64_t tempPosY = viewPortHeight - 2;
            while (tempPosY >= 0) {
                int64_t tempPosX = 0;
                while (tempPosX < viewPortWidth) {
                    int64_t tempIndex1 = tempPosX + tempPosY * (viewPortWidth + 1);
                    int64_t tempIndex2 = tempPosX + (tempPosY + 1) * (viewPortWidth + 1);
                    int8_t tempCharacter1 = tempScreenArray[tempIndex1];
                    int8_t tempCharacter2 = tempScreenArray[tempIndex2];
                    if (tempCharacter1 != ' ' && tempCharacter2 == ' ') {
                        tempScreenArray[tempIndex1] = ' ';
                        tempScreenArray[tempIndex2] = tempCharacter1;
                    }
                    tempPosX += 1;
                }
                tempPosY -= 1;
            }
            tempFallDelay = 0;
        }
        displayScreenArray(tempScreenArray);
        attron(COLOR_PAIR(primaryColorPair));
        int64_t tempPosY = 0;
        while (tempPosY < tempCranePosY) {
            mvaddch(tempPosY, tempCranePosX, '|');
            tempPosY += 1;
        }
        if (tempCraneCharacter != ' ') {
            mvaddch(tempCranePosY, tempCranePosX, tempCraneCharacter);
        }
        attroff(COLOR_PAIR(primaryColorPair));
        refresh();
        sleepMilliseconds(50);
    }
    timeout(-1);
    free(tempScreenArray);
    handleResize();
    redrawEverything();
}

void jitterSecret() {
    
}


