
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "scriptParse.h"
#include "display.h"

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

scriptOperator_t scriptOperatorSet[] = {
    {(int8_t *)"=", SCRIPT_OPERATOR_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"+", SCRIPT_OPERATOR_ADD, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 4},
    {(int8_t *)"+=", SCRIPT_OPERATOR_ADD_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"-", SCRIPT_OPERATOR_SUBTRACT, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 4},
    {(int8_t *)"-=", SCRIPT_OPERATOR_SUBTRACT_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"-", SCRIPT_OPERATOR_NEGATE, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX, 1},
    {(int8_t *)"*", SCRIPT_OPERATOR_MULTIPLY, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 3},
    {(int8_t *)"*=", SCRIPT_OPERATOR_MULTIPLY_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"/", SCRIPT_OPERATOR_DIVIDE, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 3},
    {(int8_t *)"/=", SCRIPT_OPERATOR_DIVIDE_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"%", SCRIPT_OPERATOR_MODULUS, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 3},
    {(int8_t *)"%=", SCRIPT_OPERATOR_MODULUS_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"&&", SCRIPT_OPERATOR_BOOLEAN_AND, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 11},
    {(int8_t *)"&&=", SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"||", SCRIPT_OPERATOR_BOOLEAN_OR, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 13},
    {(int8_t *)"||=", SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"^^", SCRIPT_OPERATOR_BOOLEAN_XOR, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 12},
    {(int8_t *)"^^=", SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"!", SCRIPT_OPERATOR_BOOLEAN_NOT, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX, 1},
    {(int8_t *)"&", SCRIPT_OPERATOR_BITWISE_AND, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 8},
    {(int8_t *)"&=", SCRIPT_OPERATOR_BITWISE_AND_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"|", SCRIPT_OPERATOR_BITWISE_OR, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 10},
    {(int8_t *)"|=", SCRIPT_OPERATOR_BITWISE_OR_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"^", SCRIPT_OPERATOR_BITWISE_XOR, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 9},
    {(int8_t *)"^=", SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)"~", SCRIPT_OPERATOR_BITWISE_NOT, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX, 1},
    {(int8_t *)"<<", SCRIPT_OPERATOR_BITSHIFT_LEFT, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 5},
    {(int8_t *)"<<=", SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)">>", SCRIPT_OPERATOR_BITSHIFT_RIGHT, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 5},
    {(int8_t *)">>=", SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 14},
    {(int8_t *)">", SCRIPT_OPERATOR_GREATER, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 6},
    {(int8_t *)">=", SCRIPT_OPERATOR_GREATER_OR_EQUAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 6},
    {(int8_t *)"<", SCRIPT_OPERATOR_LESS, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 6},
    {(int8_t *)"<=", SCRIPT_OPERATOR_LESS_OR_EQUAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 6},
    {(int8_t *)"==", SCRIPT_OPERATOR_EQUAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 7},
    {(int8_t *)"!=", SCRIPT_OPERATOR_NOT_EQUAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 7},
    {(int8_t *)"===", SCRIPT_OPERATOR_IDENTICAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 7},
    {(int8_t *)"!==", SCRIPT_OPERATOR_NOT_IDENTICAL, SCRIPT_OPERATOR_ARRANGEMENT_BINARY, 7},
    {(int8_t *)"++", SCRIPT_OPERATOR_INCREMENT_PREFIX, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX, 1},
    {(int8_t *)"++", SCRIPT_OPERATOR_INCREMENT_POSTFIX, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_POSTFIX, 1},
    {(int8_t *)"--", SCRIPT_OPERATOR_DECREMENT_PREFIX, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX, 1},
    {(int8_t *)"--", SCRIPT_OPERATOR_DECREMENT_POSTFIX, SCRIPT_OPERATOR_ARRANGEMENT_UNARY_POSTFIX, 1}
};

scriptBuiltInFunction_t scriptBuiltInFunctionSet[] = {
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"isNum", SCRIPT_FUNCTION_IS_NUM},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"isStr", SCRIPT_FUNCTION_IS_STR},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"isList", SCRIPT_FUNCTION_IS_LIST},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"isFunc", SCRIPT_FUNCTION_IS_FUNC},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"copy", SCRIPT_FUNCTION_COPY},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"str", SCRIPT_FUNCTION_STR},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"num", SCRIPT_FUNCTION_NUM},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"floor", SCRIPT_FUNCTION_FLOOR},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"len", SCRIPT_FUNCTION_LEN},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 3}, (int8_t *)"ins", SCRIPT_FUNCTION_INS},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 2}, (int8_t *)"rem", SCRIPT_FUNCTION_REM},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getTimestamp", SCRIPT_FUNCTION_GET_TIMESTAMP},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"pressKey", SCRIPT_FUNCTION_PRESS_KEY},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getMode", SCRIPT_FUNCTION_GET_MODE},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"setMode", SCRIPT_FUNCTION_SET_MODE},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getSelectionContents", SCRIPT_FUNCTION_GET_SELECTION_CONTENTS},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getLineCount", SCRIPT_FUNCTION_GET_LINE_COUNT},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"getLineContents", SCRIPT_FUNCTION_GET_LINE_CONTENTS},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getCursorCharIndex", SCRIPT_FUNCTION_GET_CURSOR_CHAR_INDEX},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"getCursorLineIndex", SCRIPT_FUNCTION_GET_CURSOR_LINE_INDEX},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 2}, (int8_t *)"setCursorPos", SCRIPT_FUNCTION_SET_CURSOR_POS},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 2}, (int8_t *)"runCommand", SCRIPT_FUNCTION_RUN_COMMAND},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"notifyUser", SCRIPT_FUNCTION_NOTIFY_USER},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"promptKey", SCRIPT_FUNCTION_PROMPT_KEY},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"promptChar", SCRIPT_FUNCTION_PROMPT_CHAR},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 2}, (int8_t *)"bindKey", SCRIPT_FUNCTION_BIND_KEY},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 3}, (int8_t *)"mapKey", SCRIPT_FUNCTION_MAP_KEY},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 2}, (int8_t *)"bindCommand", SCRIPT_FUNCTION_BIND_COMMAND},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 1}, (int8_t *)"testLog", SCRIPT_FUNCTION_TEST_LOG},
    {{SCRIPT_FUNCTION_TYPE_BUILT_IN, 0}, (int8_t *)"rand", SCRIPT_FUNCTION_RAND}
};

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

scriptOperator_t *scriptBodyPosGetOperator(scriptBodyPos_t *scriptBodyPos, int8_t operatorArrangement) {
    int8_t tempLength = 3;
    while (tempLength >= 1) {
        int32_t index = 0;
        while (index < sizeof(scriptOperatorSet) / sizeof(*scriptOperatorSet)) {
            scriptOperator_t *tempOperator = scriptOperatorSet + index;
            if (tempOperator->arrangement == operatorArrangement) {
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

int8_t parseScriptBody(script_t **destination, scriptBody_t *scriptBody) {
    // TODO: Everything.
    
    return true;
}


