
#ifndef TEXT_ALLOCATION_HEADER_FILE
#define TEXT_ALLOCATION_HEADER_FILE

typedef struct textAllocation {
    int8_t *text;
    int64_t length;
    int64_t allocationSize;
} textAllocation_t;

void setTextAllocationSize(textAllocation_t *allocation, int64_t size);
void insertTextIntoTextAllocation(textAllocation_t *allocation, int64_t index, int8_t *text, int64_t amount);
void removeTextFromTextAllocation(textAllocation_t *allocation, int64_t index, int64_t amount);
void cleanUpTextAllocation(textAllocation_t *allocation);

// TEXT_ALLOCATION_HEADER_FILE
#endif
