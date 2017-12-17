
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

int8_t seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine) {
    scriptBodyLine->number += 1;
    while (scriptBodyLine->index < scriptBodyLine->scriptBody->length) {
        int8_t tempCharacter = (scriptBodyLine->scriptBody->text)[scriptBodyLine->index];
        scriptBodyLine->index += 1;
        if (tempCharacter == '\n') {
            return true;
        }
        if (tempCharacter == 0) {
            return false;
        }
    }
    return false;
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

scriptValue_t convertCharacterVectorToStringValue(vector_t vector) {
    scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
    vector_t *tempVector = malloc(sizeof(vector_t));
    *tempVector = vector;
    *(vector_t **)&(tempHeapValue->data) = tempVector;
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_STRING;
    *(scriptHeapValue_t **)&(output.data) = tempHeapValue;
    return output;
}

scriptValue_t convertTextToStringValue(int8_t *text) {
    int64_t tempLength = strlen((char *)text);
    vector_t tempVector;
    createVectorFromArray(&tempVector, 1, text, tempLength + 1);
    return convertCharacterVectorToStringValue(tempVector);
}

scriptValue_t convertScriptValueToString(scriptValue_t value) {
    if (value.type == SCRIPT_VALUE_TYPE_MISSING) {
        return convertTextToStringValue((int8_t *)"(Missing Value)");
    }
    if (value.type == SCRIPT_VALUE_TYPE_NULL) {
        return convertTextToStringValue((int8_t *)"(Null)");
    }
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
            index -= 1;
        }
        return convertTextToStringValue(tempBuffer);
    }
    if (value.type == SCRIPT_VALUE_TYPE_LIST) {
        vector_t tempVector;
        createEmptyVector(&tempVector, 1);
        int8_t tempCharacter;
        tempCharacter = '[';
        pushVectorElement(&tempVector, &tempCharacter);
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(value.data);
        vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
        int64_t index = 0;
        while (index < tempList->length) {
            if (index > 0) {
                tempCharacter = ',';
                pushVectorElement(&tempVector, &tempCharacter);
                tempCharacter = ' ';
                pushVectorElement(&tempVector, &tempCharacter);
            }
            scriptValue_t tempValue;
            getVectorElement(&tempValue, tempList, index);
            scriptValue_t tempStringValue = convertScriptValueToString(tempValue);
            scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempStringValue.data);
            vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
            pushVectorOntoVector(&tempVector, tempText);
            removeVectorElement(&tempVector, tempVector.length - 1);
            index += 1;
        }
        tempCharacter = ']';
        pushVectorElement(&tempVector, &tempCharacter);
        tempCharacter = 0;
        pushVectorElement(&tempVector, &tempCharacter);
        return convertCharacterVectorToStringValue(tempVector);
    }
    if (value.type == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION) {
        return convertTextToStringValue((int8_t *)"(Built-In Function)");
    }
    if (value.type == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        return convertTextToStringValue((int8_t *)"(Custom Function)");
    }
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_NULL;
    return output;
}

int8_t scriptBodyPosTextMatchesIdentifier(scriptBodyPos_t *scriptBodyPos, int8_t *text) {
    scriptBodyPos_t tempScriptBodyPos;
    tempScriptBodyPos = *scriptBodyPos;
    scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
    int8_t *tempText = getScriptBodyPosPointer(scriptBodyPos);
    int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
    if (tempLength != strlen((char *)text)) {
        return false;
    }
    if (equalData(text, tempText, tempLength)) {
        *scriptBodyPos = tempScriptBodyPos;
        return true;
    }
    return false;
}

scriptScope_t createEmptyScriptScope() {
    scriptScope_t output;
    createEmptyVector(&(output.variableList), sizeof(scriptVariable_t));
    return output;
}

scriptScope_t *scriptBodyAddScope(scriptBody_t *body, scriptScope_t scope) {
    int32_t index = body->scopeStack.length;
    pushVectorElement(&(body->scopeStack), &scope);
    return findVectorElement(&(body->scopeStack), index);
}

scriptVariable_t createEmptyScriptVariable(int8_t *name) {
    scriptVariable_t output;
    output.name = malloc(strlen((char *)name) + 1);
    strcpy((char *)(output.name), (char *)name);
    output.value.type = SCRIPT_VALUE_TYPE_MISSING;
    return output;
}

scriptVariable_t *scriptScopeAddVariable(scriptScope_t *scope, scriptVariable_t variable) {
    int32_t index = scope->variableList.length;
    pushVectorElement(&(scope->variableList), &variable);
    return findVectorElement(&(scope->variableList), index);
}

scriptVariable_t *scriptScopeFindVariableWithNameLength(scriptScope_t *scope, int8_t *name, int64_t length) {
    int32_t index = 0;
    while (index < scope->variableList.length) {
        scriptVariable_t *tempVariable = findVectorElement(&(scope->variableList), index);
        if (strlen((char *)(tempVariable->name)) == length) {
            if (equalData(tempVariable->name, name, length)) {
                return tempVariable;
            }
        }
        index += 1;
    }
    return NULL;
}

