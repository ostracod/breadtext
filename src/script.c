
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "script.h"
#include "display.h"

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
scriptScope_t *globalScriptScope;
scriptScope_t *localScriptScope;
int8_t scriptHasError = false;
int8_t scriptErrorMessage[1000];
scriptBodyLine_t scriptErrorLine;

void initializeScriptingEnvironment() {
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
    globalScriptScope = createEmptyScriptScope();
    localScriptScope = globalScriptScope;
}

void reportScriptErrorWithoutLine(int8_t *message) {
    scriptHasError = true;
    strcpy((char *)scriptErrorMessage, (char *)message);
}

void reportScriptError(int8_t *message, scriptBodyLine_t *line) {
    reportScriptErrorWithoutLine(message);
    scriptErrorLine = *line;
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence);

// Returns whether the operation was successful.
int8_t getFunctionInvocationArguments(vector_t *destination, scriptBodyPos_t *scriptBodyPos) {
    createEmptyVector(destination, sizeof(vector_t));
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempCharacter == '\n' || tempCharacter == 0) {
            reportScriptError((int8_t *)"Unexpected end of invocation.", scriptBodyPos->scriptBodyLine);
            return false;
        }
        if (tempCharacter == ')') {
            break;
        }
        expressionResult_t tempResult = evaluateExpression(scriptBodyPos, 99);
        if (!tempResult.shouldContinue) {
            return false;
        }
        pushVectorElement(destination, &(tempResult.value));
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempCharacter == ',') {
            scriptBodyPos->index += 1;
        } else if (tempCharacter == ')') {
            break;
        } else {
            reportScriptError((int8_t *)"Bad function invocation.", scriptBodyPos->scriptBodyLine);
            return false;
        }
    }
    return true;
}

// Returns whether the operation was successful.
int8_t invokeFunction(scriptValue_t *destination, scriptValue_t function, vector_t *argumentList) {
    int32_t tempArgumentCount = argumentList->length;
    if (function.type == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION) {
        scriptBuiltInFunction_t *tempFunction = *(scriptBuiltInFunction_t **)&(function.data);
        switch (tempFunction->number) {
            case SCRIPT_FUNCTION_NOTIFY_USER:
            {
                if (tempArgumentCount != 1) {
                    reportScriptErrorWithoutLine((int8_t *)"Expected 1 argument.");
                    return false;
                }
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                scriptValue_t tempStringValue = convertScriptValueToString(tempValue);
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempStringValue.data);
                vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                notifyUser(tempText->data);
                break;
            }
            default:
            {
                if (tempArgumentCount != 0) {
                    reportScriptErrorWithoutLine((int8_t *)"Unknown function.");
                    return false;
                }
                break;
            }
        }
    }
    return true;
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence) {
    expressionResult_t expressionResult;
    expressionResult.shouldContinue = true;
    expressionResult.value.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult.destinationType = DESTINATION_TYPE_NONE;
    expressionResult.destination = NULL;
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempFirstCharacter == '\n' || tempFirstCharacter == 0) {
            return expressionResult;
        }
        if (isScriptNumberCharacter(tempFirstCharacter)) {
            scriptBodyPos_t tempScriptBodyPos = *scriptBodyPos;
            scriptBodyPosSeekEndOfNumber(&tempScriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
            int8_t tempText[tempLength + 1];
            copyData(tempText, getScriptBodyPosPointer(scriptBodyPos), tempLength);
            tempText[tempLength] = 0;
            double tempNumber;
            sscanf((char *)tempText, "%lf", &tempNumber);
            // TODO: Handle malformed numbers.
            // TODO: Handle hexadecimal numbers.
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = tempNumber;
            *scriptBodyPos = tempScriptBodyPos;
            break;
        }
        if (isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
            scriptBodyPos_t tempScriptBodyPos = *scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
            scriptVariable_t *tempVariable = scriptScopeFindVariableWithNameLength(localScriptScope, tempText, tempLength);
            if (tempVariable != NULL) {
                expressionResult.value = tempVariable->value;
                expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                expressionResult.destination = &(tempVariable->value);
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
            if (localScriptScope != globalScriptScope) {
                tempVariable = scriptScopeFindVariableWithNameLength(globalScriptScope, tempText, tempLength);
                if (tempVariable != NULL) {
                    expressionResult.value = tempVariable->value;
                    expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                    expressionResult.destination = &(tempVariable->value);
                    *scriptBodyPos = tempScriptBodyPos;
                    break;
                }
            }
            scriptBuiltInFunction_t *tempBuiltInFunction = findScriptBuiltInFunctionByName(tempText, tempLength);
            if (tempBuiltInFunction != NULL) {
                expressionResult.value.type = SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION;
                *(scriptBuiltInFunction_t **)&(expressionResult.value.data) = tempBuiltInFunction;
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
        }
        if (tempFirstCharacter == '"') {
            vector_t *tempText = malloc(sizeof(vector_t));
            createEmptyVector(tempText, 1);
            scriptBodyPos->index += 1;
            int8_t tempIsEscaped = false;
            while (true) {
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (tempCharacter == '\n' || tempCharacter == 0) {
                    reportScriptError((int8_t *)"Unexpected end of string.", scriptBodyPos->scriptBodyLine);
                    expressionResult.shouldContinue = false;
                    return expressionResult;
                }
                if (tempIsEscaped) {
                    if (tempCharacter == 'n') {
                        tempCharacter = '\n';
                    } else if (tempCharacter == 't') {
                        tempCharacter = '\t';
                    }
                    pushVectorElement(tempText, &tempCharacter);
                    tempIsEscaped = false;
                } else {
                    if (tempCharacter == '"') {
                        break;
                    } else if (tempCharacter == '\\') {
                        tempIsEscaped = true;
                    } else {
                        pushVectorElement(tempText, &tempCharacter);
                    }
                }
                scriptBodyPos->index += 1;
            }
            scriptBodyPos->index += 1;
            int8_t tempCharacter = 0;
            pushVectorElement(tempText, &tempCharacter);
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_STRING;
            *(vector_t **)&(tempHeapValue->data) = tempText;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_STRING;
            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue;
            break;
        }
        // TODO: Handle more types of expressions.
        
        reportScriptError((int8_t *)"Unknown expression type.", scriptBodyPos->scriptBodyLine);
        expressionResult.shouldContinue = false;
        return expressionResult;
    }
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t hasProcessedOperator = false;
        while (true) {
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempFirstCharacter == '(') {
                scriptBodyPos->index += 1;
                vector_t tempArgumentList;
                int8_t tempResult = getFunctionInvocationArguments(&tempArgumentList, scriptBodyPos);
                if (!tempResult) {
                    expressionResult.shouldContinue = false;
                    return expressionResult;
                }
                scriptValue_t tempValue;
                tempResult = invokeFunction(&tempValue, expressionResult.value, &tempArgumentList);
                if (!tempResult) {
                    if (scriptHasError) {
                        scriptErrorLine = *(scriptBodyPos->scriptBodyLine);
                    }
                    expressionResult.shouldContinue = false;
                    return expressionResult;
                }
                expressionResult.value = tempValue;
                expressionResult.destinationType = DESTINATION_TYPE_NONE;
                expressionResult.destination = NULL;
                hasProcessedOperator = true;
                break;
            }
            scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_BINARY);
            if (tempOperator != NULL) {
                if (tempOperator->precedence >= precedence) {
                    break;
                }
                scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
                expressionResult_t tempResult = evaluateExpression(scriptBodyPos, tempOperator->precedence);
                if (!tempResult.shouldContinue) {
                    expressionResult.shouldContinue = false;
                    if (scriptHasError) {
                        return expressionResult;
                    }
                }
                int8_t tempType1 = expressionResult.value.type;
                int8_t tempType2 = tempResult.value.type;
                switch (tempOperator->number) {
                    case SCRIPT_OPERATOR_ADD:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) += *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            expressionResult.shouldContinue = false;
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MULTIPLY:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) *= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            expressionResult.shouldContinue = false;
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_ASSIGN:
                    {
                        if (expressionResult.destinationType == DESTINATION_TYPE_VALUE) {
                            *(scriptValue_t *)(expressionResult.destination) = tempResult.value;
                        } else {
                            reportScriptError((int8_t *)"Invalid destination.", scriptBodyPos->scriptBodyLine);
                            expressionResult.shouldContinue = false;
                            return expressionResult;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                expressionResult.destinationType = DESTINATION_TYPE_NONE;
                expressionResult.destination = NULL;
                hasProcessedOperator = true;
                break;
            }
            
            break;
        }
        if (!hasProcessedOperator) {
            break;
        }
    }
    return expressionResult;
}

