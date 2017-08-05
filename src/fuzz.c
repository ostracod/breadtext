
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include "utilities.h"
#include "breadtext.h"
#include "textLine.h"
#include "textAllocation.h"
#include "display.h"
#include "fuzz.h"

#define FUZZ_KEY_AMOUNT 500

fuzzKey_t fuzzKeySet1[] = {
    {'t', NULL},
    {'h', NULL},
    {27, (int8_t *)"ESC"},
    {KEY_LEFT, (int8_t *)"LEFT"},
    {KEY_RIGHT, (int8_t *)"RIGHT"},
    {KEY_UP, (int8_t *)"UP"},
    {KEY_DOWN, (int8_t *)"DOWN"},
    {'\n', (int8_t *)"NEWLINE"},
    {127, (int8_t *)"BACKSPACE"}
};

fuzzKey_t fuzzKeySet2[] = {
    {' ', (int8_t *)"SPACE"},
    {'0', NULL},
    {'1', NULL},
    {'2', NULL},
    {'3', NULL},
    {'4', NULL},
    {'5', NULL},
    {'6', NULL},
    {'7', NULL},
    {'8', NULL},
    {'9', NULL},
    {'a', NULL},
    {'b', NULL},
    {'c', NULL},
    {'d', NULL},
    {'e', NULL},
    {'f', NULL},
    {'g', NULL},
    {'h', NULL},
    {'i', NULL},
    {'j', NULL},
    {'k', NULL},
    {'l', NULL},
    {'m', NULL},
    {'n', NULL},
    {'o', NULL},
    {'p', NULL},
    {'q', NULL},
    {'r', NULL},
    {'s', NULL},
    {'t', NULL},
    {'u', NULL},
    {'v', NULL},
    {'w', NULL},
    {'x', NULL},
    {'y', NULL},
    {'z', NULL},
    {'A', NULL},
    {'B', NULL},
    {'C', NULL},
    {'D', NULL},
    {'E', NULL},
    {'F', NULL},
    {'G', NULL},
    {'H', NULL},
    {'I', NULL},
    {'J', NULL},
    {'K', NULL},
    {'L', NULL},
    {'M', NULL},
    {'N', NULL},
    {'O', NULL},
    {'P', NULL},
    {'Q', NULL},
    {'R', NULL},
    {'S', NULL},
    {'T', NULL},
    {'U', NULL},
    {'V', NULL},
    {'W', NULL},
    {'X', NULL},
    {'Y', NULL},
    {'Z', NULL},
    {'`', NULL},
    {'~', NULL},
    {'!', NULL},
    {'@', NULL},
    {'#', NULL},
    {'$', NULL},
    {'%', NULL},
    {'^', NULL},
    {'&', NULL},
    {'*', NULL},
    {'(', NULL},
    {')', NULL},
    {'-', NULL},
    {'_', NULL},
    {'=', NULL},
    {'+', NULL},
    {'[', NULL},
    {'{', NULL},
    {']', NULL},
    {'}', NULL},
    {'\\', NULL},
    {'|', NULL},
    {';', NULL},
    {':', NULL},
    {'\'', NULL},
    {'"', NULL},
    {',', NULL},
    {'<', NULL},
    {'.', NULL},
    {'>', NULL},
    {'/', NULL},
    {'?', NULL},
    {'\t', (int8_t *)"TAB"},
    {KEY_BTAB, (int8_t *)"BTAB"}
};

fuzzKey_t *fuzzKeyList[FUZZ_KEY_AMOUNT];
int32_t fuzzKeyCount = 0;
int8_t *initialBufferContents;
int8_t *lastBufferContents;

int8_t *mallocBufferContents() {
    int64_t tempLength = 0;
    textLine_t *tempLine = rootTextLine;
    tempLine = getLeftmostTextLine(tempLine);
    while (true) {
        tempLength += tempLine->textAllocation.length;
        tempLine = getNextTextLine(tempLine);
        if (tempLine == NULL) {
            break;
        } else {
            tempLength += 1;
        }
    }
    // For null character.
    tempLength += 1;
    int8_t *output = malloc(tempLength);
    int64_t index = 0;
    tempLine = rootTextLine;
    tempLine = getLeftmostTextLine(tempLine);
    while (true) {
        int64_t tempLength = tempLine->textAllocation.length;
        copyData(output + index, tempLine->textAllocation.text, tempLength);
        index += tempLength;
        tempLine = getNextTextLine(tempLine);
        if (tempLine == NULL) {
            output[index] = 0;
            break;
        } else {
            output[index] = '\n';
            index += 1;
        }
    }
    return output;
}

