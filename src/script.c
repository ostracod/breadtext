
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "scriptParse.h"
#include "script.h"
#include "display.h"
#include "selection.h"
#include "motion.h"
#include "textCommand.h"

#define STATEMENT_JUMP_NONE 0
#define STATEMENT_JUMP_ERROR 1
#define STATEMENT_JUMP_BREAK 2
#define STATEMENT_JUMP_CONTINUE 3
#define STATEMENT_JUMP_RETURN 4

#define DESTINATION_TYPE_NONE 0
#define DESTINATION_TYPE_VARIABLE 1
#define DESTINATION_TYPE_LIST 2
#define DESTINATION_TYPE_STRING 3

#define KEY_HASH_TABLE_SIZE 31

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
vector_t keyBindingHashTable[KEY_HASH_TABLE_SIZE];
vector_t keyMappingHashTable[KEY_HASH_TABLE_SIZE];
vector_t commandBindingList;
scriptFrame_t *globalFrame;
scriptFrame_t *localFrame;
int32_t garbageCollectionDelay = 0;

void initializeScriptingEnvironment() {
    initializeScriptParsingEnvironment();
    firstHeapValue = NULL;
    firstScriptFrame = NULL;
    createEmptyVector(&scriptList, sizeof(script_t *));
    createEmptyVector(&commandBindingList, sizeof(commandBinding_t));
    createEmptyVector(&scriptTestLogMessageList, sizeof(int8_t *));
    int32_t index;
    index = 0;
    while (index < KEY_HASH_TABLE_SIZE) {
        createEmptyVector(keyBindingHashTable + index, sizeof(keyBinding_t));
        createEmptyVector(keyMappingHashTable + index, sizeof(keyMapping_t));
        index += 1;
    }
}

void addScriptTestLogMessage(int8_t *text) {
    int8_t *tempText = malloc(strlen((char *)text) + 1);
    strcpy((char *)tempText, (char *)text);
    pushVectorElement(&scriptTestLogMessageList, &tempText);
}

void garbageCollectScriptHeapValues() {
    // Mark all heap values for deletion.
    scriptHeapValue_t *tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        tempHeapValue->isMarked = true;
        tempHeapValue = tempHeapValue->next;
    }
    // Unmark all reachable values.
    scriptFrame_t *tempFrame = firstScriptFrame;
    while (tempFrame != NULL) {
        unmarkScriptFrame(tempFrame);
        tempFrame = tempFrame->next;
    }
    // Unmark all locked values.
    tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        if (tempHeapValue->lockDepth > 0) {
            unmarkScriptHeapValue(tempHeapValue);
        }
        tempHeapValue = tempHeapValue->next;
    }
    // Delete all values which are still marked.
    tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        scriptHeapValue_t *tempNextHeapValue = tempHeapValue->next;
        if (tempHeapValue->isMarked) {
            deleteScriptHeapValue(tempHeapValue, false);
        }
        tempHeapValue = tempNextHeapValue;
    }
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

// Completely unlocks all script values.
void displayScriptError() {
    scriptHeapValue_t *tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        tempHeapValue->lockDepth = 0;
        tempHeapValue = tempHeapValue->next;
    }
    int8_t tempText[1200];
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

keyMapping_t *findKeyMapping(int32_t oldKey, int32_t mode) {
    vector_t *tempKeyMapingList = keyMappingHashTable + oldKey % KEY_HASH_TABLE_SIZE;
    int64_t index = 0;
    while (index < tempKeyMapingList->length) {
        keyMapping_t *tempKeyMapping = findVectorElement(tempKeyMapingList, index);
        if (mode == tempKeyMapping->mode && oldKey == tempKeyMapping->oldKey) {
            return tempKeyMapping;
        }
        index += 1;
    }
    return NULL;
}

// insertIndex denotes where the command should be inserted
// if the command is not found.
commandBinding_t *findCommandBinding(int32_t *insertIndex, int8_t *commandName) {
    commandBinding_t *output = NULL;
    int32_t tempMinimumIndex = 0; // Inclusive.
    int32_t tempMaximumIndex = commandBindingList.length; // Exclusive.
    while (tempMinimumIndex < tempMaximumIndex) {
        int32_t tempMiddleIndex = tempMinimumIndex + (tempMaximumIndex - tempMaximumIndex) / 2;
        commandBinding_t *tempCommandBinding = findVectorElement(&commandBindingList, tempMiddleIndex);
        int32_t tempResult = strcmp((char *)(tempCommandBinding->commandName), (char *)commandName);
        if (tempResult < 0) {
            tempMaximumIndex = tempMiddleIndex;
        } else if (tempResult > 0) {
            tempMinimumIndex = tempMiddleIndex + 1;
        } else {
            output = tempCommandBinding;
            break;
        }
    }
    if (insertIndex != NULL) {
        if (output == NULL) {
            *insertIndex = tempMinimumIndex;
        } else {
            *insertIndex = -1;
        }
    }
    return output;
}

