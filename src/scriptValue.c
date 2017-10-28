
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "display.h"

scriptOperator_t scriptOperatorSet[] = {
    {(int8_t *)"=", SCRIPT_OPERATOR_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"+", SCRIPT_OPERATOR_ADD, SCRIPT_OPERATOR_TYPE_BINARY, 4},
    {(int8_t *)"+=", SCRIPT_OPERATOR_ADD_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY,14 },
    {(int8_t *)"-", SCRIPT_OPERATOR_SUBTRACT, SCRIPT_OPERATOR_TYPE_BINARY, 4},
    {(int8_t *)"-=", SCRIPT_OPERATOR_SUBTRACT_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"-", SCRIPT_OPERATOR_NEGATE, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX, 1},
    {(int8_t *)"*", SCRIPT_OPERATOR_MULTIPLY, SCRIPT_OPERATOR_TYPE_BINARY, 3},
    {(int8_t *)"*=", SCRIPT_OPERATOR_MULTIPLY_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"/", SCRIPT_OPERATOR_DIVIDE, SCRIPT_OPERATOR_TYPE_BINARY, 3},
    {(int8_t *)"/=", SCRIPT_OPERATOR_DIVIDE_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"%", SCRIPT_OPERATOR_MODULUS, SCRIPT_OPERATOR_TYPE_BINARY, 3},
    {(int8_t *)"%=", SCRIPT_OPERATOR_MODULUS_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"&&", SCRIPT_OPERATOR_BOOLEAN_AND, SCRIPT_OPERATOR_TYPE_BINARY, 11},
    {(int8_t *)"&&=", SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"||", SCRIPT_OPERATOR_BOOLEAN_OR, SCRIPT_OPERATOR_TYPE_BINARY, 13},
    {(int8_t *)"||=", SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"^^", SCRIPT_OPERATOR_BOOLEAN_XOR, SCRIPT_OPERATOR_TYPE_BINARY, 12},
    {(int8_t *)"^^=", SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"!", SCRIPT_OPERATOR_BOOLEAN_NOT, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX, 1},
    {(int8_t *)"&", SCRIPT_OPERATOR_BITWISE_AND, SCRIPT_OPERATOR_TYPE_BINARY, 8},
    {(int8_t *)"&=", SCRIPT_OPERATOR_BITWISE_AND_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"|", SCRIPT_OPERATOR_BITWISE_OR, SCRIPT_OPERATOR_TYPE_BINARY, 10},
    {(int8_t *)"|=", SCRIPT_OPERATOR_BITWISE_OR_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"^", SCRIPT_OPERATOR_BITWISE_XOR, SCRIPT_OPERATOR_TYPE_BINARY, 9},
    {(int8_t *)"^=", SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)"~", SCRIPT_OPERATOR_BITWISE_NOT, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX, 1},
    {(int8_t *)"<<", SCRIPT_OPERATOR_BITSHIFT_LEFT, SCRIPT_OPERATOR_TYPE_BINARY, 5},
    {(int8_t *)"<<=", SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)">>", SCRIPT_OPERATOR_BITSHIFT_RIGHT, SCRIPT_OPERATOR_TYPE_BINARY, 5},
    {(int8_t *)">>=", SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN, SCRIPT_OPERATOR_TYPE_BINARY, 14},
    {(int8_t *)">", SCRIPT_OPERATOR_GREATER, SCRIPT_OPERATOR_TYPE_BINARY, 6},
    {(int8_t *)">=", SCRIPT_OPERATOR_GREATER_OR_EQUAL, SCRIPT_OPERATOR_TYPE_BINARY, 6},
    {(int8_t *)"<", SCRIPT_OPERATOR_LESS, SCRIPT_OPERATOR_TYPE_BINARY, 6},
    {(int8_t *)"<=", SCRIPT_OPERATOR_LESS_OR_EQUAL, SCRIPT_OPERATOR_TYPE_BINARY, 6},
    {(int8_t *)"==", SCRIPT_OPERATOR_EQUAL, SCRIPT_OPERATOR_TYPE_BINARY, 7},
    {(int8_t *)"!=", SCRIPT_OPERATOR_NOT_EQUAL, SCRIPT_OPERATOR_TYPE_BINARY, 7},
    {(int8_t *)"===", SCRIPT_OPERATOR_IDENTICAL, SCRIPT_OPERATOR_TYPE_BINARY, 7},
    {(int8_t *)"!==", SCRIPT_OPERATOR_NOT_IDENTICAL, SCRIPT_OPERATOR_TYPE_BINARY, 7},
    {(int8_t *)"++", SCRIPT_OPERATOR_INCREMENT_PREFIX, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX, 1},
    {(int8_t *)"++", SCRIPT_OPERATOR_INCREMENT_POSTFIX, SCRIPT_OPERATOR_TYPE_UNARY_POSTFIX, 1},
    {(int8_t *)"--", SCRIPT_OPERATOR_DECREMENT_PREFIX, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX, 1},
    {(int8_t *)"--", SCRIPT_OPERATOR_DECREMENT_POSTFIX, SCRIPT_OPERATOR_TYPE_UNARY_POSTFIX, 1}
};

