
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "scriptParse.h"
#include "script.h"
#include "display.h"

#define DESTINATION_TYPE_NONE 0
#define DESTINATION_TYPE_VARIABLE 1
#define DESTINATION_TYPE_LIST 2
#define DESTINATION_TYPE_STRING 3

typedef struct expressionResult {
    scriptValue_t value;
    int8_t destinationType;
    int64_t destinationIndex;
    vector_t *destination;
} expressionResult_t;

vector_t scriptList;
int8_t scriptErrorMessage[1000];
vector_t keyBindingList;
vector_t keyMappingList;
vector_t commandBindingList;
scriptFrame_t globalFrame;
scriptFrame_t localFrame;

void initializeScriptingEnvironment() {
    initializeScriptParsingEnvironment();
    firstHeapValue = NULL;
    createEmptyVector(&scriptList, sizeof(script_t *));
    createEmptyVector(&keyBindingList, sizeof(keyBinding_t));
    createEmptyVector(&keyMappingList, sizeof(keyMapping_t));
    createEmptyVector(&commandBindingList, sizeof(commandBinding_t));
    createEmptyVector(&scriptTestLogMessageList, sizeof(int8_t *));
}

void resetScriptError() {
    scriptHasError = false;
    scriptErrorLine.number = -1;
}

void reportScriptError(int8_t *message, scriptBodyLine_t *line) {
    scriptHasError = true;
    strcpy((char *)scriptErrorMessage, (char *)message);
    scriptErrorHasLine = (line != NULL);
    if (scriptErrorHasLine) {
        scriptErrorLine = *line;
    }
}

void displayScriptError() {
    scriptHeapValue_t *tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        tempHeapValue->lockDepth = 0;
        tempHeapValue = tempHeapValue->next;
    }
    int8_t tempText[1000];
    if (scriptErrorLine.number < 0) {
        sprintf((char *)tempText, "ERROR: %s", (char *)scriptErrorMessage);
    } else {
        int8_t *tempPath = scriptErrorLine.scriptBody->path;
        int64_t tempFileNameIndex = strlen((char *)tempPath);
        while (tempFileNameIndex > 0) {
            int8_t tempCharacter = tempPath[tempFileNameIndex - 1];
            if (tempCharacter == '/') {
                break;
            }
            tempFileNameIndex -= 1;
        }
        sprintf(
            (char *)tempText,
            "ERROR: %s (Line %" PRId64 ", %s)",
            (char *)scriptErrorMessage,
            scriptErrorLine.number,
            (char *)(tempPath + tempFileNameIndex)
        );
    }
    notifyUser(tempText);
}

scriptValue_t invokeFunction(scriptBaseFunction_t *function, scriptValue_t *argumentList, int32_t argumentAmount);

