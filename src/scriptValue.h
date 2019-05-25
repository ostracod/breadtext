
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

#include "vector.h"

#define SCRIPT_VALUE_TYPE_MISSING 0
#define SCRIPT_VALUE_TYPE_NULL 1
#define SCRIPT_VALUE_TYPE_NUMBER 2
#define SCRIPT_VALUE_TYPE_STRING 3
#define SCRIPT_VALUE_TYPE_LIST 4
#define SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION 5
#define SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION 6

#define SCRIPT_BRANCH_TYPE_ROOT 1
#define SCRIPT_BRANCH_TYPE_IF 2
#define SCRIPT_BRANCH_TYPE_WHILE 3
#define SCRIPT_BRANCH_TYPE_FUNCTION 4
#define SCRIPT_BRANCH_TYPE_IMPORT 5

typedef struct scriptValue {
    int8_t type;
    // For null, data is unused.
    // For numbers, data is a double.
    // For strings, lists, and custom functions, data points to a scriptHeapValue_t.
    // For built-in functions, data points to a scriptBuiltInFunction_t.
    int8_t data[(sizeof(double) > sizeof(int8_t *)) ? sizeof(double) : sizeof(int8_t *)];
} scriptValue_t;

typedef struct scriptHeapValue scriptHeapValue_t;

typedef struct scriptHeapValue {
    int8_t type;
    int8_t isMarked;
    int32_t lockDepth;
    scriptHeapValue_t *previous;
    scriptHeapValue_t *next;
    // For strings and lists, data points to a vector_t.
    // For custom functions, data points to a scriptCustomFunction_t.
    void *data;
} scriptHeapValue_t;

typedef struct scriptVariable {
    int8_t *name;
    scriptValue_t value;
} scriptVariable_t;

typedef struct scriptScope {
    vector_t variableList;
} scriptScope_t;

typedef struct scriptBranch {
    int8_t type;
    int8_t shouldIgnore;
    int8_t hasExecuted;
    scriptBodyLine_t line;
    scriptBody_t *importScriptBody;
} scriptBranch_t;

typedef struct keyBinding {
    int32_t key;
    scriptValue_t callback;
} keyBinding_t;

typedef struct keyMapping {
    int32_t oldKey;
    int32_t newKey;
    int32_t mode;
} keyMapping_t;

typedef struct commandBinding {
    int8_t *commandName;
    scriptValue_t callback;
} commandBinding_t;

scriptHeapValue_t *firstHeapValue;

void deleteScriptCustomFunction(scriptCustomFunction_t *function);
scriptHeapValue_t *createScriptHeapValue();
void deleteScriptHeapValue(scriptHeapValue_t *value);
int8_t scriptValueIsInHeap(scriptValue_t *value);
void lockScriptValue(scriptValue_t *value);
void unlockScriptValue(scriptValue_t *value);
void unmarkScriptValue(scriptValue_t *value);
void unmarkScriptHeapValue(scriptHeapValue_t *value);
void unmarkScriptScope(scriptScope_t *scope);
void unmarkScriptBody(scriptBody_t *body);
scriptValue_t convertCharacterVectorToStringValue(vector_t vector);
scriptValue_t convertTextToStringValue(int8_t *text);
scriptValue_t convertScriptValueToString(scriptValue_t value);
scriptValue_t convertScriptValueToNumber(scriptValue_t value);
scriptScope_t *createEmptyScriptScope();
scriptScope_t *scriptBodyAddEmptyScope(scriptBody_t *body);
scriptVariable_t createEmptyScriptVariable(int8_t *name);
scriptVariable_t *scriptScopeAddVariable(scriptScope_t *scope, scriptVariable_t variable);
int64_t scriptScopeFindVariableIndexWithNameLength(scriptScope_t *scope, int8_t *name, int64_t length);
int64_t scriptScopeFindVariableIndex(scriptScope_t *scope, int8_t *name);
scriptVariable_t *scriptScopeFindVariable(scriptScope_t *scope, int8_t *name);
void cleanUpScriptVariable(scriptVariable_t *variable);
void deleteScriptScope(scriptScope_t *scope);
int8_t scriptValuesAreEqualShallow(scriptValue_t *value1, scriptValue_t *value2);
int8_t scriptValuesAreIdentical(scriptValue_t *value1, scriptValue_t *value2);
scriptValue_t copyScriptValue(scriptValue_t *value);

// SCRIPT_VALUE_HEADER_FILE
#endif