scriptVariable_t *scriptScopeFindVariable(scriptScope_t *scope, int8_t *name) {
    return scriptScopeFindVariableWithNameLength(scope, name, strlen((char *)name));
}

int8_t scriptValuesAreEqualShallow(scriptValue_t *value1, scriptValue_t *value2) {
    int8_t type1 = value1->type;
    int8_t type2 = value2->type;
    if (type1 != type2) {
        return false;
    }
    if (type1 == SCRIPT_VALUE_TYPE_NULL || type1 == SCRIPT_VALUE_TYPE_MISSING) {
        return true;
    }
    if (type1 == SCRIPT_VALUE_TYPE_NUMBER) {
        return (*(double *)&(value1->data) == *(double *)&(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION || type1 == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        return (*(void **)&(value1->data) == *(void **)&(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_STRING) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(value1->data);
        scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(value2->data);
        vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
        vector_t *tempText2 = *(vector_t **)&(tempHeapValue2->data);
        if (tempText1->length != tempText2->length) {
            return false;
        }
        int64_t index = 0;
        while (index < tempText1->length) {
            int8_t tempCharacter1;
            int8_t tempCharacter2;
            getVectorElement(&tempCharacter1, tempText1, index);
            getVectorElement(&tempCharacter2, tempText2, index);
            if (tempCharacter1 != tempCharacter2) {
                return false;
            }
            index += 1;
        }
        return true;
    }
    if (type1 == SCRIPT_VALUE_TYPE_LIST) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(value1->data);
        scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(value2->data);
        vector_t *tempList1 = *(vector_t **)&(tempHeapValue1->data);
        vector_t *tempList2 = *(vector_t **)&(tempHeapValue2->data);
        if (tempList1->length != tempList2->length) {
            return false;
        }
        int64_t index = 0;
        while (index < tempList1->length) {
            scriptValue_t tempValue1;
            scriptValue_t tempValue2;
            getVectorElement(&tempValue1, tempList1, index);
            getVectorElement(&tempValue2, tempList2, index);
            if (tempValue1.type != tempValue2.type) {
                return false;
            }
            if (tempValue1.type == SCRIPT_VALUE_TYPE_LIST) {
                if (*(void **)&(tempValue1.data) != *(void **)&(tempValue2.data)) {
                    return false;
                }
            } else {
                if (!scriptValuesAreEqualShallow(&tempValue1, &tempValue2)) {
                    return false;
                }
            }
            index += 1;
        }
        return true;
    }
    return true;
}

int8_t scriptValuesAreIdentical(scriptValue_t *value1, scriptValue_t *value2) {
    int8_t type1 = value1->type;
    int8_t type2 = value2->type;
    if (type1 != type2) {
        return false;
    }
    if (type1 == SCRIPT_VALUE_TYPE_NULL || type1 == SCRIPT_VALUE_TYPE_MISSING) {
        return true;
    }
    if (type1 == SCRIPT_VALUE_TYPE_NUMBER) {
        return (*(double *)&(value1->data) == *(double *)&(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_STRING || type1 == SCRIPT_VALUE_TYPE_LIST
            || type1 == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION || type1 == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        return (*(void **)&(value1->data) == *(void **)&(value2->data));
    }
    return true;
}

scriptValue_t copyScriptValue(scriptValue_t *value) {
    int8_t tempType = value->type;
    if (tempType == SCRIPT_VALUE_TYPE_NULL || tempType == SCRIPT_VALUE_TYPE_MISSING) {
        return *value;
    }
    if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
        return *value;
    }
    if (tempType == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION || tempType == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        return *value;
    }
    if (tempType == SCRIPT_VALUE_TYPE_STRING) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(value->data);
        vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
        vector_t *tempText2 = malloc(sizeof(vector_t));
        scriptHeapValue_t *tempHeapValue2 = createScriptHeapValue();
        scriptValue_t output;
        copyVector(tempText2, tempText1);
        *(vector_t **)&(tempHeapValue2->data) = tempText2;
        output.type = SCRIPT_VALUE_TYPE_STRING;
        *(scriptHeapValue_t **)&(output.data) = tempHeapValue2;
        return output;
    }
    if (tempType == SCRIPT_VALUE_TYPE_LIST) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(value->data);
        vector_t *tempList1 = *(vector_t **)&(tempHeapValue1->data);
        vector_t *tempList2 = malloc(sizeof(vector_t));
        scriptHeapValue_t *tempHeapValue2 = createScriptHeapValue();
        scriptValue_t output;
        copyVector(tempList2, tempList1);
        *(vector_t **)&(tempHeapValue2->data) = tempList2;
        output.type = SCRIPT_VALUE_TYPE_LIST;
        *(scriptHeapValue_t **)&(output.data) = tempHeapValue2;
        return output;
    }
    return *value;
}


