
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "display.h"

scriptBuiltInFunction_t scriptBuiltInFunctionSet[] = {
    {(int8_t *)"isNum", IS_NUM, 1},
    {(int8_t *)"isStr", IS_STR, 1},
    {(int8_t *)"isList", IS_LIST, 1},
    {(int8_t *)"isFunc", IS_FUNC, 1},
    {(int8_t *)"copy", COPY, 1},
    {(int8_t *)"str", STR, 1},
    {(int8_t *)"num", NUM, 1},
    {(int8_t *)"floor", FLOOR, 1},
    {(int8_t *)"len", LEN, 1},
    {(int8_t *)"ins", INS, 3},
    {(int8_t *)"rem", REM, 2},
    {(int8_t *)"pressKey", PRESS_KEY, 1},
    {(int8_t *)"getMode", GET_MODE, 0},
    {(int8_t *)"setMode", SET_MODE, 1},
    {(int8_t *)"getSelectionContents", GET_SELECTION_CONTENTS, 0},
    {(int8_t *)"getLineCount", GET_LINE_COUNT, 0},
    {(int8_t *)"getLineContents", GET_LINE_CONTENTS, 1},
    {(int8_t *)"getCursorCharIndex", GET_CURSOR_CHAR_INDEX, 0},
    {(int8_t *)"getCursorLineIndex", GET_CURSOR_LINE_INDEX, 0},
    {(int8_t *)"setCursorPos", SET_CURSOR_POS, 2},
    {(int8_t *)"runCommand", RUN_COMMAND, 2},
    {(int8_t *)"notifyUser", NOTIFY_USER, 1},
    {(int8_t *)"promptKey", PROMPT_KEY, 0},
    {(int8_t *)"promptCharacter", PROMPT_CHARACTER, 0},
    {(int8_t *)"bindKey", BIND_KEY, 2},
    {(int8_t *)"bindCommand", BIND_COMMAND, 2}
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


