
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

#include "vector.h"
#include "scriptParse.h"

#define SCRIPT_VALUE_TYPE_MISSING 0
#define SCRIPT_VALUE_TYPE_NULL 1
#define SCRIPT_VALUE_TYPE_NUMBER 2
#define SCRIPT_VALUE_TYPE_STRING 3
#define SCRIPT_VALUE_TYPE_LIST 4
#define SCRIPT_VALUE_TYPE_FUNCTION 5

typedef struct scriptValue {
    int8_t type;
    // For null, data is unused.
    // For numbers, data is a double.
    // For strings and lists, data points to a scriptHeapValue_t.
    // For functions, data points to a scriptBaseFunction_t.
    int8_t data[(sizeof(double) > sizeof(int8_t *)) ? sizeof(double) : sizeof(int8_t *)];
} scriptValue_t;

typedef struct scriptHeapValue scriptHeapValue_t;

typedef struct scriptHeapValue {
    int8_t type;
    int8_t isMarked;
    int32_t lockDepth;
    scriptHeapValue_t *previous;
    scriptHeapValue_t *next;
    vector_t *data; // Vector of a string or list value.
} scriptHeapValue_t;

typedef struct keyBinding {
    int32_t key;
    scriptBaseFunction_t *callback;
} keyBinding_t;

typedef struct keyMapping {
    int32_t oldKey;
    int32_t newKey;
    int32_t mode;
} keyMapping_t;

typedef struct commandBinding {
    int8_t *commandName;
    scriptBaseFunction_t *callback;
} commandBinding_t;

scriptHeapValue_t *firstHeapValue;

scriptHeapValue_t *createScriptHeapValue();
scriptValue_t convertCharacterVectorToStringValue(vector_t vector);
scriptValue_t convertTextToStringValue(int8_t *text);

// SCRIPT_VALUE_HEADER_FILE
#endif
