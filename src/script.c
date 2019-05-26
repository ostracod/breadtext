
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "scriptValue.h"
#include "scriptParse.h"
#include "script.h"

void initializeScriptingEnvironment() {
    
}

int8_t runScript(int8_t *path) {
    return true;
}

int8_t runScriptAsText(int8_t *text) {
    return true;
}

int8_t invokeKeyBinding(int32_t key) {
    return false;
}

int32_t invokeKeyMapping(int32_t key) {
    return key;
}

int8_t invokeCommandBinding(scriptValue_t *destination, int8_t **termList, int32_t termListLength) {
    return false;
}


