
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
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
    scriptValue_t value;
    int8_t destinationType;
    void *destination;
} expressionResult_t;

vector_t scriptBodyList;
scriptScope_t *globalScriptScope;
scriptScope_t *localScriptScope;
vector_t scriptBranchStack;
int8_t scriptHasError = false;
int8_t scriptErrorMessage[1000];
scriptBodyLine_t scriptErrorLine;
int8_t scriptErrorHasLine = false;

void initializeScriptingEnvironment() {
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
    createEmptyVector(&scriptBranchStack, sizeof(scriptBranch_t));
}

void reportScriptErrorWithoutLine(int8_t *message) {
    scriptHasError = true;
    strcpy((char *)scriptErrorMessage, (char *)message);
}

void reportScriptError(int8_t *message, scriptBodyLine_t *line) {
    reportScriptErrorWithoutLine(message);
    scriptErrorLine = *line;
    scriptErrorHasLine = true;
}

int8_t characterIsEndOfScriptLine(int8_t character) {
    return (character == '\n' || character == 0 || character == '#');
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence);
scriptValue_t evaluateScriptBody(scriptBodyLine_t *scriptBodyLine);

void getScriptBodyValueList(vector_t *destination, scriptBodyPos_t *scriptBodyPos, int8_t endCharacter) {
    createEmptyVector(destination, sizeof(scriptValue_t));
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (characterIsEndOfScriptLine(tempCharacter)) {
            reportScriptError((int8_t *)"Unexpected end of expression list.", scriptBodyPos->scriptBodyLine);
            return;
        }
        if (tempCharacter == endCharacter) {
            scriptBodyPos->index += 1;
            break;
        }
        expressionResult_t tempResult = evaluateExpression(scriptBodyPos, 99);
        if (scriptHasError) {
            return;
        }
        pushVectorElement(destination, &(tempResult.value));
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempCharacter == ',') {
            scriptBodyPos->index += 1;
        } else if (tempCharacter == endCharacter) {
            scriptBodyPos->index += 1;
            break;
        } else {
            reportScriptError((int8_t *)"Bad expression list.", scriptBodyPos->scriptBodyLine);
            return;
        }
    }
}

scriptValue_t invokeFunction(scriptValue_t function, vector_t *argumentList) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    int32_t tempArgumentCount = argumentList->length;
    if (function.type == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(function.data);
        customScriptFunction_t *tempFunction = *(customScriptFunction_t **)&(tempHeapValue->data);
        scriptBodyLine_t tempLine = tempFunction->scriptBodyLine;
        scriptBodyPos_t tempScriptBodyPos;
        tempScriptBodyPos.scriptBodyLine = &tempLine;
        tempScriptBodyPos.index = tempLine.index;
        while (true) {
            int8_t tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            tempScriptBodyPos.index += 1;
            if (characterIsEndOfScriptLine(tempCharacter)) {
                reportScriptError((int8_t *)"Invalid function declaration.", &tempLine);
                return output;
            }
            if (tempCharacter == '(') {
                break;
            }
        }
        scriptScope_t tempNewScope = createEmptyScriptScope();
        scriptScope_t *tempScope = scriptBodyAddScope(tempLine.scriptBody, tempNewScope);
        int64_t index = 0;
        while (true) {
            scriptBodyPosSkipWhitespace(&tempScriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (characterIsEndOfScriptLine(tempCharacter)) {
                reportScriptError((int8_t *)"Unexpected end of argument list.", &tempLine);
                return output;
            }
            if (tempCharacter == ')') {
                break;
            }
            if (index >= argumentList->length) {
                reportScriptErrorWithoutLine((int8_t *)"Too many arguments.");
                return output;
            }
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                reportScriptError((int8_t *)"Bad argument name.", &tempLine);
                return output;
            }
            scriptBodyPos_t tempEndScriptBodyPos;
            tempEndScriptBodyPos = tempScriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempEndScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(&tempScriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(&tempScriptBodyPos, &tempEndScriptBodyPos);
            int8_t tempName[tempLength + 1];
            copyData(tempName, tempText, tempLength);
            tempName[tempLength] = 0;
            scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
            scriptVariable_t *tempVariable = scriptScopeAddVariable(tempScope, tempNewVariable);
            getVectorElement(&(tempVariable->value), argumentList, index);
            index += 1;
            tempScriptBodyPos = tempEndScriptBodyPos;
            scriptBodyPosSkipWhitespace(&tempScriptBodyPos);
            tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (tempCharacter == ',') {
                tempScriptBodyPos.index += 1;
            } else if (tempCharacter == ')') {
                break;
            } else {
                reportScriptError((int8_t *)"Bad argument list.", &tempLine);
                return output;
            }
        }
        if (index < argumentList->length) {
            reportScriptErrorWithoutLine((int8_t *)"Not enough arguments.");
            return output;
        }
        scriptBranch_t tempBranch;
        tempBranch.type = SCRIPT_BRANCH_TYPE_FUNCTION;
        tempBranch.shouldIgnore = false;
        tempBranch.line = tempLine;
        pushVectorElement(&scriptBranchStack, &tempBranch);
        int8_t tempResult = seekNextScriptBodyLine(&tempLine);
        if (!tempResult) {
            reportScriptError((int8_t *)"Unexpected end of script", &tempLine);
            return output;
        }
        return evaluateScriptBody(&tempLine);
    } else if (function.type == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION) {
        scriptBuiltInFunction_t *tempFunction = *(scriptBuiltInFunction_t **)&(function.data);
        switch (tempFunction->number) {
            case SCRIPT_FUNCTION_NOTIFY_USER:
            {
                if (tempArgumentCount != 1) {
                    reportScriptErrorWithoutLine((int8_t *)"Expected 1 argument.");
                    return output;
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
                    return output;
                }
                break;
            }
        }
        return output;
    } else {
        reportScriptErrorWithoutLine((int8_t *)"Value is not a function.");
        return output;
    }
}

