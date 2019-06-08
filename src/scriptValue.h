
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

typedef struct scriptValue {
    int8_t type;
    // For null, data is unused.
    // For numbers, data is a double.
    // For strings and lists, data is a pointer to scriptHeapValue_t.
    // For functions, data is a pointer to scriptBaseFunction_t.
    int8_t data[(sizeof(double) > sizeof(int8_t *)) ? sizeof(double) : sizeof(int8_t *)];
} scriptValue_t;

typedef struct scriptFrame scriptFrame_t;

typedef struct scriptFrame {
    scriptFrame_t *previous;
    scriptFrame_t *next;
    // The order of values corresponds to that
    // of the scope of the invoked function.
    scriptValue_t *valueList; // Array of scriptValue_t.
    int32_t valueAmount;
} scriptFrame_t;

#include "vector.h"
#include "scriptParse.h"

#define SCRIPT_VALUE_TYPE_MISSING 0
#define SCRIPT_VALUE_TYPE_NULL 1
#define SCRIPT_VALUE_TYPE_NUMBER 2
#define SCRIPT_VALUE_TYPE_STRING 3
#define SCRIPT_VALUE_TYPE_LIST 4
#define SCRIPT_VALUE_TYPE_FUNCTION 5

typedef struct scriptHeapValue scriptHeapValue_t;

typedef struct scriptHeapValue {
    int8_t type;
    int8_t isMarked;
    int32_t lockDepth;
    int64_t referenceCount;
    scriptHeapValue_t *previous;
    scriptHeapValue_t *next;
    // For strings, data is a vector of int8_t.
    // For lists, data is a vector of scriptValue_t.
    vector_t data;
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
scriptFrame_t *firstScriptFrame;

scriptHeapValue_t *createScriptHeapValue();
void deleteScriptHeapValue(scriptHeapValue_t *value);
int8_t scriptValueIsInHeap(scriptValue_t *value);
void addScriptFrame(scriptFrame_t *frame);
void removeScriptFrame(scriptFrame_t *frame);
void deleteScriptHeapValueIfUnreferenced(scriptHeapValue_t *value);
void lockScriptValue(scriptValue_t *value);
void unlockScriptValue(scriptValue_t *value);
void addScriptValueReference(scriptValue_t *value);
void removeScriptValueReference(scriptValue_t *value);
void swapScriptValueReference(scriptValue_t *destination, scriptValue_t *source);
void unmarkScriptValue(scriptValue_t *value);
void unmarkScriptHeapValue(scriptHeapValue_t *value);
void unmarkScriptFrame(scriptFrame_t *frame);
scriptValue_t convertCharacterVectorToStringValue(vector_t vector);
scriptValue_t convertTextToStringValue(int8_t *text);
scriptValue_t convertScriptValueToString(scriptValue_t value);
scriptValue_t convertScriptValueToNumber(scriptValue_t value);
int8_t scriptValuesAreEqualShallow(scriptValue_t *value1, scriptValue_t *value2);
int8_t scriptValuesAreIdentical(scriptValue_t *value1, scriptValue_t *value2);
scriptValue_t copyScriptValue(scriptValue_t *value);

// SCRIPT_VALUE_HEADER_FILE
#endif
