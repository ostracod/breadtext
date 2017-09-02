
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

