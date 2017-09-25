
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

#define SCRIPT_VALUE_TYPE_NULL 0
#define SCRIPT_VALUE_TYPE_NUMBER 1
#define SCRIPT_VALUE_TYPE_STRING 2
#define SCRIPT_VALUE_TYPE_LIST 3
#define SCRIPT_VALUE_TYPE_FUNCTION 4

#define SCRIPT_FUNCTION_TYPE_BUILT_IN 1
#define SCRIPT_FUNCTION_TYPE_CUSTOM 2

typedef struct scriptBody {
    int8_t *path;
    int8_t *text;
    int64_t length;
} scriptBody_t;

typedef struct scriptBodyLine {
    scriptBody_t *scriptBody;
    int64_t index;
    int64_t number;
} scriptBodyLine_t;

typedef struct scriptBodyPos {
    scriptBodyLine_t *scriptBodyLine;
    int64_t index;
} scriptBodyPos_t;

typedef struct builtInScriptFunction {
    int8_t *name;
    int32_t number;
    int32_t argumentAmount;
} builtInScriptFunction_t;

typedef struct customScriptFunction {
    scriptBodyLine_t scriptBodyLine;
} customScriptFunction_t;

typedef struct scriptFunction {
    int8_t type;
    // For built-in functions, data points to a builtInScriptFunction_t.
    // For custom functions, data points to a customScriptFunction_t.
    void *data;
} scriptFunction_t;

typedef struct scriptValue {
    int8_t type;
    // For null, data is unused.
    // For numbers, data is a double.
    // For strings, lists, and functions, data points to a scriptHeapValue_t.
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
    // For functions, data points to a scriptFunction_t.
    void *data;
} scriptHeapValue_t;

int8_t loadScriptBody(scriptBody_t *destination, int8_t *path);
void seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine);
int8_t scriptBodyPosGetCharacter(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSkipWhitespace(scriptBodyPos_t *scriptBodyPos);
int8_t isFirstScriptIdentifierCharacter(int8_t character);
int8_t isScriptIdentifierCharacter(int8_t character);
void scriptBodyPosSeekEndOfIdentifier(scriptBodyPos_t *scriptBodyPos);

// SCRIPT_VALUE_HEADER_FILE
#endif
