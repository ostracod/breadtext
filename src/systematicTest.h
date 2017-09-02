
#ifndef SYSTEMATIC_TEST_HEADER_FILE
#define SYSTEMATIC_TEST_HEADER_FILE

typedef struct namedKey {
    int32_t key;
    int8_t *name;
} namedKey_t;

int8_t *systematicTestDefinitionPath;
int32_t systematicTestKey;

int32_t convertNameToKey(int8_t *name);
int8_t runSystematicTest();

// SYSTEMATIC_TEST_HEADER_FILE
#endif