int8_t escapeScriptCharacter(int8_t character) {
    if (character == 'n') {
        return '\n';
    } else if (character == 't') {
        return '\t';
    } else {
        return character;
    }
}

void storeExpressionResultValueInDestination(expressionResult_t *expressionResult, scriptBodyLine_t *scriptBodyLine) {
    if (expressionResult->destinationType == DESTINATION_TYPE_VALUE) {
        *(scriptValue_t *)(expressionResult->destination) = expressionResult->value;
    } else if (expressionResult->destinationType == DESTINATION_TYPE_CHARACTER) {
        if (expressionResult->value.type == SCRIPT_VALUE_TYPE_NUMBER) {
            *(int8_t *)(expressionResult->destination) = (int8_t)*(double *)&(expressionResult->value.data);
        } else {
            reportScriptError((int8_t *)"Bad operand types.", scriptBodyLine);
        }
    } else {
        reportScriptError((int8_t *)"Invalid destination.", scriptBodyLine);
    }
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence) {
    expressionResult_t expressionResult;
    expressionResult.value.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult.destinationType = DESTINATION_TYPE_NONE;
    expressionResult.destination = NULL;
    scriptBodyPosSkipWhitespace(scriptBodyPos);
    scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX);
    if (tempOperator != NULL) {
        scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
        expressionResult_t tempResult = evaluateExpression(scriptBodyPos, tempOperator->precedence);
        if (scriptHasError) {
            return expressionResult;
        }
        int8_t tempType = tempResult.value.type;
        switch (tempOperator->number) {
            case SCRIPT_OPERATOR_BOOLEAN_NOT:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (*(double *)&(tempResult.value.data) == 0.0);
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_BITWISE_NOT:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (double)~(uint32_t)*(double *)&(tempResult.value.data);
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_INCREMENT_PREFIX:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    if (tempResult.destinationType == DESTINATION_TYPE_VALUE) {
                        scriptValue_t *tempValue = (scriptValue_t *)(tempResult.destination);
                        *(double *)&(tempValue->data) += 1;
                        expressionResult.value = *tempValue;
                    } else if (tempResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                        int8_t *tempValue = (int8_t *)(tempResult.destination);
                        *tempValue += 1;
                        *(double *)&(expressionResult.value.data) = (double)*tempValue;
                    }
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_DECREMENT_PREFIX:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    if (tempResult.destinationType == DESTINATION_TYPE_VALUE) {
                        scriptValue_t *tempValue = (scriptValue_t *)(tempResult.destination);
                        *(double *)&(tempValue->data) -= 1;
                        expressionResult.value = *tempValue;
                    } else if (tempResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                        int8_t *tempValue = (int8_t *)(tempResult.destination);
                        *tempValue -= 1;
                        *(double *)&(expressionResult.value.data) = (double)*tempValue;
                    }
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            default:
            {
                break;
            }
        }
        return expressionResult;
    }
    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
    while (true) {
        if (characterIsEndOfScriptLine(tempFirstCharacter)) {
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
                if (characterIsEndOfScriptLine(tempCharacter)) {
                    reportScriptError((int8_t *)"Unexpected end of string.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                if (tempIsEscaped) {
                    tempCharacter = escapeScriptCharacter(tempCharacter);
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
        if (tempFirstCharacter == '\'') {
            scriptBodyPos->index += 1;
            int8_t tempValue = scriptBodyPosGetCharacter(scriptBodyPos);
            if (characterIsEndOfScriptLine(tempValue)) {
                reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            if (tempValue == '\\') {
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (characterIsEndOfScriptLine(tempCharacter)) {
                    reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPos->index += 1;
                tempValue = escapeScriptCharacter(tempCharacter);
            }
            int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempCharacter != '\'') {
                reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = tempValue;
            break;
        }
        if (tempFirstCharacter == '(') {
            scriptBodyPos->index += 1;
            expressionResult = evaluateExpression(scriptBodyPos, 99);
            if (scriptHasError) {
                return expressionResult;
            }
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempCharacter != ')') {
                reportScriptError((int8_t *)"Missing close parenthesis.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            break;
        }
        if (tempFirstCharacter == '[') {
            scriptBodyPos->index += 1;
            vector_t *tempList = malloc(sizeof(vector_t));
            getScriptBodyValueList(tempList, scriptBodyPos, ']');
            if (scriptHasError) {
                return expressionResult;
            }
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
            *(vector_t **)&(tempHeapValue->data) = tempList;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_LIST;
            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"true")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = 1.0;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"false")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = 0.0;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"null")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NULL;
            break;
        }
        reportScriptError((int8_t *)"Unknown expression type.", scriptBodyPos->scriptBodyLine);
        return expressionResult;
    }
    while (true) {
        int8_t hasProcessedOperator = false;
        while (true) {
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempFirstCharacter == '(') {
                scriptBodyPos->index += 1;
                vector_t tempArgumentList;
                getScriptBodyValueList(&tempArgumentList, scriptBodyPos, ')');
                if (scriptHasError) {
                    return expressionResult;
                }
                scriptValue_t tempValue = invokeFunction(expressionResult.value, &tempArgumentList);
                if (scriptHasError) {
                    if (!scriptErrorHasLine) {
                        scriptErrorLine = *(scriptBodyPos->scriptBodyLine);
                        scriptErrorHasLine = true;
                    }
                    return expressionResult;
                }
                expressionResult.value = tempValue;
                expressionResult.destinationType = DESTINATION_TYPE_NONE;
                expressionResult.destination = NULL;
                hasProcessedOperator = true;
                break;
            }
            if (tempFirstCharacter == '[') {
                scriptBodyPos->index += 1;
                expressionResult_t tempResult = evaluateExpression(scriptBodyPos, 99);
                if (scriptHasError) {
                    return expressionResult;
                }
                int8_t tempType1 = expressionResult.value.type;
                int8_t tempType2 = tempResult.value.type;
                if (tempType2 != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                int64_t index = (int64_t)*(double *)&(tempResult.value.data);
                if (tempType1 == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(expressionResult.value.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                    if (index < 0 || index >= tempText->length - 1) {
                        reportScriptError((int8_t *)"Index out of range.", scriptBodyPos->scriptBodyLine);
                        return expressionResult;
                    }
                    int8_t *tempLocation = (int8_t *)findVectorElement(tempText, index);
                    int8_t tempCharacter = *tempLocation;
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (double)tempCharacter;
                    expressionResult.destinationType = DESTINATION_TYPE_CHARACTER;
                    expressionResult.destination = tempLocation;
                } else if (tempType1 == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(expressionResult.value.data);
                    vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
                    if (index < 0 || index >= tempList->length - 1) {
                        reportScriptError((int8_t *)"Index out of range.", scriptBodyPos->scriptBodyLine);
                        return expressionResult;
                    }
                    scriptValue_t *tempLocation = (scriptValue_t *)findVectorElement(tempList, index);
                    scriptValue_t tempValue = *tempLocation;
                    expressionResult.value = tempValue;
                    expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                    expressionResult.destination = tempLocation;
                } else {
                    reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPosSkipWhitespace(scriptBodyPos);
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (tempCharacter != ']') {
                    reportScriptError((int8_t *)"Missing close bracket.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPos->index += 1;
                hasProcessedOperator = true;
                break;
            }
            scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_UNARY_POSTFIX);
            if (tempOperator != NULL) {
                scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
                int8_t tempType = expressionResult.value.type;
                switch (tempOperator->number) {
                    case SCRIPT_OPERATOR_INCREMENT_POSTFIX:
                    {
                        if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                            if (expressionResult.destinationType == DESTINATION_TYPE_VALUE) {
                                scriptValue_t *tempValue = (scriptValue_t *)(expressionResult.destination);
                                *(double *)&(tempValue->data) += 1;
                            } else if (expressionResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                                int8_t *tempValue = (int8_t *)(expressionResult.destination);
                                *tempValue += 1;
                            }
                        } else {
                            reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DECREMENT_POSTFIX:
                    {
                        if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                            if (expressionResult.destinationType == DESTINATION_TYPE_VALUE) {
                                scriptValue_t *tempValue = (scriptValue_t *)(expressionResult.destination);
                                *(double *)&(tempValue->data) -= 1;
                            } else if (expressionResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                                int8_t *tempValue = (int8_t *)(expressionResult.destination);
                                *tempValue -= 1;
                            }
                        } else {
                            reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                hasProcessedOperator = true;
                break;
            }
            break;
        }
        if (!hasProcessedOperator) {
            break;
        }
    }
    while (true) {
        int8_t hasProcessedOperator = false;
        while (true) {
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_BINARY);
            if (tempOperator != NULL) {
                if (tempOperator->precedence >= precedence) {
                    break;
                }
                scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
                expressionResult_t tempResult = evaluateExpression(scriptBodyPos, tempOperator->precedence);
                if (scriptHasError) {
                    return expressionResult;
                }
                int8_t tempType1 = expressionResult.value.type;
                int8_t tempType2 = tempResult.value.type;
                switch (tempOperator->number) {
                    case SCRIPT_OPERATOR_ADD:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) += *(double *)&(tempResult.value.data);
                        } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                            scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(expressionResult.value.data);
                            scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(tempResult.value.data);
                            vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
                            vector_t *tempText2 = *(vector_t **)&(tempHeapValue2->data);
                            vector_t *tempText3 = malloc(sizeof(vector_t));
                            copyVector(tempText3, tempText1);
                            removeVectorElement(tempText3, tempText3->length - 1);
                            pushVectorOntoVector(tempText3, tempText2);
                            scriptHeapValue_t *tempHeapValue3 = createScriptHeapValue();
                            tempHeapValue3->type = SCRIPT_VALUE_TYPE_STRING;
                            *(vector_t **)&(tempHeapValue3->data) = tempText3;
                            expressionResult.value.type = SCRIPT_VALUE_TYPE_STRING;
                            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue3;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_SUBTRACT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) -= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
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
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DIVIDE:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue = *(double *)&(tempResult.value.data);
                            if (tempValue == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) /= tempValue;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MODULUS:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            if (tempValue2 == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) = fmod(tempValue1, tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_AND:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 && tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_OR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 || tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_XOR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(!!tempValue1 ^ !!tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_AND:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 & tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_OR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 | tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_XOR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 ^ tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_LEFT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 << tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_RIGHT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >> tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_GREATER:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 > tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_LESS:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 < tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 == tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_NOT_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 != tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_GREATER_OR_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >= tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_LESS_OR_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 <= tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_IDENTICAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 == tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_NOT_IDENTICAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 != tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_ASSIGN:
                    {
                        expressionResult.value = tempResult.value;
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_ADD_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) += *(double *)&(tempResult.value.data);
                        } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                            scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(expressionResult.value.data);
                            scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(tempResult.value.data);
                            vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
                            vector_t *tempText2 = *(vector_t **)&(tempHeapValue2->data);
                            removeVectorElement(tempText1, tempText1->length - 1);
                            pushVectorOntoVector(tempText1, tempText2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_SUBTRACT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) -= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MULTIPLY_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) *= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DIVIDE_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue = *(double *)&(tempResult.value.data);
                            if (tempValue == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) /= tempValue;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MODULUS_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            if (tempValue2 == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) = fmod(tempValue1, tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 && tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 || tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(!!tempValue1 ^ !!tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_AND_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 & tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_OR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 | tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 ^ tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 << tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >> tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
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

int8_t evaluateStatement(scriptValue_t *returnValue, scriptBodyLine_t *scriptBodyLine) {
    if (scriptBodyLine->index >= scriptBodyLine->scriptBody->length) {
        return false;
    }
    scriptBodyPos_t scriptBodyPos;
    scriptBodyPos.scriptBodyLine = scriptBodyLine;
    scriptBodyPos.index = scriptBodyLine->index;
    scriptBodyPosSkipWhitespace(&scriptBodyPos);
    scriptBranch_t *currentBranch = findVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
    vector_t *currentScopeStack = &(currentBranch->line.scriptBody->scopeStack);
    globalScriptScope = findVectorElement(currentScopeStack, 0);
    localScriptScope = findVectorElement(currentScopeStack, currentScopeStack->length - 1);
    if (currentBranch->shouldIgnore) {
        scriptBranch_t *lastBranch = findVectorElement(&scriptBranchStack, scriptBranchStack.length - 2);
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_IF;
            tempBranch.shouldIgnore = true;
            tempBranch.hasExecuted = false;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"else")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                if (!lastBranch->shouldIgnore && !currentBranch->hasExecuted) {
                    scriptBodyPosSkipWhitespace(&scriptBodyPos);
                    if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
                        expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                        if (scriptHasError) {
                            return false;
                        }
                        if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempCondition = *(double *)&(tempResult.value.data);
                            if (tempCondition) {
                                currentBranch->shouldIgnore = false;
                                currentBranch->hasExecuted = true;
                            }
                        } else {
                            reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                            return false;
                        }
                    } else {
                        currentBranch->shouldIgnore = false;
                        currentBranch->hasExecuted = true;
                    }
                }
                return seekNextScriptBodyLine(scriptBodyLine);
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"end")) {
            removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        return seekNextScriptBodyLine(scriptBodyLine);
    } else {
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
                if (scriptHasError) {
                    return false;
                }
                tempVariable->value = tempResult.value;
            }
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
            expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
            if (scriptHasError) {
                return false;
            }
            if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempCondition = *(double *)&(tempResult.value.data);
                int8_t tempShouldExecute = (tempCondition != 0);
                scriptBranch_t tempBranch;
                tempBranch.type = SCRIPT_BRANCH_TYPE_IF;
                tempBranch.shouldIgnore = !tempShouldExecute;
                tempBranch.hasExecuted = tempShouldExecute;
                tempBranch.line = *scriptBodyLine;
                pushVectorElement(&scriptBranchStack, &tempBranch);
                return seekNextScriptBodyLine(scriptBodyLine);
            } else {
                reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"else")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                currentBranch->shouldIgnore = true;
                return seekNextScriptBodyLine(scriptBodyLine);
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"while")) {
            expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
            if (scriptHasError) {
                return false;
            }
            if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempCondition = *(double *)&(tempResult.value.data);
                int8_t tempShouldExecute = (tempCondition != 0);
                scriptBranch_t tempBranch;
                tempBranch.type = SCRIPT_BRANCH_TYPE_WHILE;
                tempBranch.shouldIgnore = !tempShouldExecute;
                tempBranch.line = *scriptBodyLine;
                pushVectorElement(&scriptBranchStack, &tempBranch);
                return seekNextScriptBodyLine(scriptBodyLine);
            } else {
                reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"func")) {
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
            customScriptFunction_t *tempScriptFunction = malloc(sizeof(customScriptFunction_t));
            tempScriptFunction->scriptBodyLine = *scriptBodyLine;
            scriptHeapValue_t *tempHeapValue = malloc(sizeof(scriptHeapValue_t));
            tempHeapValue->type = SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION;
            *(customScriptFunction_t **)&(tempHeapValue->data) = tempScriptFunction;
            scriptValue_t tempValue;
            tempValue.type = SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION;
            *(scriptHeapValue_t **)&(tempValue.data) = tempHeapValue;
            tempVariable->value = tempValue;
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_FUNCTION;
            tempBranch.shouldIgnore = true;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"end")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                return seekNextScriptBodyLine(scriptBodyLine);
            }
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_WHILE) {
                *scriptBodyLine = currentBranch->line;
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                return true;
            }
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_FUNCTION) {
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                // TODO: Clean up.
                vector_t *tempScopeStack = &(scriptBodyLine->scriptBody->scopeStack);
                removeVectorElement(tempScopeStack, tempScopeStack->length - 1);
                returnValue->type = SCRIPT_VALUE_TYPE_MISSING;
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"break")) {
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid break statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t *tempBranch = findVectorElement(&scriptBranchStack, index);
                tempBranch->shouldIgnore = true;
                if (tempBranch->type == SCRIPT_BRANCH_TYPE_WHILE) {
                    break;
                }
                index -= 1;
            }
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"continue")) {
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid continue statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t tempBranch;
                getVectorElement(&tempBranch, &scriptBranchStack, index);
                removeVectorElement(&scriptBranchStack, index);
                if (tempBranch.type == SCRIPT_BRANCH_TYPE_WHILE) {
                    *scriptBodyLine = tempBranch.line;
                    return true;
                }
                index -= 1;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"ret")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (!characterIsEndOfScriptLine(tempCharacter)) {
                expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                if (scriptHasError) {
                    return false;
                }
                *returnValue = tempResult.value;
            } else {
                returnValue->type = SCRIPT_VALUE_TYPE_MISSING;
            }
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid return statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t tempBranch;
                getVectorElement(&tempBranch, &scriptBranchStack, index);
                removeVectorElement(&scriptBranchStack, index);
                if (tempBranch.type == SCRIPT_BRANCH_TYPE_FUNCTION) {
                    return false;
                }
                index -= 1;
            }
        }
        scriptBodyPos_t tempScriptBodyPos;
        tempScriptBodyPos.scriptBodyLine = scriptBodyLine;
        tempScriptBodyPos.index = scriptBodyLine->index;
        evaluateExpression(&tempScriptBodyPos, 99);
        if (scriptHasError) {
            return false;
        }
        return seekNextScriptBodyLine(scriptBodyLine);
    }
}