void storeValueInExpressionResultDestination(expressionResult_t *expressionResult, scriptValue_t value) {
    int8_t tempType = expressionResult->destinationType;
    if (tempType == DESTINATION_TYPE_NONE) {
        reportScriptError((int8_t *)"Invalid destination.", NULL);
        return;
    }
    if (tempType == DESTINATION_TYPE_VARIABLE) {
        swapScriptValueReference((scriptValue_t *)(expressionResult->destination), &value);
        return;
    }
    int64_t index = expressionResult->destinationIndex;
    vector_t *tempVector = (vector_t *)(expressionResult->destination);
    if (index < 0 || index >= tempVector->length) {
        reportScriptError((int8_t *)"Index out of range.", NULL);
        return;
    }
    if (tempType == DESTINATION_TYPE_LIST) {
        scriptValue_t *destination = findVectorElement(tempVector, index);
        swapScriptValueReference(destination, &value);
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

// Output value will be locked.
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
    unlockScriptValue(&tempValue);
    return output;
}

// Output value will be locked.
scriptValue_t evaluateBinaryExpression(scriptBinaryExpression_t *expression) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult_t tempResult1 = evaluateExpression(expression->operand1);
    if (scriptHasError) {
        return output;
    }
    expressionResult_t tempResult2 = evaluateExpression(expression->operand2);
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
                lockScriptValue(&output);
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
            lockScriptValue(&output);
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
                lockScriptValue(&output);
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
    unlockScriptValue(&(tempResult1.value));
    unlockScriptValue(&(tempResult2.value));
    return output;
}

scriptValue_t invokeFunction(scriptBaseFunction_t *function, scriptValue_t *argumentList, int32_t argumentAmount);

// Output value will be locked.
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
            vector_t tempText;
            copyVector(&tempText, &tempExpression->text);
            output.value = convertCharacterVectorToStringValue(tempText);
            lockScriptValue(&(output.value));
            break;
        }
        case SCRIPT_EXPRESSION_TYPE_LIST:
        {
            scriptListExpression_t *tempExpression = (scriptListExpression_t *)expression;
            vector_t tempValueList;
            createEmptyVector(&tempValueList, sizeof(scriptValue_t));
            int32_t index;
            index = 0;
            while (index < tempExpression->expressionList.length) {
                scriptBaseExpression_t *tempElementExpression;
                getVectorElement(&tempElementExpression, &(tempExpression->expressionList), index);
                expressionResult_t tempResult = evaluateExpression(tempElementExpression);
                if (scriptHasError) {
                    return output;
                }
                pushVectorElement(&tempValueList, &(tempResult.value));
                addScriptValueReference(&(tempResult.value));
                index += 1;
            }
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
            tempHeapValue->data = tempValueList;
            output.value.type = SCRIPT_VALUE_TYPE_LIST;
            *(scriptHeapValue_t **)(output.value.data) = tempHeapValue;
            lockScriptValue(&(output.value));
            index = 0;
            while (index < tempValueList.length) {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, &tempValueList, index);
                unlockScriptValue(&tempValue);
                index += 1;
            }
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
            scriptFrame_t *tempFrame;
            if (tempExpression->variable.isGlobal) {
                tempFrame = globalFrame;
            } else {
                tempFrame = localFrame;
            }
            scriptValue_t *tempValuePointer = tempFrame->valueList + tempExpression->variable.scopeIndex;
            output.value = *tempValuePointer;
            output.destinationType = DESTINATION_TYPE_VARIABLE;
            output.destination = tempValuePointer;
            lockScriptValue(&(output.value));
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
            expressionResult_t tempResult2 = evaluateExpression(tempExpression->index);
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
            lockScriptValue(&(output.value));
            unlockScriptValue(&(tempResult1.value));
            unlockScriptValue(&(tempResult2.value));
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
        default:
        {
            break;
        }
    }
    return output;
}

int8_t evaluateStatementList(scriptValue_t *returnValue, vector_t *statementList);
script_t *importScript(int8_t *path);

