
#ifndef SYSTEMATIC_TEST_HEADER_FILE
#define SYSTEMATIC_TEST_HEADER_FILE

typedef struct namedKey {
    int32_t key;
    int8_t *name;
} namedKey_t;

int32_t convertNameToKey(int8_t *name);

// SYSTEMATIC_TEST_HEADER_FILE
#endif
