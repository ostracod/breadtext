
#ifndef VECTOR_HEADER_FILE
#define VECTOR_HEADER_FILE

typedef struct vector {
    int64_t elementSize;
    int8_t *data;
    int64_t dataSize;
    int64_t length;
} vector_t;

void createVector(vector_t *destination, int64_t elementSize, int64_t length);
void createEmptyVector(vector_t *destination, int64_t elementSize);
void createVectorFromArray(vector_t *destination, int64_t elementSize, void *source, int64_t elementCount);
void copyVector(vector_t *destination, vector_t *source);
void cleanUpVector(vector_t *vector);
void setVectorLength(vector_t *vector, int64_t length);
void getVectorElement(void *destination, vector_t *vector, int64_t index);
void setVectorElement(vector_t *vector, int64_t index, void *source);
// Please use findVectorElement VERY carefully.
// The pointer will point to a bad location if the
// vector is resized.
void *findVectorElement(vector_t *vector, int64_t index);
void insertVectorElement(vector_t *vector, int64_t index, void *source);
void insertVectorElementArray(vector_t *vector, int64_t index, void *source, int64_t amount);
void removeVectorElement(vector_t *vector, int64_t index);
void pushVectorElement(vector_t *vector, void *source);
void pushVectorElementArray(vector_t *vector, void *source, int64_t amount);
void popVectorElement(void *destination, vector_t *vector);
void insertVectorIntoVector(vector_t *vector, int64_t index, vector_t *source);
void pushVectorOntoVector(vector_t *vector, vector_t *source);

// VECTOR_HEADER_FILE
#endif

