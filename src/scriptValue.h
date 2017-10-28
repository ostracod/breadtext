
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

#define SCRIPT_VALUE_TYPE_MISSING 0
#define SCRIPT_VALUE_TYPE_NULL 1
#define SCRIPT_VALUE_TYPE_NUMBER 2
#define SCRIPT_VALUE_TYPE_STRING 3
#define SCRIPT_VALUE_TYPE_LIST 4
#define SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION 5
#define SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION 6

#define IS_NUM 1
#define IS_STR 2
#define IS_LIST 3
#define IS_FUNC 4
#define COPY 5
#define STR 6
#define NUM 7
#define FLOOR 8
#define LEN 9
#define INS 10
#define REM 11
#define PRESS_KEY 12
#define GET_MODE 13
#define SET_MODE 14
#define GET_SELECTION_CONTENTS 15
#define GET_LINE_COUNT 16
#define GET_LINE_CONTENTS 17
#define GET_CURSOR_CHAR_INDEX 18
#define GET_CURSOR_LINE_INDEX 19
#define SET_CURSOR_POS 20
#define RUN_COMMAND 21
#define NOTIFY_USER 22
#define PROMPT_KEY 23
#define PROMPT_CHARACTER 24
#define BIND_KEY 25
#define BIND_COMMAND 26

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

typedef struct scriptBuiltInFunction {
    int8_t *name;
    int32_t number;
    int32_t argumentAmount;
} scriptBuiltInFunction_t;

typedef struct customScriptFunction {
    scriptBodyLine_t scriptBodyLine;
} customScriptFunction_t;

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

int8_t loadScriptBody(scriptBody_t *destination, int8_t *path);
void seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine);
int8_t scriptBodyPosGetCharacter(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSkipWhitespace(scriptBodyPos_t *scriptBodyPos);
int8_t isFirstScriptIdentifierCharacter(int8_t character);
int8_t isScriptIdentifierCharacter(int8_t character);
int8_t isScriptNumberCharacter(int8_t character);
void scriptBodyPosSeekEndOfIdentifier(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSeekEndOfNumber(scriptBodyPos_t *scriptBodyPos);
int64_t getDistanceToScriptBodyPos(scriptBodyPos_t *startScriptBodyPos, scriptBodyPos_t *endScriptBodyPos);
int8_t *getScriptBodyPosPointer(scriptBodyPos_t *scriptBodyPos);
scriptBuiltInFunction_t *findScriptBuiltInFunctionByName(int8_t *name, int64_t length);
scriptHeapValue_t *createScriptHeapValue();
scriptValue_t convertScriptValueToString(scriptValue_t value);

// SCRIPT_VALUE_HEADER_FILE
#endif