scriptValue_t evaluateScriptBody(scriptBodyLine_t *scriptBodyLine) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    while (true) {
        int8_t tempResult = evaluateStatement(&output, scriptBodyLine);
        if (!tempResult || scriptHasError) {
            break;
        }
    }
    return output;
}

void importScriptHelper(int8_t *path) {
    int32_t index = 0;
    while (index < scriptBodyList.length) {
        scriptBody_t tempScriptBody;
        getVectorElement(&tempScriptBody, &scriptBodyList, index);
        if (strcmp((char *)(tempScriptBody.path), (char *)path) == 0) {
            return;
        }
        index += 1;
    }
    scriptBody_t tempScriptBody;
    int8_t tempResult = loadScriptBody(&tempScriptBody, path);
    if (!tempResult) {
        reportScriptErrorWithoutLine((int8_t *)"Import file missing.");
        return;
    }
    createEmptyVector(&(tempScriptBody.scopeStack), sizeof(scriptScope_t));
    scriptBodyAddScope(&tempScriptBody, createEmptyScriptScope());
    pushVectorElement(&scriptBodyList, &tempScriptBody);
    scriptBodyLine_t tempScriptBodyLine;
    tempScriptBodyLine.scriptBody = &tempScriptBody;
    tempScriptBodyLine.index = 0;
    tempScriptBodyLine.number = 1;
    scriptBranch_t tempBranch;
    tempBranch.type = SCRIPT_BRANCH_TYPE_ROOT;
    tempBranch.shouldIgnore = false;
    tempBranch.line = tempScriptBodyLine;
    pushVectorElement(&scriptBranchStack, &tempBranch);
    evaluateScriptBody(&tempScriptBodyLine);
}

void importScript(int8_t *path) {
    path = mallocRealpath(path);
    importScriptHelper(path);
    free(path);
}

int8_t runScript(int8_t *path) {
    scriptHasError = false;
    scriptErrorLine.number = -1;
    importScript(path);
    if (scriptHasError) {
        int8_t tempText[1000];
        if (scriptErrorLine.number < 0) {
            sprintf((char *)tempText, "ERROR: %s", (char *)scriptErrorMessage);
        } else {
            sprintf((char *)tempText, "ERROR: %s (Line %lld)", (char *)scriptErrorMessage, scriptErrorLine.number);
        }
        notifyUser(tempText);
    }
    return !scriptHasError;
}
