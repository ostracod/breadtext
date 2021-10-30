
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
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
    output->referenceCount = 0;
    firstHeapValue = output;
    return output;
}

void removeScriptValueReferenceHelper(scriptValue_t *value, int8_t shouldRecur);

void deleteScriptHeapValue(scriptHeapValue_t *value, int8_t shouldRecur) {
    if (value->previous == NULL) {
        firstHeapValue = value->next;
    } else {
        value->previous->next = value->next;
    }
    if (value->next != NULL) {
        value->next->previous = value->previous;
    }
    if (value->type == SCRIPT_VALUE_TYPE_LIST) {
        vector_t *tempVector = &(value->data);
        int64_t index = 0;
        while (index < tempVector->length) {
            scriptValue_t *tempValue = findVectorElement(tempVector, index);
            removeScriptValueReferenceHelper(tempValue, shouldRecur);
            index += 1;
        }
    }
    cleanUpVector(&(value->data));
    free(value);
}

int8_t scriptValueIsInHeap(scriptValue_t *value) {
    int8_t tempType = value->type;
    return (tempType == SCRIPT_VALUE_TYPE_STRING || tempType == SCRIPT_VALUE_TYPE_LIST);
}

void addScriptFrame(scriptFrame_t *frame) {
    frame->previous = NULL;
    frame->next = firstScriptFrame;
    if (firstScriptFrame != NULL) {
        firstScriptFrame->previous = frame;
    }
    firstScriptFrame = frame;
}

void removeScriptFrame(scriptFrame_t *frame) {
    if (frame->previous == NULL) {
        firstScriptFrame = frame->next;
    } else {
        frame->previous->next = frame->next;
    }
    if (frame->next != NULL) {
        frame->next->previous = frame->previous;
    }
}

void deleteScriptHeapValueIfUnreferenced(scriptHeapValue_t *value) {
    if (value->referenceCount <= 0 && value->lockDepth <= 0) {
        deleteScriptHeapValue(value, true);
    }
}

void lockScriptValue(scriptValue_t *value) {
    if (!scriptValueIsInHeap(value)) {
        return;
    }
    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value->data);
    tempHeapValue->lockDepth += 1;
}

void unlockScriptValue(scriptValue_t *value) {
    if (!scriptValueIsInHeap(value)) {
        return;
    }
    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value->data);
    tempHeapValue->lockDepth -= 1;
    deleteScriptHeapValueIfUnreferenced(tempHeapValue);
}

void addScriptValueReference(scriptValue_t *value) {
    if (!scriptValueIsInHeap(value)) {
        return;
    }
    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value->data);
    tempHeapValue->referenceCount += 1;
}

void removeScriptValueReferenceHelper(scriptValue_t *value, int8_t shouldRecur) {
    if (!scriptValueIsInHeap(value)) {
        return;
    }
    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value->data);
    tempHeapValue->referenceCount -= 1;
    if (shouldRecur) {
        deleteScriptHeapValueIfUnreferenced(tempHeapValue);
    }
}

void removeScriptValueReference(scriptValue_t *value) {
    removeScriptValueReferenceHelper(value, true);
}

void swapScriptValueReference(scriptValue_t *destination, scriptValue_t *source) {
    addScriptValueReference(source);
    removeScriptValueReference(destination);
    *destination = *source;
}

void unmarkScriptValue(scriptValue_t *value) {
    if (scriptValueIsInHeap(value)) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)(value->data);
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
        vector_t *tempVector = &(value->data);
        int64_t index = 0;
        while (index < tempVector->length) {
            scriptValue_t *tempValue = findVectorElement(tempVector, index);
            unmarkScriptValue(tempValue);
            index += 1;
        }
    }
}

