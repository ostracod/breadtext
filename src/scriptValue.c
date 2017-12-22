
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
    {(int8_t *)"promptChar", SCRIPT_FUNCTION_PROMPT_CHAR, 0},
    {(int8_t *)"bindKey", SCRIPT_FUNCTION_BIND_KEY, 2},
    {(int8_t *)"mapKey", SCRIPT_FUNCTION_MAP_KEY, 3},
    {(int8_t *)"bindCommand", SCRIPT_FUNCTION_BIND_COMMAND, 2},
    {(int8_t *)"testLog", SCRIPT_FUNCTION_TEST_LOG, 1}
};

scriptConstant_t scriptConstantSet[] = {
    {(int8_t *)"KEY_ESCAPE", 27},
    {(int8_t *)"KEY_LEFT", KEY_LEFT},
    {(int8_t *)"KEY_RIGHT", KEY_RIGHT},
    {(int8_t *)"KEY_UP", KEY_UP},
    {(int8_t *)"KEY_DOWN", KEY_DOWN},
    {(int8_t *)"KEY_SPACE", ' '},
    {(int8_t *)"KEY_NEWLINE", '\n'},
    {(int8_t *)"KEY_BACKSPACE", 127},
    {(int8_t *)"KEY_TAB", '\t'},
    {(int8_t *)"KEY_BACKTAB", KEY_BTAB},
    {(int8_t *)"MODE_COMMAND", COMMAND_MODE},
    {(int8_t *)"MODE_TEXT_ENTRY", TEXT_ENTRY_MODE},
    {(int8_t *)"MODE_TEXT_REPLACE", TEXT_REPLACE_MODE},
    {(int8_t *)"MODE_HIGHLIGHT_CHARACTER", HIGHLIGHT_CHARACTER_MODE},
    {(int8_t *)"MODE_HIGHLIGHT_STATIC", HIGHLIGHT_STATIC_MODE},
    {(int8_t *)"MODE_HIGHLIGHT_LINE", HIGHLIGHT_LINE_MODE}
};

scriptHeapValue_t *firstScriptHeapValue = NULL;

int8_t loadScriptBody(scriptBody_t **destination, int8_t *path) {
    FILE *tempFile = fopen((char *)path, "r");
    if (tempFile == NULL) {
        return false;
    }
    scriptBody_t *tempScriptBody = malloc(sizeof(scriptBody_t));
    tempScriptBody->path = malloc(strlen((char *)path) + 1);
    strcpy((char *)(tempScriptBody->path), (char *)path);
    fseek(tempFile, 0, SEEK_END);
    tempScriptBody->length = ftell(tempFile);
    tempScriptBody->text = malloc(tempScriptBody->length + 1);
    fseek(tempFile, 0, SEEK_SET);
    fread(tempScriptBody->text, 1, tempScriptBody->length, tempFile);
    (tempScriptBody->text)[tempScriptBody->length] = 0;
    fclose(tempFile);
    *destination = tempScriptBody;
    return true;
}

void loadScriptBodyFromText(scriptBody_t **destination, int8_t *text) {
    scriptBody_t *tempScriptBody = malloc(sizeof(scriptBody_t));
    int8_t tempPath[] = "/bupkis.btsl";
    tempScriptBody->path = malloc(strlen((char *)tempPath) + 1);
    strcpy((char *)(tempScriptBody->path), (char *)tempPath);
    tempScriptBody->length = strlen((char *)text);
    tempScriptBody->text = malloc(tempScriptBody->length + 1);
    strcpy((char *)(tempScriptBody->text), (char *)text);
    *destination = tempScriptBody;
}

