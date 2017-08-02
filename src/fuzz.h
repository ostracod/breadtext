
#ifndef FUZZ_HEADER_FILE
#define FUZZ_HEADER_FILE

typedef struct fuzzKey {
    int32_t key;
    int8_t *name;
} fuzzKey_t;

void performFuzzTest();

// FUZZ_HEADER_FILE
#endif