// returnValue will be locked.
int8_t evaluateStatement(scriptValue_t *returnValue, scriptBaseStatement_t *statement) {
    garbageCollectionDelay -= 1;
    if (garbageCollectionDelay <= 0) {
        garbageCollectScriptHeapValues();
        garbageCollectionDelay = 150000;
    }
    scriptBodyLine_t *tempLine = &(statement->scriptBodyLine);
    switch (statement->type) {
        case SCRIPT_STATEMENT_TYPE_EXPRESSION:
        {
            scriptExpressionStatement_t *tempStatement = (scriptExpressionStatement_t *)statement;
            scriptBaseExpression_t *tempExpression = tempStatement->expression;
            expressionResult_t tempResult = evaluateExpression(tempExpression);
            unlockScriptValue(&(tempResult.value));
            break;
        }
        case SCRIPT_STATEMENT_TYPE_IF:
        {
            scriptIfStatement_t *tempStatement = (scriptIfStatement_t *)statement;
            vector_t *tempClauseList = &(tempStatement->clauseList);
            int32_t index = 0;
            while (index < tempClauseList->length) {
                scriptIfClause_t *tempClause;
                getVectorElement(&tempClause, tempClauseList, index);
                scriptBaseExpression_t *tempCondition = tempClause->condition;
                int8_t tempShouldEvaluateStatements;
                if (tempCondition == NULL) {
                    tempShouldEvaluateStatements = true;
                } else {
                    expressionResult_t tempResult = evaluateExpression(tempCondition);
                    if (scriptHasError) {
                        scriptErrorLine = tempClause->scriptBodyLine;
                        scriptErrorHasLine = true;
                        return STATEMENT_JUMP_ERROR;
                    }
                    if (tempResult.value.type != SCRIPT_VALUE_TYPE_NUMBER) {
                        reportScriptError((int8_t *)"Invalid condition type.", tempLine);
                        return STATEMENT_JUMP_ERROR;
                    }
                    double tempCondition = *(double *)(tempResult.value.data);
                    tempShouldEvaluateStatements = (tempCondition != 0.0);
                }
                if (tempShouldEvaluateStatements) {
                    int8_t tempResult2 = evaluateStatementList(returnValue, &(tempClause->statementList));
                    if (tempResult2 != STATEMENT_JUMP_NONE) {
                        return tempResult2;
                    }
                    break;
                }
                index += 1;
            }
            break;
        }
        case SCRIPT_STATEMENT_TYPE_WHILE:
        {
            scriptWhileStatement_t *tempStatement = (scriptWhileStatement_t *)statement;
            scriptBaseExpression_t *tempCondition = tempStatement->condition;
            while (true) {
                expressionResult_t tempResult = evaluateExpression(tempCondition);
                if (scriptHasError) {
                    break;
                }
                if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                    double tempCondition = *(double *)(tempResult.value.data);
                    if (tempCondition == 0.0) {
                        break;
                    }
                } else {
                    reportScriptError((int8_t *)"Invalid condition type.", tempLine);
                    return STATEMENT_JUMP_ERROR;
                }
                int8_t tempResult2 = evaluateStatementList(returnValue, &(tempStatement->statementList));
                if (tempResult2 == STATEMENT_JUMP_BREAK) {
                    break;
                } else if (tempResult2 != STATEMENT_JUMP_CONTINUE
                        && tempResult2 != STATEMENT_JUMP_NONE) {
                    return tempResult2;
                }
            }
            break;
        }
        case SCRIPT_STATEMENT_TYPE_BREAK:
        {
            return STATEMENT_JUMP_BREAK;
        }
        case SCRIPT_STATEMENT_TYPE_CONTINUE:
        {
            return STATEMENT_JUMP_CONTINUE;
        }
        case SCRIPT_STATEMENT_TYPE_RETURN:
        {
            scriptReturnStatement_t *tempStatement = (scriptReturnStatement_t *)statement;
            scriptBaseExpression_t *tempExpression = tempStatement->expression;
            if (tempExpression != NULL) {
                expressionResult_t tempResult = evaluateExpression(tempExpression);
                if (scriptHasError) {
                    break;
                }
                *returnValue = tempResult.value;
            }
            return STATEMENT_JUMP_RETURN;
        }
        case SCRIPT_STATEMENT_TYPE_IMPORT:
        {
            scriptImportStatement_t *tempStatement = (scriptImportStatement_t *)statement;
            scriptBaseExpression_t *tempExpression = tempStatement->path;
            expressionResult_t tempResult = evaluateExpression(tempExpression);
            if (scriptHasError) {
                break;
            }
            if (tempResult.value.type != SCRIPT_VALUE_TYPE_STRING) {
                reportScriptError((int8_t *)"Invalid path type.", tempLine);
                break;
            }
            int8_t *tempPath = getScriptValueText(tempResult.value);
            script_t *tempScript = importScript(tempPath);
            if (scriptHasError) {
                break;
            }
            scriptScope_t *tempGlobalScope = &(tempScript->entryPointFunction->scope);
            int32_t index = 0;
            while (index < tempStatement->variableList.length) {
                scriptVariable_t tempVariable;
                getVectorElement(&tempVariable, &(tempStatement->variableList), index);
                int32_t tempScopeIndex = scriptScopeFindVariable(
                    tempGlobalScope,
                    tempVariable.name
                );
                if (tempScopeIndex < 0) {
                    reportScriptError((int8_t *)"Missing import variable.", tempLine);
                    return STATEMENT_JUMP_ERROR;
                }
                scriptValue_t tempValue = tempScript->globalFrame.valueList[tempScopeIndex];
                swapScriptValueReference(localFrame->valueList + tempVariable.scopeIndex, &tempValue);
                index += 1;
            }
            unlockScriptValue(&(tempResult.value));
            break;
        }
        default:
        {
            break;
        }
    }
    if (scriptHasError) {
        if (!scriptErrorHasLine) {
            scriptErrorLine = statement->scriptBodyLine;
            scriptErrorHasLine = true;
        }
        return STATEMENT_JUMP_ERROR;
    }
    return STATEMENT_JUMP_NONE;
}

// returnValue will be locked.
int8_t evaluateStatementList(scriptValue_t *returnValue, vector_t *statementList) {
    int32_t index = 0;
    while (index < statementList->length) {
        scriptBaseStatement_t *tempStatement;
        getVectorElement(&tempStatement, statementList, index);
        int8_t tempResult = evaluateStatement(returnValue, tempStatement);
        if (tempResult != STATEMENT_JUMP_NONE) {
            return tempResult;
        }
        index += 1;
    }
    return STATEMENT_JUMP_NONE;
}

void reportBadArgType() {
    reportScriptError((int8_t *)"Bad argument type.", NULL);
}

int32_t getArgFileHandle(scriptValue_t value) {
    if (value.type != SCRIPT_VALUE_TYPE_NUMBER) {
        reportBadArgType();
        return -1;
    }
    int32_t fileHandle = (int32_t)*(double *)(value.data);
    if (fileHandle < 0) {
        reportScriptError((int8_t *)"File handle cannot be negative.", NULL);
    }
    return fileHandle;
}

