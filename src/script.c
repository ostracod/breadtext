
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "script.h"

vector_t scriptBodyList;

void initializeScriptingEnvironment() {
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
}

int8_t isFirstIdentifierCharacter(int8_t character) {
    return ((character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || character == '_');
}

int8_t isIdentifierCharacter(int8_t character) {
    return ((character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || (character >= '0' && character <= '9')
            || character == '_');
}

int8_t *findEndOfIdentifier(int8_t *text) {
    while (true) {
        int8_t tempCharacter = *text;
        if (!isIdentifierCharacter(tempCharacter)) {
            return text;
        }
        text += 1;
    }
}

int8_t evaluateExpression(scriptBodyPos_t *scriptBodyPos) {
    scriptBodyPosSkipWhitespace(scriptBodyPos);
    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
    // TODO: Actually evaluate the expression.
    
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


