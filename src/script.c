
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
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
    // For variables, destinationIndex is unused.
    // For lists and strings, destinationIndex is an index in the destination vector_t.
    int64_t destinationIndex;
    // For variables, destination is a pointer to scriptValue_t.
    // For lists and strings, destination is a pointer to vector_t.
    void *destination;
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

void storeValueInExpressionResultDestination(expressionResult_t *expressionResult, scriptValue_t value) {
    int8_t tempType = expressionResult->destinationType;
    if (tempType == DESTINATION_TYPE_NONE) {
        reportScriptError((int8_t *)"Invalid destination.", NULL);
        return;
    }
    if (tempType == DESTINATION_TYPE_VARIABLE) {
        *(scriptValue_t *)(expressionResult->destination) = value;
        return;
    }
    int64_t index = expressionResult->destinationIndex;
    vector_t *tempVector = (vector_t *)(expressionResult->destination);
    if (index < 0 || index >= tempVector->length) {
        reportScriptError((int8_t *)"Index out of range.", NULL);
        return;
    }
    if (tempType == DESTINATION_TYPE_LIST) {
        setVectorElement(tempVector, index, &value);
    } else if (tempType == DESTINATION_TYPE_STRING) {
        if (value.type == SCRIPT_VALUE_TYPE_NUMBER) {
            int8_t tempCharacter = (int8_t)*(double *)(value.data);
            setVectorElement(tempVector, index, &tempCharacter);
        } else {
            reportScriptError((int8_t *)"Bad operand types.", NULL);
        }
    }
}

expressionResult_t evaluateExpression(scriptBaseExpression_t *expression);

scriptValue_t evaluateUnaryExpression(scriptUnaryExpression_t *expression) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult_t tempResult = evaluateExpression(expression->operand);
    if (scriptHasError) {
        return output;
    }
    scriptValue_t tempValue = tempResult.value;
    int8_t tempType = tempValue.type;
    switch (expression->operator->number) {
        case SCRIPT_OPERATOR_NEGATE:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = -*(double *)(tempValue.data);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_NOT:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (*(double *)(tempValue.data) == 0.0);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_NOT:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)~(uint32_t)*(double *)(tempValue.data);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_INCREMENT_PREFIX:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue.data) + 1;
                storeValueInExpressionResultDestination(&tempResult, output);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_DECREMENT_PREFIX:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue.data) - 1;
                storeValueInExpressionResultDestination(&tempResult, output);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_INCREMENT_POSTFIX:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output = tempValue;
                *(double *)(tempValue.data) += 1;
                storeValueInExpressionResultDestination(&tempResult, tempValue);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_DECREMENT_POSTFIX:
        {
            if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                output = tempValue;
                *(double *)(tempValue.data) -= 1;
                storeValueInExpressionResultDestination(&tempResult, tempValue);
            } else {
                reportScriptError((int8_t *)"Bad operand type.", NULL);
                return output;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return output;
}