// All argument values must be locked.
// Output value will be locked.
scriptValue_t invokeFunction(scriptBaseFunction_t *function, scriptValue_t *argumentList, int32_t argumentAmount) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    if (argumentAmount != function->argumentAmount) {
        reportScriptError((int8_t *)"Incorrect number of arguments.", NULL);
        return output;
    }
    if (function->type == SCRIPT_FUNCTION_TYPE_CUSTOM) {
        scriptCustomFunction_t *tempFunction = (scriptCustomFunction_t *)function;
        scriptFrame_t *lastGlobalFrame = globalFrame;
        scriptFrame_t *lastLocalFrame = localFrame;
        globalFrame = &(tempFunction->script->globalFrame);
        if (tempFunction->isEntryPoint) {
            localFrame = globalFrame;
            evaluateStatementList(&output, &tempFunction->statementList);
        } else {
            int32_t index;
            int32_t tempLength = tempFunction->scope.variableNameList.length;
            scriptValue_t frameValueList[tempLength];
            index = 0;
            while (index < tempLength) {
                if (index < function->argumentAmount) {
                    scriptValue_t tempValue = argumentList[index];
                    frameValueList[index] = tempValue;
                    addScriptValueReference(&tempValue);
                } else {
                    (frameValueList + index)->type = SCRIPT_VALUE_TYPE_MISSING;
                }
                index += 1;
            }
            scriptFrame_t tempFrame;
            tempFrame.valueList = frameValueList;
            tempFrame.valueAmount = tempLength;
            addScriptFrame(&tempFrame);
            localFrame = &tempFrame;
            evaluateStatementList(&output, &tempFunction->statementList);
            index = 0;
            while (index < tempLength) {
                removeScriptValueReference(frameValueList + index);
                index += 1;
            }
            removeScriptFrame(&tempFrame);
        }
        globalFrame = lastGlobalFrame;
        localFrame = lastLocalFrame;
    }
    if (function->type == SCRIPT_FUNCTION_TYPE_BUILT_IN) {
        scriptBuiltInFunction_t *tempFunction = (scriptBuiltInFunction_t *)function;
        switch (tempFunction->number) {
            case SCRIPT_FUNCTION_IS_NUM:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(argumentList[0].type == SCRIPT_VALUE_TYPE_NUMBER);
                break;
            }
            case SCRIPT_FUNCTION_IS_STR:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(argumentList[0].type == SCRIPT_VALUE_TYPE_STRING);
                break;
            }
            case SCRIPT_FUNCTION_IS_LIST:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(argumentList[0].type == SCRIPT_VALUE_TYPE_LIST);
                break;
            }
            case SCRIPT_FUNCTION_IS_FUNC:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(argumentList[0].type == SCRIPT_VALUE_TYPE_FUNCTION);
                break;
            }
            case SCRIPT_FUNCTION_COPY:
            {
                output = copyScriptValue(argumentList + 0);
                lockScriptValue(&output);
                break;
            }
            case SCRIPT_FUNCTION_STR:
            {
                output = convertScriptValueToString(argumentList[0]);
                lockScriptValue(&output);
                break;
            }
            case SCRIPT_FUNCTION_NUM:
            {
                output = convertScriptValueToNumber(argumentList[0]);
                break;
            }
            case SCRIPT_FUNCTION_FLOOR:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = floor(*(double *)(tempValue.data));
                break;
            }
            case SCRIPT_FUNCTION_RAND:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = ((double)rand()) / ((double)RAND_MAX);
                break;
            }
            case SCRIPT_FUNCTION_POW:
            {
                scriptValue_t tempValue1 = argumentList[0];
                scriptValue_t tempValue2 = argumentList[1];
                if (tempValue1.type != SCRIPT_VALUE_TYPE_NUMBER || tempValue2.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                double tempResult = pow(tempNumber1, tempNumber2);
                if (isnan(tempResult)) {
                    reportScriptError((int8_t *)"Exponentiation error.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = tempResult;
                break;
            }
            case SCRIPT_FUNCTION_LOG:
            {
                scriptValue_t tempValue1 = argumentList[0];
                scriptValue_t tempValue2 = argumentList[1];
                if (tempValue1.type != SCRIPT_VALUE_TYPE_NUMBER || tempValue2.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                double tempNumber1 = *(double *)(tempValue1.data);
                double tempNumber2 = *(double *)(tempValue2.data);
                double tempResult = log(tempNumber1) / log(tempNumber2);
                if (isnan(tempResult)) {
                    reportScriptError((int8_t *)"Logarithm error.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = tempResult;
                break;
            }
            case SCRIPT_FUNCTION_LEN:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempValue.data);
                    vector_t *tempText = &(tempHeapValue->data);
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)(output.data) = (double)(tempText->length - 1);
                } else if (tempValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempValue.data);
                    vector_t *tempList = &(tempHeapValue->data);
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)(output.data) = (double)(tempList->length);
                } else {
                    reportBadArgType();
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_INS:
            {
                scriptValue_t tempSequenceValue = argumentList[0];
                scriptValue_t tempIndexValue = argumentList[1];
                scriptValue_t tempItemValue = argumentList[2];
                if (tempIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t index = (int64_t)*(double *)(tempIndexValue.data);
                if (index < 0) {
                    reportScriptError((int8_t *)"Index out of range.", NULL);
                    return output;
                }
                if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    if (tempItemValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                        reportBadArgType();
                        return output;
                    }
                    int8_t tempCharacter = (int8_t)*(double *)(tempItemValue.data);
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempText = &(tempHeapValue->data);
                    if (index >= tempText->length) {
                        reportScriptError((int8_t *)"Index out of range.", NULL);
                        return output;
                    }
                    insertVectorElement(tempText, index, &tempCharacter);
                } else if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempList = &(tempHeapValue->data);
                    if (index > tempList->length) {
                        reportScriptError((int8_t *)"Index out of range.", NULL);
                        return output;
                    }
                    insertVectorElement(tempList, index, &tempItemValue);
                    addScriptValueReference(&tempItemValue);
                } else {
                    reportBadArgType();
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_PUSH:
            {
                scriptValue_t tempSequenceValue = argumentList[0];
                scriptValue_t tempItemValue = argumentList[1];
                if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    if (tempItemValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                        reportBadArgType();
                        return output;
                    }
                    int8_t tempCharacter = (int8_t)*(double *)(tempItemValue.data);
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempText = &(tempHeapValue->data);
                    insertVectorElement(tempText, tempText->length - 1, &tempCharacter);
                } else if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempList = &(tempHeapValue->data);
                    pushVectorElement(tempList, &tempItemValue);
                    addScriptValueReference(&tempItemValue);
                } else {
                    reportBadArgType();
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_REM:
            {
                scriptValue_t tempSequenceValue = argumentList[0];
                scriptValue_t tempIndexValue = argumentList[1];
                if (tempIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t index = (int64_t)*(double *)(tempIndexValue.data);
                if (index < 0) {
                    reportScriptError((int8_t *)"Index out of range.", NULL);
                    return output;
                }
                if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempText = &(tempHeapValue->data);
                    if (index >= tempText->length - 1) {
                        reportScriptError((int8_t *)"Index out of range.", NULL);
                        return output;
                    }
                    removeVectorElement(tempText, index);
                } else if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempSequenceValue.data);
                    vector_t *tempList = &(tempHeapValue->data);
                    if (index >= tempList->length) {
                        reportScriptError((int8_t *)"Index out of range.", NULL);
                        return output;
                    }
                    scriptValue_t *tempItem = findVectorElement(tempList, index);
                    removeScriptValueReference(tempItem);
                    removeVectorElement(tempList, index);
                } else {
                    reportBadArgType();
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_GET_TIMESTAMP:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = getTimestamp();
                break;
            }
            case SCRIPT_FUNCTION_FILE_EXISTS:
            {
                scriptValue_t pathValue = argumentList[0];
                if (pathValue.type != SCRIPT_VALUE_TYPE_STRING) {
                    reportBadArgType();
                    return output;
                }
                int8_t *path = getScriptValueText(pathValue);
                int8_t fileExists = (access((char *)path, F_OK) != -1);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fileExists;
                break;
            }
            case SCRIPT_FUNCTION_OPEN_FILE:
            {
                scriptValue_t pathValue = argumentList[0];
                if (pathValue.type != SCRIPT_VALUE_TYPE_STRING) {
                    reportBadArgType();
                    return output;
                }
                int8_t *path = getScriptValueText(pathValue);
                int32_t fileHandle = open((char *)path, O_RDWR | O_CREAT);
                if (fileHandle < 0) {
                    reportScriptError((int8_t *)"Could not open file.", NULL);
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fileHandle;
                break;
            }
            case SCRIPT_FUNCTION_GET_FILE_SIZE:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                int64_t fileOffset = lseek(fileHandle, 0L, SEEK_CUR);
                int64_t fileSize = lseek(fileHandle, 0L, SEEK_END);
                lseek(fileHandle, fileOffset, SEEK_SET);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fileSize;
                break;
            }
            case SCRIPT_FUNCTION_SET_FILE_SIZE:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                scriptValue_t sizeValue = argumentList[1];
                if (sizeValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t size = (int64_t)*(double *)(sizeValue.data);
                if (size < 0) {
                    reportScriptError((int8_t *)"File size cannot be negative.", NULL);
                    return output;
                }
                int64_t fileOffset = lseek(fileHandle, 0L, SEEK_CUR);
                int32_t result = ftruncate(fileHandle, size);
                if (result < 0) {
                    reportScriptError((int8_t *)"Could not set file size.", NULL);
                    return output;
                }
                if (fileOffset > size) {
                    fileOffset = size;
                }
                lseek(fileHandle, fileOffset, SEEK_SET);
                break;
            }
            case SCRIPT_FUNCTION_READ_FILE:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                scriptValue_t amountValue = argumentList[1];
                if (amountValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t amount = (int64_t)*(double *)(amountValue.data);
                if (amount < 0) {
                    reportScriptError((int8_t *)"File read amount cannot be negative.", NULL);
                    return output;
                }
                vector_t textVector;
                createVector(&textVector, 1, amount + 1);
                int64_t count = read(fileHandle, textVector.data, amount);
                if (count < 0) {
                    reportScriptError((int8_t *)"Could not read file.", NULL);
                    return output;
                }
                textVector.data[count] = 0;
                setVectorLength(&textVector, count + 1);
                output = convertCharacterVectorToStringValue(textVector);
                lockScriptValue(&output);
                break;
            }
            case SCRIPT_FUNCTION_WRITE_FILE:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                scriptValue_t textValue = argumentList[1];
                if (textValue.type != SCRIPT_VALUE_TYPE_STRING) {
                    reportBadArgType();
                    return output;
                }
                scriptHeapValue_t *heapValue = *(scriptHeapValue_t **)(textValue.data);
                vector_t *textVector = &(heapValue->data);
                int64_t result = write(fileHandle, textVector->data, textVector->length - 1);
                if (result < 0) {
                    reportScriptError((int8_t *)"Could not write to file.", NULL);
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_GET_FILE_OFFSET:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                int64_t fileOffset = lseek(fileHandle, 0L, SEEK_CUR);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = fileOffset;
                break;
            }
            case SCRIPT_FUNCTION_SET_FILE_OFFSET:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                scriptValue_t offsetValue = argumentList[1];
                if (offsetValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t offset = (int64_t)*(double *)(offsetValue.data);
                if (offset < 0) {
                    reportScriptError((int8_t *)"File offset cannot be negative.", NULL);
                    return output;
                }
                lseek(fileHandle, offset, SEEK_SET);
                break;
            }
            case SCRIPT_FUNCTION_CLOSE_FILE:
            {
                int32_t fileHandle = getArgFileHandle(argumentList[0]);
                if (fileHandle < 0) {
                    return output;
                }
                int32_t result = close(fileHandle);
                if (result < 0) {
                    reportScriptError((int8_t *)"Could not close file.", NULL);
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_DELETE_FILE:
            {
                scriptValue_t pathValue = argumentList[0];
                if (pathValue.type != SCRIPT_VALUE_TYPE_STRING) {
                    reportBadArgType();
                    return output;
                }
                int8_t *path = getScriptValueText(pathValue);
                int32_t result = remove((char *)path);
                if (result < 0) {
                    reportScriptError((int8_t *)"Could not delete file.", NULL);
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_PRESS_KEY:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int32_t tempKey = (int32_t)*(double *)(tempValue.data);
                handleKey(tempKey, false, false, false);
                break;
            }
            case SCRIPT_FUNCTION_PRESS_KEYS:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempValue.data);
                    vector_t *tempText = &(tempHeapValue->data);
                    int32_t index = 0;
                    while (index < tempText->length) {
                        int8_t tempKey;
                        getVectorElement(&tempKey, tempText, index);
                        handleKey(tempKey, false, false, false);
                        index += 1;
                    }
                } else if (tempValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempValue.data);
                    vector_t *tempList = &(tempHeapValue->data);
                    int32_t index = 0;
                    while (index < tempList->length) {
                        scriptValue_t tempValue;
                        getVectorElement(&tempValue, tempList, index);
                        if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                            reportScriptError((int8_t *)"Bad key type.", NULL);
                            return output;
                        }
                        int32_t tempKey = (int32_t)*(double *)(tempValue.data);
                        handleKey(tempKey, false, false, false);
                        index += 1;
                    }
                } else {
                    reportBadArgType();
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_GET_MODE:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(activityMode);
                break;
            }
            case SCRIPT_FUNCTION_SET_MODE:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int32_t tempMode = (int32_t)*(double *)(tempValue.data);
                if (tempMode < 1 || tempMode > 9) {
                    reportScriptError((int8_t *)"Invalid mode.", NULL);
                    return output;
                }
                if (tempMode != COMMAND_MODE && activityMode != COMMAND_MODE) {
                    setActivityMode(COMMAND_MODE);
                }
                if (tempMode == HIGHLIGHT_CHARACTER_MODE || tempMode == HIGHLIGHT_STATIC_MODE) {
                    highlightTextPos = cursorTextPos;
                }
                setActivityMode(tempMode);
                break;
            }
            case SCRIPT_FUNCTION_GET_SELECTION_CONTENTS:
            {
                int8_t *tempText = allocateStringFromSelection();
                output = convertTextToStringValue(tempText);
                free(tempText);
                lockScriptValue(&output);
                break;
            }
            case SCRIPT_FUNCTION_GET_LINE_COUNT:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                textLine_t *tempLine = getRightmostTextLine(rootTextLine);
                *(double *)(output.data) = (double)getTextLineNumber(tempLine);
                break;
            }
            case SCRIPT_FUNCTION_GET_LINE_CONTENTS:
            {
                scriptValue_t tempValue = argumentList[0];
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t tempLineIndex = (int64_t)*(double *)(tempValue.data);
                textLine_t *tempLine = getTextLineByNumber(tempLineIndex + 1);
                if (tempLine == NULL) {
                    reportScriptError((int8_t *)"Bad line index.", NULL);
                    return output;
                }
                textAllocation_t *tempAllocation = &(tempLine->textAllocation);
                int8_t tempBuffer[tempAllocation->length + 1];
                copyData(tempBuffer, tempAllocation->text, tempAllocation->length);
                tempBuffer[tempAllocation->length] = 0;
                output = convertTextToStringValue(tempBuffer);
                lockScriptValue(&output);
                break;
            }
            case SCRIPT_FUNCTION_GET_CURSOR_CHAR_INDEX:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)getTextPosIndex(&cursorTextPos);
                break;
            }
            case SCRIPT_FUNCTION_GET_CURSOR_LINE_INDEX:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(getTextLineNumber(cursorTextPos.line) - 1);
                break;
            }
            case SCRIPT_FUNCTION_SET_CURSOR_POS:
            {
                scriptValue_t tempCharIndexValue = argumentList[0];
                scriptValue_t tempLineIndexValue = argumentList[1];
                if (tempCharIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER || tempLineIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int64_t tempCharIndex = (int64_t)*(double *)(tempCharIndexValue.data);
                int64_t tempLineIndex = (int64_t)*(double *)(tempLineIndexValue.data);
                textLine_t *tempLine = getTextLineByNumber(tempLineIndex + 1);
                if (tempLine == NULL) {
                    reportScriptError((int8_t *)"Bad line index.", NULL);
                    return output;
                }
                textAllocation_t *tempAllocation = &(tempLine->textAllocation);
                if (tempCharIndex < 0 || tempCharIndex > tempAllocation->length) {
                    reportScriptError((int8_t *)"Bad char index.", NULL);
                    return output;
                }
                textPos_t tempTextPos;
                tempTextPos.line = tempLine;
                setTextPosIndex(&tempTextPos, tempCharIndex);
                moveCursor(&tempTextPos);
                break;
            }
            case SCRIPT_FUNCTION_RUN_COMMAND:
            {
                scriptValue_t tempCommandNameValue = argumentList[0];
                scriptValue_t tempArgumentsValue = argumentList[1];
                if (tempCommandNameValue.type != SCRIPT_VALUE_TYPE_STRING || tempArgumentsValue.type != SCRIPT_VALUE_TYPE_LIST) {
                    reportBadArgType();
                    return output;
                }
                scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(tempCommandNameValue.data);
                scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)(tempArgumentsValue.data);
                vector_t *tempName = &(tempHeapValue1->data);
                vector_t *tempList = &(tempHeapValue2->data);
                int8_t *tempTermList[tempList->length + 1];
                tempTermList[0] = tempName->data;
                int64_t index = 0;
                while (index < tempList->length) {
                    scriptValue_t tempArgumentValue;
                    getVectorElement(&tempArgumentValue, tempList, index);
                    scriptValue_t tempStringValue = convertScriptValueToString(tempArgumentValue);
                    scriptHeapValue_t *tempHeapValue3 = *(scriptHeapValue_t **)(tempStringValue.data);
                    vector_t *tempText = &(tempHeapValue3->data);
                    tempTermList[index + 1] = tempText->data;
                    index += 1;
                }
                executeTextCommandByTermList(&output, tempTermList, tempList->length + 1);
                if (scriptHasError) {
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_NOTIFY_USER:
            {
                scriptValue_t tempStringValue = convertScriptValueToString(argumentList[0]);
                int8_t *tempText = getScriptValueText(tempStringValue);
                notifyUser(tempText);
                break;
            }
            case SCRIPT_FUNCTION_PROMPT_KEY:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)(output.data) = (double)(getch());
                break;
            }
            case SCRIPT_FUNCTION_PROMPT_CHAR:
            {
                notifyUser((int8_t *)"Type a character.");
                int32_t tempKey = getch();
                if (tempKey < 32 || tempKey > 126) {
                    output.type = SCRIPT_VALUE_TYPE_NULL;
                } else {
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)(output.data) = (double)(tempKey);
                }
                break;
            }
            case SCRIPT_FUNCTION_BIND_KEY:
            {
                scriptValue_t tempKeyValue = argumentList[0];
                scriptValue_t tempCallbackValue = argumentList[1];
                if (tempKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER || tempCallbackValue.type != SCRIPT_VALUE_TYPE_FUNCTION) {
                    reportBadArgType();
                    return output;
                }
                int32_t tempKey = (int32_t)*(double *)(tempKeyValue.data);
                scriptBaseFunction_t *tempCallback = *(scriptBaseFunction_t **)(tempCallbackValue.data);
                keyBinding_t tempKeyBinding;
                tempKeyBinding.key = tempKey;
                tempKeyBinding.callback = tempCallback;
                vector_t *tempKeyBindingList = keyBindingHashTable + tempKey % KEY_HASH_TABLE_SIZE;
                pushVectorElement(tempKeyBindingList, &tempKeyBinding);
                break;
            }
            case SCRIPT_FUNCTION_MAP_KEY:
            {
                scriptValue_t tempOldKeyValue = argumentList[0];
                scriptValue_t tempNewKeyValue = argumentList[1];
                scriptValue_t tempModeValue = argumentList[2];
                if (tempOldKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER || tempNewKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER
                        || tempModeValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportBadArgType();
                    return output;
                }
                int32_t tempOldKey = (int32_t)*(double *)(tempOldKeyValue.data);
                int32_t tempNewKey = (int32_t)*(double *)(tempNewKeyValue.data);
                int32_t tempMode = (int32_t)*(double *)(tempModeValue.data);
                keyMapping_t *tempOldKeyMapping = findKeyMapping(tempOldKey, tempMode);
                if (tempOldKeyMapping == NULL) {
                    vector_t *tempKeyMapingList = keyMappingHashTable + tempOldKey % KEY_HASH_TABLE_SIZE;
                    keyMapping_t tempNewKeyMapping;
                    tempNewKeyMapping.oldKey = tempOldKey;
                    tempNewKeyMapping.newKey = tempNewKey;
                    tempNewKeyMapping.mode = tempMode;
                    pushVectorElement(tempKeyMapingList, &tempNewKeyMapping);
                } else {
                    tempOldKeyMapping->newKey = tempNewKey;
                }
                break;
            }
            case SCRIPT_FUNCTION_BIND_COMMAND:
            {
                scriptValue_t tempCommandNameValue = argumentList[0];
                scriptValue_t tempCallbackValue = argumentList[1];
                if (tempCommandNameValue.type != SCRIPT_VALUE_TYPE_STRING || tempCallbackValue.type != SCRIPT_VALUE_TYPE_FUNCTION) {
                    reportBadArgType();
                    return output;
                }
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempCommandNameValue.data);
                vector_t *tempText = &(tempHeapValue->data);
                scriptBaseFunction_t *tempCallback = *(scriptBaseFunction_t **)(tempCallbackValue.data);
                int32_t tempInsertIndex;
                commandBinding_t *tempOldCommandBinding = findCommandBinding(
                    &tempInsertIndex,
                    tempText->data
                );
                if (tempOldCommandBinding == NULL) {
                    commandBinding_t tempNewCommandBinding;
                    tempNewCommandBinding.commandName = malloc(tempText->length);
                    strcpy((char *)(tempNewCommandBinding.commandName), (char *)(tempText->data));
                    tempNewCommandBinding.callback = tempCallback;
                    insertVectorElement(&commandBindingList, tempInsertIndex, &tempNewCommandBinding);
                } else {
                    tempOldCommandBinding->callback = tempCallback;
                }
                break;
            }
            case SCRIPT_FUNCTION_TEST_LOG:
            {
                scriptValue_t tempStringValue = convertScriptValueToString(argumentList[0]);
                int8_t *tempText = getScriptValueText(tempStringValue);
                addScriptTestLogMessage(tempText);
                break;
            }
            default: {
                break;
            }
        }
    }
    int32_t index = 0;
    while (index < argumentAmount) {
        unlockScriptValue(argumentList + index);
        index += 1;
    }
    return output;
}