expressionResult_t evaluateExpression(scriptBaseExpression_t *expression) {
    expressionResult_t output;
    output.value.type = SCRIPT_VALUE_TYPE_MISSING;
    output.destinationType = DESTINATION_TYPE_NONE;
    switch (expression->type) {
        case SCRIPT_EXPRESSION_TYPE_NULL:
        {
            output.value.type = SCRIPT_VALUE_TYPE_NULL;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_NUMBER:
        {
            scriptNumberExpression_t *tempExpression = (scriptNumberExpression_t *)expression;
            output.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)(output.value.data) = tempExpression->value;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_STRING:
        {
            scriptStringExpression_t *tempExpression = (scriptStringExpression_t *)expression;
            output.value = convertCharacterVectorToStringValue(tempExpression->text);
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_FUNCTION:
        {
            scriptFunctionExpression_t *tempExpression = (scriptFunctionExpression_t *)expression;
            output.value.type = SCRIPT_VALUE_TYPE_FUNCTION;
            *(scriptBaseFunction_t **)(output.value.data) = tempExpression->function;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_INVOCATION:
        {
            scriptInvocationExpression_t *tempExpression = (scriptInvocationExpression_t *)expression;
            expressionResult_t tempResult = evaluateExpression(tempExpression->function);
            if (scriptHasError) {
                return output;
            }
            if (tempResult.value.type != SCRIPT_VALUE_TYPE_FUNCTION) {
                reportScriptError((int8_t *)"Bad type for function invocation.", NULL);
                return output;
            }
            scriptBaseFunction_t *tempFunction = *(scriptBaseFunction_t **)(tempResult.value.data);
            int32_t tempArgumentAmount = tempExpression->argumentList.length;
            scriptValue_t tempArgumentList[tempArgumentAmount];
            int32_t index = 0;
            while (index < tempArgumentAmount) {
                scriptBaseExpression_t *tempArgumentExpression;
                getVectorElement(&tempArgumentExpression, &(tempExpression->argumentList), index);
                expressionResult_t tempResult = evaluateExpression(tempArgumentExpression);
                if (scriptHasError) {
                    return output;
                }
                tempArgumentList[index] = tempResult.value;
                index += 1;
            }
            output.value = invokeFunction(tempFunction, tempArgumentList, tempArgumentAmount);
            break;
        }
        // TODO: Implement all of the other expression types.
        
        default:
        {
            break;
        }
    }
    return output;
}

int8_t evaluateStatement(scriptValue_t *returnValue, scriptBaseStatement_t *statement) {
    // TODO: Garbage collection.
    
    switch (statement->type) {
        case SCRIPT_STATEMENT_TYPE_EXPRESSION:
        {
            scriptExpressionStatement_t *tempStatement = (scriptExpressionStatement_t *)statement;
            scriptBaseExpression_t *tempExpression = tempStatement->expression;
            evaluateExpression(tempExpression);
            if (scriptHasError) {
                scriptErrorLine = statement->scriptBodyLine;
                scriptErrorHasLine = true;
                return false;
            }
            break;
        }
        // TODO: Implement all the other stuff too.
        
        default:
        {
            break;
        }
    }
    return true;
}

scriptValue_t invokeFunction(scriptBaseFunction_t *function, scriptValue_t *argumentList, int32_t argumentAmount) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    if (argumentAmount != function->argumentAmount) {
        reportScriptError((int8_t *)"Incorrect number of arguments.", NULL);
        return output;
    }
    if (function->type == SCRIPT_FUNCTION_TYPE_CUSTOM) {
        scriptCustomFunction_t *tempFunction = (scriptCustomFunction_t *)function;
        int32_t index;
        int32_t tempLength = tempFunction->scope.variableNameList.length;
        scriptValue_t frameValueList[tempLength];
        index = 0;
        while (index < tempLength) {
            (frameValueList + index)->type = SCRIPT_VALUE_TYPE_MISSING;
            index += 1;
        }
        // TODO: Populate argument variables.
        
        scriptFrame_t lastGlobalFrame = globalFrame;
        scriptFrame_t lastLocalFrame = localFrame;
        localFrame.valueList = frameValueList;
        if (tempFunction->isEntryPoint) {
            globalFrame.valueList = frameValueList;
        }
        index = 0;
        while (index < tempFunction->statementList.length) {
            scriptBaseStatement_t *tempStatement;
            getVectorElement(&tempStatement, &(tempFunction->statementList), index);
            int8_t tempResult = evaluateStatement(&output, tempStatement);
            if (!tempResult) {
                break;
            }
            index += 1;
        }
        globalFrame = lastGlobalFrame;
        localFrame = lastLocalFrame;
        return output;
    }
    if (function->type == SCRIPT_FUNCTION_TYPE_BUILT_IN) {
        scriptBuiltInFunction_t *tempFunction = (scriptBuiltInFunction_t *)function;
        switch (tempFunction->number) {
            case SCRIPT_FUNCTION_NOTIFY_USER:
            {
                scriptValue_t tempValue = argumentList[0];
                scriptValue_t tempStringValue = convertScriptValueToString(tempValue);
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempStringValue.data);
                vector_t *tempText = &(tempHeapValue->data);
                notifyUser(tempText->data);
                break;
            }
            // TODO: Implement all of the other built-in functions.
            
            default: {
                break;
            }
        }
        
    }
    return output;
}

void evaluateScript(script_t *script) {
    invokeFunction((scriptBaseFunction_t *)(script->entryPointFunction), NULL, 0);
}

void parseAndEvaluateScript(scriptBody_t *scriptBody) {
    script_t *tempScript;
    int8_t tempResult = parseScriptBody(&tempScript, scriptBody);
    if (!tempResult) {
        return;
    }
    pushVectorElement(&scriptList, &tempScript);
    evaluateScript(tempScript);
}

void importScriptHelper(int8_t *path) {
    int32_t index = 0;
    while (index < scriptList.length) {
        script_t *tempScript;
        getVectorElement(&tempScript, &scriptList, index);
        int8_t *tempPath = tempScript->scriptBody->path;
        if (strcmp((char *)tempPath, (char *)path) == 0) {
            return;
        }
        index += 1;
    }
    scriptBody_t *tempScriptBody;
    int8_t tempResult = loadScriptBody(&tempScriptBody, path);
    if (!tempResult) {
        reportScriptError((int8_t *)"Import file missing.", NULL);
        return;
    }
    parseAndEvaluateScript(tempScriptBody);
}

void importScript(int8_t *path) {
    path = mallocRealpath(path);
    importScriptHelper(path);
    free(path);
}

int8_t runScript(int8_t *path) {
    resetScriptError();
    importScript(path);
    if (scriptHasError) {
        displayScriptError();
    }
    return !scriptHasError;
}

int8_t runScriptAsText(int8_t *text) {
    resetScriptError();
    scriptBody_t *tempScriptBody;
    loadScriptBodyFromText(&tempScriptBody, text);
    parseAndEvaluateScript(tempScriptBody);
    if (scriptHasError) {
        displayScriptError();
    }
    return !scriptHasError;
}

int8_t invokeKeyBinding(int32_t key) {
    // TODO: Implement.
    
    return false;
}

int32_t invokeKeyMapping(int32_t key) {
    // TODO: Implement.
    
    return key;
}

int8_t invokeCommandBinding(scriptValue_t *destination, int8_t **termList, int32_t termListLength) {
    // TODO: Implement.
    
    return false;
}


