
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "script.h"

#define IS_NUM 1
#define IS_STR 2
#define IS_LIST 3
#define IS_FUNC 4
#define COPY 5
#define STR 6
#define NUM 7
#define FLOOR 8
#define LEN 9
#define INS 10
#define REM 11
#define PRESS_KEY 12
#define GET_MODE 13
#define SET_MODE 14
#define GET_SELECTION_CONTENTS 15
#define GET_LINE_COUNT 16
#define GET_LINE_CONTENTS 17
#define GET_CURSOR_CHAR_INDEX 18
#define GET_CURSOR_LINE_INDEX 19
#define SET_CURSOR_POS 20
#define RUN_COMMAND 21
#define NOTIFY_USER 22
#define PROMPT_KEY 23
#define PROMPT_CHARACTER 24
#define BIND_KEY 25
#define BIND_COMMAND 26

builtInScriptFunction_t builtInScriptFunctionNameSet[] = {
    {(int8_t *)"isNum", IS_NUM, 1},
    {(int8_t *)"isStr", IS_STR, 1},
    {(int8_t *)"isList", IS_LIST, 1},
    {(int8_t *)"isFunc", IS_FUNC, 1},
    {(int8_t *)"copy", COPY, 1},
    {(int8_t *)"str", STR, 1},
    {(int8_t *)"num", NUM, 1},
    {(int8_t *)"floor", FLOOR, 1},
    {(int8_t *)"len", LEN, 1},
    {(int8_t *)"ins", INS, 3},
    {(int8_t *)"rem", REM, 2},
    {(int8_t *)"pressKey", PRESS_KEY, 1},
    {(int8_t *)"getMode", GET_MODE, 0},
    {(int8_t *)"setMode", SET_MODE, 1},
    {(int8_t *)"getSelectionContents", GET_SELECTION_CONTENTS, 0},
    {(int8_t *)"getLineCount", GET_LINE_COUNT, 0},
    {(int8_t *)"getLineContents", GET_LINE_CONTENTS, 1},
    {(int8_t *)"getCursorCharIndex", GET_CURSOR_CHAR_INDEX, 0},
    {(int8_t *)"getCursorLineIndex", GET_CURSOR_LINE_INDEX, 0},
    {(int8_t *)"setCursorPos", SET_CURSOR_POS, 2},
    {(int8_t *)"runCommand", RUN_COMMAND, 2},
    {(int8_t *)"notifyUser", NOTIFY_USER, 1},
    {(int8_t *)"promptKey", PROMPT_KEY, 0},
    {(int8_t *)"promptCharacter", PROMPT_CHARACTER, 0},
    {(int8_t *)"bindKey", BIND_KEY, 2},
    {(int8_t *)"bindCommand", BIND_COMMAND, 2}
};

vector_t scriptBodyList;

void initializeScriptingEnvironment() {
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
}

builtInScriptFunction_t *findBuiltInScriptFunctionByName(int8_t *name, int64_t length) {
    int32_t index = 0;
    while (index < sizeof(builtInScriptFunctionNameSet) / sizeof(*builtInScriptFunctionNameSet)) {
        builtInScriptFunction_t *tempFunction = builtInScriptFunctionNameSet + index;
        if (strlen((char *)(tempFunction->name)) == length) {
            if (equalData(tempFunction->name, name, length)) {
                return tempFunction;
            }
        }
        index += 1;
    }
    return NULL;
}

int8_t evaluateExpression(scriptBodyPos_t *scriptBodyPos) {
    scriptBodyPosSkipWhitespace(scriptBodyPos);
    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
    if (isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
        
    }
    // TODO: Handle more kinds of expressions.
    
    return true;
}

int8_t evaluateExpressionStatement(scriptBodyLine_t *scriptBodyLine) {
    scriptBodyPos_t tempScriptBodyPos;
    tempScriptBodyPos.scriptBodyLine = scriptBodyLine;
    tempScriptBodyPos.index = scriptBodyLine->index;
    int8_t tempResult = evaluateExpression(&tempScriptBodyPos);
    seekNextScriptBodyLine(scriptBodyLine);
    return tempResult;
}

int8_t evaluateStatement(scriptBodyLine_t *scriptBodyLine) {
    if (scriptBodyLine->index >= scriptBodyLine->scriptBody->length) {
        return false;
    }
    // TODO: Add other kinds of statements.
    return evaluateExpressionStatement(scriptBodyLine);
}

int8_t importScriptHelper(int8_t *path) {
    int32_t index = 0;
    while (index < scriptBodyList.length) {
        scriptBody_t tempScriptBody;
        getVectorElement(&tempScriptBody, &scriptBodyList, index);
        if (strcmp((char *)(tempScriptBody.path), (char *)path) == 0) {
            return true;
        }
        index += 1;
    }
    scriptBody_t tempScriptBody;
    int8_t tempResult = loadScriptBody(&tempScriptBody, path);
    if (!tempResult) {
        return false;
    }
    pushVectorElement(&scriptBodyList, &tempScriptBody);
    scriptBodyLine_t tempScriptBodyLine;
    tempScriptBodyLine.scriptBody = &tempScriptBody;
    tempScriptBodyLine.index = 0;
    tempScriptBodyLine.number = 1;
    while (true) {
        int8_t tempResult = evaluateStatement(&tempScriptBodyLine);
        if (!tempResult) {
            break;
        }
    }
    return true;
}

int8_t importScript(int8_t *path) {
    path = mallocRealpath(path);
    int8_t output = importScriptHelper(path);
    free(path);
    return output;
}