scriptBuiltInFunction_t scriptBuiltInFunctionSet[] = {
    {(int8_t *)"isNum", SCRIPT_FUNCTION_IS_NUM, 1},
    {(int8_t *)"isStr", SCRIPT_FUNCTION_IS_STR, 1},
    {(int8_t *)"isList", SCRIPT_FUNCTION_IS_LIST, 1},
    {(int8_t *)"isFunc", SCRIPT_FUNCTION_IS_FUNC, 1},
    {(int8_t *)"copy", SCRIPT_FUNCTION_COPY, 1},
    {(int8_t *)"str", SCRIPT_FUNCTION_STR, 1},
    {(int8_t *)"num", SCRIPT_FUNCTION_NUM, 1},
    {(int8_t *)"floor", SCRIPT_FUNCTION_FLOOR, 1},
    {(int8_t *)"len", SCRIPT_FUNCTION_LEN, 1},
    {(int8_t *)"ins", SCRIPT_FUNCTION_INS, 3},
    {(int8_t *)"rem", SCRIPT_FUNCTION_REM, 2},
    {(int8_t *)"pressKey", SCRIPT_FUNCTION_PRESS_KEY, 1},
    {(int8_t *)"getMode", SCRIPT_FUNCTION_GET_MODE, 0},
    {(int8_t *)"setMode", SCRIPT_FUNCTION_SET_MODE, 1},
    {(int8_t *)"getSelectionContents", SCRIPT_FUNCTION_GET_SELECTION_CONTENTS, 0},
    {(int8_t *)"getLineCount", SCRIPT_FUNCTION_GET_LINE_COUNT, 0},
    {(int8_t *)"getLineContents", SCRIPT_FUNCTION_GET_LINE_CONTENTS, 1},
    {(int8_t *)"getCursorCharIndex", SCRIPT_FUNCTION_GET_CURSOR_CHAR_INDEX, 0},
    {(int8_t *)"getCursorLineIndex", SCRIPT_FUNCTION_GET_CURSOR_LINE_INDEX, 0},
    {(int8_t *)"setCursorPos", SCRIPT_FUNCTION_SET_CURSOR_POS, 2},
    {(int8_t *)"runCommand", SCRIPT_FUNCTION_RUN_COMMAND, 2},
    {(int8_t *)"notifyUser", SCRIPT_FUNCTION_NOTIFY_USER, 1},
    {(int8_t *)"promptKey", SCRIPT_FUNCTION_PROMPT_KEY, 0},
    {(int8_t *)"promptCharacter", SCRIPT_FUNCTION_PROMPT_CHARACTER, 0},
    {(int8_t *)"bindKey", SCRIPT_FUNCTION_BIND_KEY, 2},
    {(int8_t *)"bindCommand", SCRIPT_FUNCTION_BIND_COMMAND, 2}
};

scriptHeapValue_t *firstScriptHeapValue = NULL;

int8_t loadScriptBody(scriptBody_t *destination, int8_t *path) {
    FILE *tempFile = fopen((char *)path, "r");
    if (tempFile == NULL) {
        return false;
    }
    destination->path = malloc(strlen((char *)path) + 1);
    strcpy((char *)(destination->path), (char *)path);
    fseek(tempFile, 0, SEEK_END);
    destination->length = ftell(tempFile);
    destination->text = malloc(destination->length + 1);
    fseek(tempFile, 0, SEEK_SET);
    fread(destination->text, 1, destination->length, tempFile);
    (destination->text)[destination->length] = 0;
    fclose(tempFile);
    return true;
}

void seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine) {
    while (scriptBodyLine->index < scriptBodyLine->scriptBody->length) {
        int8_t tempCharacter = (scriptBodyLine->scriptBody->text)[scriptBodyLine->index];
        scriptBodyLine->index += 1;
        if (tempCharacter == '\n') {
            break;
        }
    }
    scriptBodyLine->number += 1;
}

int8_t scriptBodyPosGetCharacter(scriptBodyPos_t *scriptBodyPos) {
    return (scriptBodyPos->scriptBodyLine->scriptBody->text)[scriptBodyPos->index];
}

void scriptBodyPosSkipWhitespace(scriptBodyPos_t *scriptBodyPos) {
    while (true) {
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        scriptBodyPos->index += 1;
    }
}