int8_t seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine) {
    scriptBodyLine->number += 1;
    int8_t tempIsEscaped = false;
    while (scriptBodyLine->index < scriptBodyLine->scriptBody->length) {
        int8_t tempCharacter = (scriptBodyLine->scriptBody->text)[scriptBodyLine->index];
        scriptBodyLine->index += 1;
        if (tempIsEscaped) {
            if (tempCharacter == '\n') {
                scriptBodyLine->number += 1;
            }
            tempIsEscaped = false;
        } else {
            if (tempCharacter == '\n') {
                return true;
            }
            if (tempCharacter == '\\') {
                tempIsEscaped = true;
            }
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
        if (tempCharacter == '\\') {
            scriptBodyPos->index += 1;
            tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempCharacter != ' ' && tempCharacter != '\t' && tempCharacter != '\n') {
                scriptBodyPos->index -= 1;
                break;
            }
        } else if (tempCharacter != ' ' && tempCharacter != '\t') {
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

void deleteScriptCustomFunction(scriptCustomFunction_t *function) {
    free(function);
}

scriptHeapValue_t *createScriptHeapValue() {
    scriptHeapValue_t *output = malloc(sizeof(scriptHeapValue_t));
    output->type = SCRIPT_VALUE_TYPE_MISSING;
    output->previous = NULL;
    output->next = firstHeapValue;
    if (firstHeapValue != NULL) {
        firstHeapValue->previous = output;
    }
    output->lockDepth = 0;
    firstHeapValue = output;
    return output;
}

void deleteScriptHeapValue(scriptHeapValue_t *value) {
    if (value->previous == NULL) {
        firstHeapValue = value->next;
    } else {
        value->previous->next = value->next;
    }
    if (value->next != NULL) {
        value->next->previous = value->previous;
    }
    int8_t tempType = value->type;
    if (tempType == SCRIPT_VALUE_TYPE_STRING || tempType == SCRIPT_VALUE_TYPE_LIST) {
        vector_t *tempVector = *(vector_t **)&(value->data);
        cleanUpVector(tempVector);
        free(tempVector);
    }
    if (tempType == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        scriptCustomFunction_t *tempFunction = *(scriptCustomFunction_t **)&(value->data);
        deleteScriptCustomFunction(tempFunction);
    }
    free(value);
}

int8_t scriptValueIsInHeap(scriptValue_t *value) {
    int8_t tempType = value->type;
    return (tempType == SCRIPT_VALUE_TYPE_STRING || tempType == SCRIPT_VALUE_TYPE_LIST
        || tempType == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION);
}

void lockScriptValue(scriptValue_t *value) {
    if (scriptValueIsInHeap(value)) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(value->data);
        tempHeapValue->lockDepth += 1;
    }
}

void unlockScriptValue(scriptValue_t *value) {
    if (scriptValueIsInHeap(value)) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(value->data);
        tempHeapValue->lockDepth -= 1;
    }
}

void unmarkScriptValue(scriptValue_t *value) {
    if (scriptValueIsInHeap(value)) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(value->data);
        unmarkScriptHeapValue(tempHeapValue);
    }
}

void unmarkScriptHeapValue(scriptHeapValue_t *value) {
    if (!value->isMarked) {
        return;
    }
    value->isMarked = false;
    int8_t tempType = value->type;
    if (tempType == SCRIPT_VALUE_TYPE_LIST) {
        vector_t *tempVector = *(vector_t **)&(value->data);
        int64_t index = 0;
        while (index < tempVector->length) {
            scriptValue_t *tempValue = findVectorElement(tempVector, index);
            unmarkScriptValue(tempValue);
            index += 1;
        }
    }
}

void unmarkScriptScope(scriptScope_t *scope) {
    int64_t index = 0;
    while (index < scope->variableList.length) {
        scriptVariable_t *tempVariable = findVectorElement(&(scope->variableList), index);
        unmarkScriptValue(&(tempVariable->value));
        index += 1;
    }
}

void unmarkScriptBody(scriptBody_t *body) {
    int64_t index = 0;
    while (index < body->scopeStack.length) {
        scriptScope_t *tempScope;
        getVectorElement(&tempScope, &(body->scopeStack), index);
        unmarkScriptScope(tempScope);
        index += 1;
    }
}

