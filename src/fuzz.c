
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include "utilities.h"
#include "breadtext.h"
#include "fuzz.h"

#define FUZZ_KEY_AMOUNT 500

fuzzKey_t fuzzKeySet[] = {
    {'t', NULL},
    {27, (int8_t *)"ESC"}
};

void performFuzzTest() {
    int32_t tempCount = 0;
    while (tempCount < FUZZ_KEY_AMOUNT) {
        int32_t index = rand() % (sizeof(fuzzKeySet) / sizeof(*fuzzKeySet));
        fuzzKey_t *tempFuzzKey = fuzzKeySet + index;
        handleKey(tempFuzzKey->key);
        sleepMilliseconds(10);
        tempCount += 1;
    }
}

void handleSegmentationFault(int signum) {
    // Set SEGFAULT behavior back to normal.
    signal(SIGSEGV, SIG_DFL);
    endwin();
    printf("Failure!!!\n");
    exit(1);
}
