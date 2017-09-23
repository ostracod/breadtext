
#ifndef SCRIPT_VALUE_HEADER_FILE
#define SCRIPT_VALUE_HEADER_FILE

typedef struct scriptBody {
    int8_t *path;
    int8_t *text;
    int64_t length;
} scriptBody_t;

typedef struct scriptBodyLine {
    scriptBody_t *scriptBody;
    int64_t index;
    int64_t number;
} scriptBodyPos_t;

scriptBody_t *loadScriptBody(int8_t *path);

// SCRIPT_VALUE_HEADER_FILE
#endif
