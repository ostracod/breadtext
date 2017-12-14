
#ifndef VECTOR_HEADER_FILE
#define VECTOR_HEADER_FILE

typedef struct vector {
    int64_t elementSize;
    int8_t *data;
    int64_t dataSize;
    int64_t length;
} vector_t;

void createEmptyVector(vector_t *destination, int64_t elementSize);
void createVectorFromArray(vector_t *destination, int64_t elementSize, void *source, int64_t elementCount);
void deleteVector(vector_t *vector);
void setVectorLength(vector_t *vector, int64_t length);
void getVectorElement(void *destination, vector_t *vector, int64_t index);
void setVectorElement(vector_t *vector, int64_t index, void *source);
void *findVectorElement(vector_t *vector, int64_t index);
void insertVectorElement(vector_t *vector, int64_t index, void *source);
void removeVectorElement(vector_t *vector, int64_t index);
void pushVectorElement(vector_t *vector, void *source);
void popVectorElement(void *destination, vector_t *vector);

// VECTOR_HEADER_FILE
#endif