void putRandomTextIntoLine(textLine_t *line) {
    int32_t tempLength = rand() % 100;
    int8_t tempBuffer[tempLength];
    int32_t index = 0;
    while (index < tempLength) {
        if (rand() % 8 == 0) {
            tempBuffer[index] = 32;
        } else {
            tempBuffer[index] = 32 + rand() % 95;
        }
        index += 1;
    }
    insertTextIntoTextAllocation(&(line->textAllocation), 0, tempBuffer, tempLength);
}

void putRandomTextIntoBuffer() {
    textLine_t *tempLine = rootTextLine;
    putRandomTextIntoLine(tempLine);
    int32_t tempAmount = 10 + rand() % 100;
    int32_t tempCount = 1;
    while (tempCount < tempAmount) {
        textLine_t *tempNextLine = createEmptyTextLine();
        insertTextLineRight(tempLine, tempNextLine);
        tempLine = tempNextLine;
        putRandomTextIntoLine(tempLine);
        tempCount += 1;
    }
}

fuzzKey_t *getNextFuzzKey() {
    if (fuzzKeyCount >= FUZZ_KEY_AMOUNT) {
        endwin();
        exit(0);
    }
    sleepMilliseconds(1);
    fuzzKey_t *tempFuzzKey;
    if (rand() % 5 == 0) {
        int32_t index = rand() % (sizeof(fuzzKeySet1) / sizeof(*fuzzKeySet1));
        tempFuzzKey = fuzzKeySet1 + index;
    } else {
        int32_t index = rand() % (sizeof(fuzzKeySet2) / sizeof(*fuzzKeySet2));
        tempFuzzKey = fuzzKeySet2 + index;
    }
    fuzzKeyList[fuzzKeyCount] = tempFuzzKey;
    fuzzKeyCount += 1;
    lastBufferContents = mallocBufferContents();
    return tempFuzzKey;
}

void startFuzzTest() {
    struct sigaction tempAction;
    memset(&tempAction, 0, sizeof(tempAction));
    tempAction.sa_handler = handleSegmentationFault;
    sigaction(SIGSEGV, &tempAction, NULL);
    shouldUseSystemClipboard = false;
    putRandomTextIntoBuffer();
    redrawEverything();
    initialBufferContents = mallocBufferContents();
}

void handleSegmentationFault(int signum) {
    // Set SEGFAULT behavior back to normal.
    signal(SIGSEGV, SIG_DFL);
    endwin();
    int8_t *tempPath = mallocRealpath((int8_t *)"./failure.txt");
    FILE *tempFile = fopen((char *)tempPath, "w");
    fprintf(tempFile, "START OF BUFFER\n%s\nEND OF BUFFER\n\nKEYS\n", initialBufferContents);
    int32_t index = 0;
    while (index < fuzzKeyCount) {
        fuzzKey_t *tempFuzzKey = fuzzKeyList[index];
        if (tempFuzzKey->name == NULL) {
            fprintf(tempFile, "%c ", (char)tempFuzzKey->key);
        } else {
            fprintf(tempFile, "%s ", tempFuzzKey->name);
        }
        index += 1;
    }
    int32_t tempLineNumber = getTextLineNumber(cursorTextPos.line);
    int32_t tempIndex = getTextPosIndex(&cursorTextPos);
    fprintf(tempFile, "\n\nCURSOR POS\n%d %d\n\nWINDOW DIMENSIONS\n%d %d\n\nSTART OF BUFFER\n%s\nEND OF BUFFER\n",
        tempLineNumber, tempIndex, windowWidth, windowHeight, lastBufferContents);
    fclose(tempFile);
    exit(1);
}
