
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "vector.h"

void createVector(vector_t *destination, int64_t elementSize, int64_t length) {
    destination->elementSize = elementSize;
    int64_t tempLength = (length < 2) ? 2 : length;
    destination->dataSize = elementSize * tempLength;
    destination->data = malloc(destination->dataSize);
    destination->length = length;
}

void createEmptyVector(vector_t *destination, int64_t elementSize) {
    createVector(destination, elementSize, 0);
}

void createVectorFromArray(vector_t *destination, int64_t elementSize, void *source, int64_t elementCount) {
    createVector(destination, elementSize, elementCount);
    copyData(destination->data, source, elementSize * elementCount);
}

void copyVector(vector_t *destination, vector_t *source) {
    createVectorFromArray(destination, source->elementSize, source->data, source->length);
}

void cleanUpVector(vector_t *vector) {
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
    copyData(vector->data + (index + 1) * vector->elementSize, vector->data + index * vector->elementSize, (vector->length - index - 1) * vector->elementSize);
    copyData(vector->data + index * vector->elementSize, source, vector->elementSize);
}

void removeVectorElement(vector_t *vector, int64_t index) {
    copyData(vector->data + index * vector->elementSize, vector->data + (index + 1) * vector->elementSize, (vector->length - index - 1) * vector->elementSize);
    setVectorLength(vector, vector->length - 1);
}

void pushVectorElement(vector_t *vector, void *source) {
    insertVectorElement(vector, vector->length, source);
}

void popVectorElement(void *destination, vector_t *vector) {
    int64_t index = vector->length - 1;
    getVectorElement(destination, vector, index);
    removeVectorElement(vector, index);
}

void insertVectorElementArray(vector_t *vector, int64_t index, void *source, int64_t amount) {
    setVectorLength(vector, vector->length + amount);
    copyData(vector->data + (index + amount) * vector->elementSize, vector->data + index * vector->elementSize, (vector->length - index - amount) * vector->elementSize);
    copyData(vector->data + index * vector->elementSize, source, vector->elementSize * amount);
}

void insertVectorIntoVector(vector_t *vector, int64_t index, vector_t *source) {
    insertVectorElementArray(vector, vector->length, source->data, source->length);
}

void pushVectorElementArray(vector_t *vector, void *source, int64_t amount) {
    insertVectorElementArray(vector, vector->length, source, amount);
}

void pushVectorOntoVector(vector_t *vector, vector_t *source) {
    insertVectorIntoVector(vector, vector->length, source);
}