scriptValue_t convertCharacterVectorToStringValue(vector_t vector) {
    scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
    vector_t *tempVector = malloc(sizeof(vector_t));
    *tempVector = vector;
    tempHeapValue->type = SCRIPT_VALUE_TYPE_STRING;
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

scriptValue_t convertScriptValueToNumber(scriptValue_t value) {
    if (value.type == SCRIPT_VALUE_TYPE_NUMBER) {
        return value;
    }
    if (value.type == SCRIPT_VALUE_TYPE_STRING) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(value.data);
        vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
        double tempNumber;
        int32_t tempResult = sscanf((char *)(tempText->data), "%lf", &tempNumber);
        scriptValue_t output;
        if (tempResult < 1) {
            output.type = SCRIPT_VALUE_TYPE_NULL;
            return output;
        }
        output.type = SCRIPT_VALUE_TYPE_NUMBER;
        *(double *)&(output.data) = tempNumber;
        return output;
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

scriptScope_t *createEmptyScriptScope() {
    scriptScope_t *output = malloc(sizeof(scriptScope_t));
    createEmptyVector(&(output->variableList), sizeof(scriptVariable_t));
    return output;
}

scriptScope_t *scriptBodyAddEmptyScope(scriptBody_t *body) {
    scriptScope_t *output = createEmptyScriptScope();
    pushVectorElement(&(body->scopeStack), &output);
    return output;
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

int64_t scriptScopeFindVariableIndexWithNameLength(scriptScope_t *scope, int8_t *name, int64_t length) {
    int32_t index = 0;
    while (index < scope->variableList.length) {
        scriptVariable_t *tempVariable = findVectorElement(&(scope->variableList), index);
        if (strlen((char *)(tempVariable->name)) == length) {
            if (equalData(tempVariable->name, name, length)) {
                return index;
            }
        }
        index += 1;
    }
    return -1;
}

int64_t scriptScopeFindVariableIndex(scriptScope_t *scope, int8_t *name) {
    return scriptScopeFindVariableIndexWithNameLength(scope, name, strlen((char *)name));
}

scriptVariable_t *scriptScopeFindVariable(scriptScope_t *scope, int8_t *name) {
    int64_t index = scriptScopeFindVariableIndex(scope, name);
    if (index < 0) {
        return NULL;
    }
    return findVectorElement(&(scope->variableList), index);
}

void cleanUpScriptVariable(scriptVariable_t *variable) {
    free(variable->name);
}

void deleteScriptScope(scriptScope_t *scope) {
    int64_t index = 0;
    while (index < scope->variableList.length) {
        scriptVariable_t *tempVariable = findVectorElement(&(scope->variableList), index);
        cleanUpScriptVariable(tempVariable);
        index += 1;
    }
    cleanUpVector(&(scope->variableList));
    free(scope);
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
        tempHeapValue2->type = SCRIPT_VALUE_TYPE_STRING;
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
        tempHeapValue2->type = SCRIPT_VALUE_TYPE_LIST;
        *(vector_t **)&(tempHeapValue2->data) = tempList2;
        output.type = SCRIPT_VALUE_TYPE_LIST;
        *(scriptHeapValue_t **)&(output.data) = tempHeapValue2;
        return output;
    }
    return *value;
}

scriptConstant_t *getScriptConstantByName(int8_t *name, int64_t length) {
    int32_t index = 0;
    while (index < sizeof(scriptConstantSet) / sizeof(*scriptConstantSet)) {
        scriptConstant_t *tempConstant = scriptConstantSet + index;
        if (strlen((char *)(tempConstant->name)) == length) {
            if (equalData(tempConstant->name, name, length)) {
                return tempConstant;
            }
        }
        index += 1;
    }
    return NULL;
}



