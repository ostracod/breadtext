
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

#define FUZZ_KEY_AMOUNT 20

fuzzKey_t fuzzKeySet[] = {
    {'t', NULL},
    {27, (int8_t *)"ESC"}
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

void performFuzzTest() {
    putRandomTextIntoBuffer();
    redrawEverything();
    initialBufferContents = mallocBufferContents();
    int32_t tempCount = 0;
    while (tempCount < FUZZ_KEY_AMOUNT) {
        int32_t index = rand() % (sizeof(fuzzKeySet) / sizeof(*fuzzKeySet));
        fuzzKey_t *tempFuzzKey = fuzzKeySet + index;
        fuzzKeyList[fuzzKeyCount] = tempFuzzKey;
        fuzzKeyCount += 1;
        lastBufferContents = mallocBufferContents();
        handleKey(tempFuzzKey->key);
        sleepMilliseconds(1);
        tempCount += 1;
    }
    // TEST CODE.
    handleSegmentationFault(0);
}

void handleSegmentationFault(int signum) {
    // Set SEGFAULT behavior back to normal.
    signal(SIGSEGV, SIG_DFL);
    endwin();
    int8_t *tempPath = mallocRealpath((int8_t *)"./failure.txt");
    FILE *tempFile = fopen((char *)tempPath, "w");
    fprintf(tempFile, "%s\n\n", initialBufferContents);
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
    fprintf(tempFile, "\n\n%s\n", lastBufferContents);
    fclose(tempFile);
    exit(1);
}
