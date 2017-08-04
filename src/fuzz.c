
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include "utilities.h"
#include "breadtext.h"
#include "fuzz.h"

#define FUZZ_KEY_AMOUNT 20

fuzzKey_t fuzzKeySet[] = {
    {'t', NULL},
    {27, (int8_t *)"ESC"}
};

fuzzKey_t *fuzzKeyList[FUZZ_KEY_AMOUNT];
int32_t fuzzKeyCount = 0;

void performFuzzTest() {
    int32_t tempCount = 0;
    while (tempCount < FUZZ_KEY_AMOUNT) {
        int32_t index = rand() % (sizeof(fuzzKeySet) / sizeof(*fuzzKeySet));
        fuzzKey_t *tempFuzzKey = fuzzKeySet + index;
        fuzzKeyList[fuzzKeyCount] = tempFuzzKey;
        fuzzKeyCount += 1;
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
    fprintf(tempFile, "\n");
    fclose(tempFile);
    exit(1);
}
