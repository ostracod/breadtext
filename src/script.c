
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "script.h"

#define DESTINATION_TYPE_NONE 0
#define DESTINATION_TYPE_VALUE 1
#define DESTINATION_TYPE_CHARACTER 2

typedef struct expressionResult {
    int8_t shouldContinue;
    scriptValue_t value;
    int8_t destinationType;
    void *destination;
} expressionResult_t;

vector_t scriptBodyList;

void initializeScriptingEnvironment() {
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos) {
    expressionResult_t expressionResult;
    expressionResult.shouldContinue = true;
    expressionResult.destinationType = DESTINATION_TYPE_NONE;
    expressionResult.destination = NULL;
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
            scriptBodyPos_t tempScriptBodyPos = *scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
            scriptBuiltInFunction_t *tempBuiltInFunction = findScriptBuiltInFunctionByName(tempText, tempLength);
            if (tempBuiltInFunction != NULL) {
                expressionResult.value.type = SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION;
                *(scriptBuiltInFunction_t **)&(expressionResult.value.data) = tempBuiltInFunction;
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
        }
        // TODO: Handle more types of expressions.
        
        // TODO: Error handling.
        expressionResult.shouldContinue = false;
        return expressionResult;
    }
    // TODO: Handle binary operators.
    
    return expressionResult;
}

int8_t evaluateExpressionStatement(scriptBodyLine_t *scriptBodyLine) {
    scriptBodyPos_t tempScriptBodyPos;
    tempScriptBodyPos.scriptBodyLine = scriptBodyLine;
    tempScriptBodyPos.index = scriptBodyLine->index;
    expressionResult_t tempResult = evaluateExpression(&tempScriptBodyPos);
    seekNextScriptBodyLine(scriptBodyLine);
    return tempResult.shouldContinue;
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