void evaluateScript(script_t *script) {
    scriptScope_t *tempGlobalScope = &(script->entryPointFunction->scope);
    int32_t tempLength = tempGlobalScope->variableNameList.length;
    scriptValue_t *frameValueList = malloc(sizeof(scriptValue_t) * tempLength);
    int32_t index = 0;
    while (index < tempLength) {
        (frameValueList + index)->type = SCRIPT_VALUE_TYPE_MISSING;
        index += 1;
    }
    script->globalFrame.valueList = frameValueList;
    script->globalFrame.valueAmount = tempLength;
    addScriptFrame(&(script->globalFrame));
    scriptValue_t tempResult = invokeFunction((scriptBaseFunction_t *)(script->entryPointFunction), NULL, 0);
    unlockScriptValue(&tempResult);
}

script_t *parseAndEvaluateScript(scriptBody_t *scriptBody) {
    script_t *tempScript;
    int8_t tempResult = parseScriptBody(&tempScript, scriptBody);
    if (!tempResult) {
        return NULL;
    }
    pushVectorElement(&scriptList, &tempScript);
    evaluateScript(tempScript);
    return tempScript;
}

script_t *importScriptHelper(int8_t *path) {
    int32_t index = 0;
    while (index < scriptList.length) {
        script_t *tempScript;
        getVectorElement(&tempScript, &scriptList, index);
        int8_t *tempPath = tempScript->scriptBody->path;
        if (strcmp((char *)tempPath, (char *)path) == 0) {
            return NULL;
        }
        index += 1;
    }
    scriptBody_t *tempScriptBody;
    int8_t tempResult = loadScriptBody(&tempScriptBody, path);
    if (!tempResult) {
        reportScriptError((int8_t *)"Import file missing.", NULL);
        return NULL;
    }
    return parseAndEvaluateScript(tempScriptBody);
}

