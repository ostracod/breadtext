
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "breadtext.h"
#include "textLine.h"
#include "textAllocation.h"
#include "display.h"
#include "systematicTest.h"

namedKey_t namedKeySet[] = {
    {27, (int8_t *)"ESC"},
    {KEY_LEFT, (int8_t *)"LEFT"},
    {KEY_RIGHT, (int8_t *)"RIGHT"},
    {KEY_UP, (int8_t *)"UP"},
    {KEY_DOWN, (int8_t *)"DOWN"},
    {'\n', (int8_t *)"NEWLINE"},
    {127, (int8_t *)"BACKSPACE"},
    {'\t', (int8_t *)"TAB"},
    {KEY_BTAB, (int8_t *)"BTAB"}
};

int32_t convertNameToKey(int8_t *name) {
    int index = 0;
    while (index < sizeof(namedKeySet) / sizeof(*namedKeySet)) {
        namedKey_t *tempNamedKey = namedKeySet + index;
        if (strcmp((char *)name, (char *)(tempNamedKey->name)) == 0) {
            return tempNamedKey->key;
        }
        index += 1;
    }
    return -1;
}

void simulateKeyPress(int32_t key) {
    systematicTestKey = key;
    while (true) {
        int32_t tempKey = getNextKey();
        if (tempKey < 0) {
            break;
        }
        handleKey(tempKey);
    }
    sleepMilliseconds(1);
}

int8_t processSystematicTestCommand(int8_t *command) {
    // TODO: Parse and perform the command.
    return true;
}

int8_t runSystematicTest() {
    int8_t output = true;
    FILE *tempFile = fopen((char *)systematicTestDefinitionPath, "r");
    if (tempFile == NULL) {
        // TODO: Report missing file.
        return false;
    }
    while (true) {
        int8_t *tempText = NULL;
        size_t tempSize = 0;
        int64_t tempCount = getline((char **)&tempText, &tempSize, tempFile);
        if (tempCount < 0) {
            break;
        }
        int64_t tempLength = strlen((char *)tempText);
        while (tempLength > 0) {
            int64_t index = tempLength - 1;
            if (tempText[index] == '\n') {
                tempText[index] = 0;
                tempLength = index;
            } else {
                break;
            }
        }
        if (tempLength > 0) {
            int8_t tempResult = processSystematicTestCommand(tempText);
            if (!tempResult) {
                output = false;
            }
        }
        free(tempText);
    }
    fclose(tempFile);
    return output;
}