int8_t isFirstScriptIdentifierCharacter(int8_t character) {
    return ((character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || character == '_');
}

int8_t isScriptIdentifierCharacter(int8_t character) {
    return ((character >= 'a' && character <= 'z')
            || (character >= 'A' && character <= 'Z')
            || (character >= '0' && character <= '9')
            || character == '_');
}

int8_t isScriptNumberCharacter(int8_t character) {
    return ((character >= '0' && character <= '9')
            || character == '.');
}

void scriptBodyPosSeekEndOfIdentifier(scriptBodyPos_t *scriptBodyPos) {
    while (true) {
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (!isScriptIdentifierCharacter(tempCharacter)) {
            break;
        }
        scriptBodyPos->index += 1;
    }
}

void scriptBodyPosSeekEndOfNumber(scriptBodyPos_t *scriptBodyPos) {
    while (true) {
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (!isScriptNumberCharacter(tempCharacter)) {
            break;
        }
        scriptBodyPos->index += 1;
    }
}

int8_t scriptBodyPosMatchesOperator(scriptBodyPos_t *scriptBodyPos, scriptOperator_t *operator) {
    int8_t *tempText = getScriptBodyPosPointer(scriptBodyPos);
    int8_t tempOffset = 0;
    while (true) {
        int8_t tempCharacter1 = (operator->text)[tempOffset];
        int8_t tempCharacter2 = tempText[tempOffset];
        if (tempCharacter1 == 0) {
            return true;
        }
        if (tempCharacter1 != tempCharacter2) {
            return false;
        }
        tempOffset += 1;
    }
}

scriptOperator_t *scriptBodyPosGetOperator(scriptBodyPos_t *scriptBodyPos, int8_t operatorType) {
    int8_t tempLength = 3;
    while (tempLength >= 1) {
        int32_t index = 0;
        while (index < sizeof(scriptOperatorSet) / sizeof(*scriptOperatorSet)) {
            scriptOperator_t *tempOperator = scriptOperatorSet + index;
            if (tempOperator->type == operatorType) {
                if (strlen((char *)(tempOperator->text)) == tempLength) {
                    if (scriptBodyPosMatchesOperator(scriptBodyPos, tempOperator)) {
                        return tempOperator;
                    }
                }
            }
            index += 1;
        }
        tempLength -= 1;
    }
    return NULL;
}

void scriptBodyPosSkipOperator(scriptBodyPos_t *scriptBodyPos, scriptOperator_t *operator) {
    scriptBodyPos->index += strlen((char *)(operator->text));
}

int64_t getDistanceToScriptBodyPos(scriptBodyPos_t *startScriptBodyPos, scriptBodyPos_t *endScriptBodyPos) {
    return endScriptBodyPos->index - startScriptBodyPos->index;
}

int8_t *getScriptBodyPosPointer(scriptBodyPos_t *scriptBodyPos) {
    return scriptBodyPos->scriptBodyLine->scriptBody->text + scriptBodyPos->index;
}

scriptBuiltInFunction_t *findScriptBuiltInFunctionByName(int8_t *name, int64_t length) {
    int32_t index = 0;
    while (index < sizeof(scriptBuiltInFunctionSet) / sizeof(*scriptBuiltInFunctionSet)) {
        scriptBuiltInFunction_t *tempFunction = scriptBuiltInFunctionSet + index;
        if (strlen((char *)(tempFunction->name)) == length) {
            if (equalData(tempFunction->name, name, length)) {
                return tempFunction;
            }
        }
        index += 1;
    }
    return NULL;
}

scriptHeapValue_t *createScriptHeapValue() {
    scriptHeapValue_t *output = malloc(sizeof(scriptHeapValue_t));
    // TODO: Manage garbage collection of the heap value.
    
    return output;
}

scriptValue_t convertScriptValueToString(scriptValue_t value) {
    if (value.type == SCRIPT_VALUE_TYPE_STRING) {
        return value;
    }
    if (value.type == SCRIPT_VALUE_TYPE_NUMBER) {
        double tempNumber = *(double *)&(value.data);
        int8_t tempBuffer[50];
        sprintf((char *)tempBuffer, "%lf", tempNumber);
        int32_t tempLength = strlen((char *)tempBuffer);
        int32_t tempStartIndex = 0;
        while (tempStartIndex < tempLength) {
            int8_t tempCharacter = tempBuffer[tempStartIndex];
            if (tempCharacter == '.') {
                break;
            }
            tempStartIndex += 1;
        }
        int32_t index = tempLength - 1;
        while (index >= tempStartIndex) {
            int8_t tempCharacter = tempBuffer[index];
            if (tempCharacter != '0' && tempCharacter != '.') {
                break;
            }
            tempBuffer[index] = 0;
            tempLength = index;
            index -= 1;
        }
        scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
        vector_t *tempVector = malloc(sizeof(vector_t));
        createVectorFromArray(tempVector, 1, tempBuffer, tempLength + 1);
        *(vector_t **)&(tempHeapValue->data) = tempVector;
        scriptValue_t output;
        output.type = SCRIPT_VALUE_TYPE_STRING;
        *(scriptHeapValue_t **)&(output.data) = tempHeapValue;
        return output;
    }
    
    // TODO: Accommodate other input types.
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_NULL;
    return output;
}