script_t *importScript(int8_t *path) {
    path = mallocRealpath(path);
    script_t *output = importScriptHelper(path);
    free(path);
    return output;
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
    resetScriptError();
    int8_t output = false;
    vector_t *tempKeyBindingList = keyBindingHashTable + key % KEY_HASH_TABLE_SIZE;
    int64_t index = 0;
    while (index < tempKeyBindingList->length) {
        keyBinding_t tempKeyBinding;
        getVectorElement(&tempKeyBinding, tempKeyBindingList, index);
        if (tempKeyBinding.key == key) {
            scriptValue_t tempResult = invokeFunction(tempKeyBinding.callback, NULL, 0);
            if (scriptHasError) {
                output = true;
                break;
            }
            if (tempResult.type != SCRIPT_VALUE_TYPE_NUMBER) {
                reportScriptError((int8_t *)"Key binding must return boolean.", NULL);
                output = true;
                break;
            }
            int8_t tempShouldOverride = (*(double *)(tempResult.data) != 0.0);
            if (tempShouldOverride) {
                output = true;
                break;
            }
        }
        index += 1;
    }
    if (scriptHasError) {
        displayScriptError();
    }
    return output;
}

int32_t invokeKeyMapping(int32_t key) {
    keyMapping_t *tempKeyMapping = findKeyMapping(key, activityMode);
    if (tempKeyMapping != NULL) {
        return tempKeyMapping->newKey;
    }
    return key;
}

