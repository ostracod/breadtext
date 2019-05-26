
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