scriptValue_t evaluateBinaryExpression(scriptBinaryExpression_t *expression) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult_t tempResult1 = evaluateExpression(expression->operand1);
    if (scriptHasError) {
        return output;
    }
    lockScriptValue(&(tempResult1.value));
    expressionResult_t tempResult2 = evaluateExpression(expression->operand2);
    unlockScriptValue(&(tempResult1.value));
    if (scriptHasError) {
        return output;
    }
    scriptValue_t tempValue1 = tempResult1.value;
    scriptValue_t tempValue2 = tempResult2.value;
    int8_t tempType1 = tempValue1.type;
    int8_t tempType2 = tempValue2.type;
    switch (expression->operator->number) {
        case SCRIPT_OPERATOR_ADD:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) + *(double *)&(tempValue2.data);
            } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(tempValue1.data);
                scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)(tempValue2.data);
                vector_t *tempText1 = &(tempHeapValue1->data);
                vector_t *tempText2 = &(tempHeapValue2->data);
                vector_t tempText3;
                copyVector(&tempText3, tempText1);
                removeVectorElement(&tempText3, tempText3.length - 1);
                pushVectorOntoVector(&tempText3, tempText2);
                scriptHeapValue_t *tempHeapValue3 = createScriptHeapValue();
                tempHeapValue3->type = SCRIPT_VALUE_TYPE_STRING;
                tempHeapValue3->data = tempText3;
                output.type = SCRIPT_VALUE_TYPE_STRING;
                *(scriptHeapValue_t **)(output.data) = tempHeapValue3;
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_SUBTRACT:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) - *(double *)&(tempValue2.data);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_MULTIPLY:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) * *(double *)&(tempValue2.data);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_DIVIDE:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber2 = *(double *)(tempValue2.data);
                if (tempNumber2 == 0.0) {
                    reportScriptError((int8_t *)"Divide by zero.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) / tempNumber2;
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_MODULUS:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber2 = *(double *)(tempValue2.data);
                if (tempNumber2 == 0.0) {
                    reportScriptError((int8_t *)"Divide by zero.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fmod(*(double *)(tempValue1.data), tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_AND:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 && tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_OR:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 || tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_XOR:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(!!tempNumber1 ^ !!tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_AND:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 & tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_OR:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 | tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_XOR:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 ^ tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITSHIFT_LEFT:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 << tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITSHIFT_RIGHT:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 >> tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_GREATER:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 > tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_LESS:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 < tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_EQUAL:
        {
            int8_t tempConditionResult = scriptValuesAreEqualShallow(&tempValue1, &tempValue2);
            output.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)(output.data) = (double)tempConditionResult;
            break;
        }
        case SCRIPT_OPERATOR_NOT_EQUAL:
        {
            int8_t tempConditionResult = scriptValuesAreEqualShallow(&tempValue1, &tempValue2);
            output.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)(output.data) = (double)!tempConditionResult;
            break;
        }
        case SCRIPT_OPERATOR_GREATER_OR_EQUAL:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 >= tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_LESS_OR_EQUAL:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 <= tempNumber2);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_IDENTICAL:
        {
            int8_t tempConditionResult = scriptValuesAreIdentical(&tempValue1, &tempValue2);
            output.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)(output.data) = (double)tempConditionResult;
            break;
        }
        case SCRIPT_OPERATOR_NOT_IDENTICAL:
        {
            int8_t tempConditionResult = scriptValuesAreIdentical(&tempValue1, &tempValue2);
            output.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)(output.data) = (double)!tempConditionResult;
            break;
        }
        case SCRIPT_OPERATOR_ASSIGN:
        {
            output = tempValue2;
            storeValueInExpressionResultDestination(&tempResult1, output);
            break;
        }
        case SCRIPT_OPERATOR_ADD_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) + *(double *)(tempValue2.data);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(tempValue1.data);
                scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)(tempValue2.data);
                vector_t *tempText1 = &(tempHeapValue1->data);
                vector_t *tempText2 = &(tempHeapValue2->data);
                removeVectorElement(tempText1, tempText1->length - 1);
                pushVectorOntoVector(tempText1, tempText2);
                output = tempValue1;
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_SUBTRACT_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) - *(double *)(tempValue2.data);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_MULTIPLY_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) * *(double *)(tempValue2.data);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_DIVIDE_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber2 = *(double *)(tempValue2.data);
                if (tempNumber2 == 0.0) {
                    reportScriptError((int8_t *)"Divide by zero.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = *(double *)(tempValue1.data) / tempNumber2;
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_MODULUS_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber2 = *(double *)(tempValue2.data);
                if (tempNumber2 == 0.0) {
                    reportScriptError((int8_t *)"Divide by zero.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fmod(*(double *)(tempValue1.data), tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 && tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 || tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(!!tempNumber1 ^ !!tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_AND_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 & tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_OR_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 | tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 ^ tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 << tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        case SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN:
        {
            if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                uint32_t tempNumber1 = (uint32_t)*(double *)(tempValue1.data);
                uint32_t tempNumber2 = (uint32_t)*(double *)(tempValue2.data);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(tempNumber1 >> tempNumber2);
                storeValueInExpressionResultDestination(&tempResult1, output);
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return output;
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
        case SCRIPT_EXPRESSION_TYPE_LIST:
        {
            scriptListExpression_t *tempExpression = (scriptListExpression_t *)expression;
            vector_t tempValueList;
            createEmptyVector(&tempValueList, sizeof(scriptValue_t));
            // TODO: Lock script values.
            int32_t index = 0;
            while (index < tempExpression->expressionList.length) {
                scriptBaseExpression_t *tempElementExpression;
                getVectorElement(&tempElementExpression, &(tempExpression->expressionList), index);
                expressionResult_t tempResult = evaluateExpression(tempElementExpression);
                if (scriptHasError) {
                    return output;
                }
                pushVectorElement(&tempValueList, &tempResult.value);
                index += 1;
            }
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
            tempHeapValue->data = tempValueList;
            output.value.type = SCRIPT_VALUE_TYPE_LIST;
            *(scriptHeapValue_t **)(output.value.data) = tempHeapValue;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_FUNCTION:
        {
            scriptFunctionExpression_t *tempExpression = (scriptFunctionExpression_t *)expression;
            output.value.type = SCRIPT_VALUE_TYPE_FUNCTION;
            *(scriptBaseFunction_t **)(output.value.data) = tempExpression->function;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_VARIABLE:
        {
            scriptVariableExpression_t *tempExpression = (scriptVariableExpression_t *)expression;
            scriptFrame_t tempFrame;
            if (tempExpression->variable.isGlobal) {
                tempFrame = globalFrame;
            } else {
                tempFrame = localFrame;
            }
            scriptValue_t *tempValuePointer = tempFrame.valueList + tempExpression->variable.scopeIndex;
            output.value = *tempValuePointer;
            output.destinationType = DESTINATION_TYPE_VARIABLE;
            output.destination = tempValuePointer;
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_UNARY:
        {
            output.value = evaluateUnaryExpression((scriptUnaryExpression_t *)expression);
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_BINARY:
        {
            output.value = evaluateBinaryExpression((scriptBinaryExpression_t *)expression);
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_INDEX:
        {
            scriptIndexExpression_t *tempExpression = (scriptIndexExpression_t *)expression;
            expressionResult_t tempResult1 = evaluateExpression(tempExpression->list);
            if (scriptHasError) {
                return output;
            }
            lockScriptValue(&(tempResult1.value));
            expressionResult_t tempResult2 = evaluateExpression(tempExpression->index);
            unlockScriptValue(&(tempResult1.value));
            if (scriptHasError) {
                return output;
            }
            int8_t tempType1 = tempResult1.value.type;
            int8_t tempType2 = tempResult2.value.type;
            if (tempType2 != SCRIPT_VALUE_TYPE_NUMBER) {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
            int64_t index = (int64_t)*(double *)(tempResult2.value.data);
            if (tempType1 == SCRIPT_VALUE_TYPE_STRING) {
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempResult1.value.data);
                vector_t *tempText = &(tempHeapValue->data);
                if (index < 0 || index >= tempText->length - 1) {
                    reportScriptError((int8_t *)"Index out of range.", NULL);
                    return output;
                }
                int8_t tempCharacter;
                getVectorElement(&tempCharacter, tempText, index);
                output.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.value.data) = (double)tempCharacter;
                output.destinationType = DESTINATION_TYPE_STRING;
                output.destinationIndex = index;
                output.destination = tempText;
            } else if (tempType1 == SCRIPT_VALUE_TYPE_LIST) {
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempResult1.value.data);
                vector_t *tempList = &(tempHeapValue->data);
                if (index < 0 || index > tempList->length - 1) {
                    reportScriptError((int8_t *)"Index out of range.", NULL);
                    return output;
                }
                scriptValue_t tempValue;
                getVectorElement(&tempValue, tempList, index);
                output.value = tempValue;
                output.destinationType = DESTINATION_TYPE_LIST;
                output.destinationIndex = index;
                output.destination = tempList;
            } else {
                reportScriptError((int8_t *)"Bad operand types.", NULL);
                return output;
            }
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
            // TODO: Lock script values.
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
            break;
        }
        // TODO: Implement all the other stuff too.
        
        default:
        {
            break;
        }
    }
    if (scriptHasError) {
        scriptErrorLine = statement->scriptBodyLine;
        scriptErrorHasLine = true;
        return false;
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