// destination will be locked (if the pointer is not null).
int8_t invokeCommandBinding(scriptValue_t *destination, int8_t **termList, int32_t termListLength) {
    commandBinding_t *tempCommandBinding = findCommandBinding(NULL, termList[0]);
    if (tempCommandBinding == NULL) {
        return false;
    }
    if (activityMode == TEXT_COMMAND_MODE) {
        setActivityMode(PREVIOUS_MODE);
    }
    vector_t tempArgumentList;
    createEmptyVector(&tempArgumentList, sizeof(scriptValue_t));
    int64_t index = 1;
    while (index < termListLength) {
        scriptValue_t tempValue = convertTextToStringValue(termList[index]);
        pushVectorElement(&tempArgumentList, &tempValue);
        addScriptValueReference(&tempValue);
        index += 1;
    }
    scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
    tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
    tempHeapValue->data = tempArgumentList;
    scriptValue_t tempListValue;
    tempListValue.type = SCRIPT_VALUE_TYPE_LIST;
    *(scriptHeapValue_t **)(tempListValue.data) = tempHeapValue;
    scriptValue_t tempSingleArgumentList[1];
    tempSingleArgumentList[0] = tempListValue;
    resetScriptError();
    lockScriptValue(&tempListValue);
    scriptValue_t tempResult = invokeFunction(tempCommandBinding->callback, tempSingleArgumentList, 1);
    if (scriptHasError) {
        displayScriptError();
    }
    if (destination == NULL) {
        unlockScriptValue(&tempResult);
    } else {
        *destination = tempResult;
    }
    return true;
}