int8_t evaluateStatement(scriptBodyLine_t *scriptBodyLine) {
    if (scriptBodyLine->index >= scriptBodyLine->scriptBody->length) {
        return false;
    }
    scriptBodyPos_t scriptBodyPos;
    scriptBodyPos.scriptBodyLine = scriptBodyLine;
    scriptBodyPos.index = scriptBodyLine->index;
    scriptBodyPosSkipWhitespace(&scriptBodyPos);
    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
    if (isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"dec")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                reportScriptError((int8_t *)"Missing declaration name.", scriptBodyLine);
                return false;
            }
            scriptBodyPos_t tempScriptBodyPos;
            tempScriptBodyPos = scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(&scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(&scriptBodyPos, &tempScriptBodyPos);
            int8_t tempName[tempLength + 1];
            copyData(tempName, tempText, tempLength);
            tempName[tempLength] = 0;
            scriptVariable_t *tempVariable = scriptScopeFindVariable(localScriptScope, tempName);
            if (tempVariable == NULL) {
                scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
                tempVariable = scriptScopeAddVariable(localScriptScope, tempNewVariable);
            }
            scriptBodyPos = tempScriptBodyPos;
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (tempCharacter == '=') {
                scriptBodyPos.index += 1;
                scriptBodyPosSkipWhitespace(&scriptBodyPos);
                expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                if (!tempResult.shouldContinue) {
                    return false;
                }
                tempVariable->value = tempResult.value;
            }
            seekNextScriptBodyLine(scriptBodyLine);
            return true;
        }
    }
    scriptBodyPos_t tempScriptBodyPos;
    tempScriptBodyPos.scriptBodyLine = scriptBodyLine;
    tempScriptBodyPos.index = scriptBodyLine->index;
    expressionResult_t tempResult = evaluateExpression(&tempScriptBodyPos, 99);
    seekNextScriptBodyLine(scriptBodyLine);
    return tempResult.shouldContinue;
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

int8_t runScript(int8_t *path) {
    scriptHasError = false;
    scriptErrorLine.number = -1;
    int8_t output = importScript(path);
    if (scriptHasError) {
        int8_t tempText[1000];
        if (scriptErrorLine.number < 0) {
            sprintf((char *)tempText, "ERROR: %s", (char *)scriptErrorMessage);
        } else {
            sprintf((char *)tempText, "ERROR: %s (Line %lld)", (char *)scriptErrorMessage, scriptErrorLine.number);
        }
        notifyUser(tempText);
    }
    return output;
}
