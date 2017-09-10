
#ifndef FUZZ_HEADER_FILE
#define FUZZ_HEADER_FILE

typedef struct fuzzKey {
    int32_t key;
    int8_t *name;
} fuzzKey_t;

void runFuzzTest();
void handleSegmentationFault(int signum);

// FUZZ_HEADER_FILE
#endif