void unmarkScriptFrame(scriptFrame_t *frame) {
    int32_t index = 0;
    while (index < frame->valueAmount) {
        scriptValue_t *tempValue = frame->valueList + index;
        unmarkScriptValue(tempValue);
        index += 1;
    }
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

int8_t *getScriptValueText(scriptValue_t value) {
    scriptHeapValue_t *heapValue = *(scriptHeapValue_t **)(value.data);
    vector_t *textVector = &(heapValue->data);
    return textVector->data;
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
        double tempNumber = *(double *)(value.data);
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

scriptValue_t convertScriptValueToNumber(scriptValue_t value) {
    if (value.type == SCRIPT_VALUE_TYPE_NUMBER) {
        return value;
    }
    if (value.type == SCRIPT_VALUE_TYPE_STRING) {
        int8_t *tempText = getScriptValueText(value);
        double tempNumber;
        int32_t tempResult = sscanf((char *)tempText, "%lf", &tempNumber);
        scriptValue_t output;
        if (tempResult < 1) {
            output.type = SCRIPT_VALUE_TYPE_NULL;
            return output;
        }
        output.type = SCRIPT_VALUE_TYPE_NUMBER;
        *(double *)(output.data) = tempNumber;
        return output;
    }
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_NULL;
    return output;
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
        return (*(double *)(value1->data) == *(double *)(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_FUNCTION) {
        return (*(scriptBaseFunction_t **)(value1->data) == *(scriptBaseFunction_t **)(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_STRING) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(value1->data);
        scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)(value2->data);
        vector_t *tempText1 = &(tempHeapValue1->data);
        vector_t *tempText2 = &(tempHeapValue2->data);
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
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(value1->data);
        scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)(value2->data);
        vector_t *tempList1 = &(tempHeapValue1->data);
        vector_t *tempList2 = &(tempHeapValue2->data);
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
                if (*(scriptHeapValue_t **)(tempValue1.data) != *(scriptHeapValue_t **)(tempValue2.data)) {
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
        return (*(double *)(value1->data) == *(double *)(value2->data));
    }
    if (type1 == SCRIPT_VALUE_TYPE_STRING || type1 == SCRIPT_VALUE_TYPE_LIST
            || type1 == SCRIPT_VALUE_TYPE_FUNCTION) {
        return (*(void **)(value1->data) == *(void **)(value2->data));
    }
    return true;
}

scriptValue_t copyScriptValue(scriptValue_t *value) {
    int8_t tempType = value->type;
    if (tempType == SCRIPT_VALUE_TYPE_NULL || tempType == SCRIPT_VALUE_TYPE_MISSING) {
        return *value;
    }
    if (tempType == SCRIPT_VALUE_TYPE_NUMBER || tempType == SCRIPT_VALUE_TYPE_FUNCTION) {
        return *value;
    }
    if (tempType == SCRIPT_VALUE_TYPE_STRING) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(value->data);
        vector_t *tempText = &(tempHeapValue1->data);
        scriptHeapValue_t *tempHeapValue2 = createScriptHeapValue();
        scriptValue_t output;
        tempHeapValue2->type = SCRIPT_VALUE_TYPE_STRING;
        copyVector(&(tempHeapValue2->data), tempText);
        output.type = SCRIPT_VALUE_TYPE_STRING;
        *(scriptHeapValue_t **)(output.data) = tempHeapValue2;
        return output;
    }
    if (tempType == SCRIPT_VALUE_TYPE_LIST) {
        scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)(value->data);
        vector_t *tempList = &(tempHeapValue1->data);
        scriptHeapValue_t *tempHeapValue2 = createScriptHeapValue();
        scriptValue_t output;
        tempHeapValue2->type = SCRIPT_VALUE_TYPE_LIST;
        copyVector(&(tempHeapValue2->data), tempList);
        output.type = SCRIPT_VALUE_TYPE_LIST;
        *(scriptHeapValue_t **)(output.data) = tempHeapValue2;
        int64_t index = 0;
        while (index < tempHeapValue2->data.length) {
            scriptValue_t *tempValue = findVectorElement(&(tempHeapValue2->data), index);
            addScriptValueReference(tempValue);
            index += 1;
        }
        return output;
    }
    return *value;
}


