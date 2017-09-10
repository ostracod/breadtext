
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "vector.h"

void createEmptyVector(vector_t *destination, int64_t elementSize) {
    destination->elementSize = elementSize;
    destination->dataSize = elementSize * 2;
    destination->data = malloc(destination->dataSize);
    destination->length = 0;
}

void deleteVector(vector_t *vector) {
    free(vector->data);
}

void setVectorLength(vector_t *vector, int64_t length) {
    int64_t tempSize = length * vector->elementSize;
    if (tempSize > vector->dataSize) {
        vector->dataSize = tempSize * 2;
        vector->data = realloc(vector->data, vector->dataSize);
    } else if (tempSize < vector->dataSize / 4 && tempSize != 0) {
        vector->dataSize = tempSize * 2;
        vector->data = realloc(vector->data, vector->dataSize);
    }
    vector->length = length;
}

void getVectorElement(void *destination, vector_t *vector, int64_t index) {
    copyData(destination, vector->data + index * vector->elementSize, vector->elementSize);
}

void setVectorElement(vector_t *vector, int64_t index, void *source) {
    copyData(vector->data + index * vector->elementSize, source, vector->elementSize);
}

void *findVectorElement(vector_t *vector, int64_t index) {
    return vector->data + index * vector->elementSize;
}

void insertVectorElement(vector_t *vector, int64_t index, void *source) {
    setVectorLength(vector, vector->length + 1);
    copyData(vector->data + (index + 1) * vector->elementSize, vector->data + index * vector->elementSize, (vector->length - index) * vector->elementSize);
    copyData(vector->data + index * vector->elementSize, source, vector->elementSize);
}

void removeVectorElement(vector_t *vector, int64_t index) {
    copyData(vector->data + index * vector->elementSize, vector->data + (index + 1) * vector->elementSize, (vector->length - index - 1) * vector->elementSize);
    setVectorLength(vector, vector->length - 1);
}

void pushVectorElement(vector_t *vector, void *source) {
    insertVectorElement(vector, vector->length, source);
}


