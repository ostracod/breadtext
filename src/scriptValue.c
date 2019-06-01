
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "scriptValue.h"

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

scriptValue_t convertCharacterVectorToStringValue(vector_t vector) {
    scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
    tempHeapValue->type = SCRIPT_VALUE_TYPE_STRING;
    tempHeapValue->data = vector;
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_STRING;
    *(scriptHeapValue_t **)(output.data) = tempHeapValue;
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
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value.data);
        vector_t *tempList = &(tempHeapValue->data);
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
            scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(tempStringValue.data);
            vector_t *tempText = &(tempHeapValue->data);
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
    if (value.type == SCRIPT_VALUE_TYPE_FUNCTION) {
        return convertTextToStringValue((int8_t *)"(Function)");
    }
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_NULL;
    return output;
}


