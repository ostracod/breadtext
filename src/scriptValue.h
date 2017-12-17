
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

#define SCRIPT_FUNCTION_IS_NUM 1
#define SCRIPT_FUNCTION_IS_STR 2
#define SCRIPT_FUNCTION_IS_LIST 3
#define SCRIPT_FUNCTION_IS_FUNC 4
#define SCRIPT_FUNCTION_COPY 5
#define SCRIPT_FUNCTION_STR 6
#define SCRIPT_FUNCTION_NUM 7
#define SCRIPT_FUNCTION_FLOOR 8
#define SCRIPT_FUNCTION_LEN 9
#define SCRIPT_FUNCTION_INS 10
#define SCRIPT_FUNCTION_REM 11
#define SCRIPT_FUNCTION_PRESS_KEY 12
#define SCRIPT_FUNCTION_GET_MODE 13
#define SCRIPT_FUNCTION_SET_MODE 14
#define SCRIPT_FUNCTION_GET_SELECTION_CONTENTS 15
#define SCRIPT_FUNCTION_GET_LINE_COUNT 16
#define SCRIPT_FUNCTION_GET_LINE_CONTENTS 17
#define SCRIPT_FUNCTION_GET_CURSOR_CHAR_INDEX 18
#define SCRIPT_FUNCTION_GET_CURSOR_LINE_INDEX 19
#define SCRIPT_FUNCTION_SET_CURSOR_POS 20
#define SCRIPT_FUNCTION_RUN_COMMAND 21
#define SCRIPT_FUNCTION_NOTIFY_USER 22
#define SCRIPT_FUNCTION_PROMPT_KEY 23
#define SCRIPT_FUNCTION_PROMPT_CHARACTER 24
#define SCRIPT_FUNCTION_BIND_KEY 25
#define SCRIPT_FUNCTION_BIND_COMMAND 26

#define SCRIPT_OPERATOR_TYPE_BINARY 1
#define SCRIPT_OPERATOR_TYPE_UNARY_PREFIX 2
#define SCRIPT_OPERATOR_TYPE_UNARY_POSTFIX 3

#define SCRIPT_OPERATOR_ASSIGN 1
#define SCRIPT_OPERATOR_ADD 2
#define SCRIPT_OPERATOR_ADD_ASSIGN 3
#define SCRIPT_OPERATOR_SUBTRACT 4
#define SCRIPT_OPERATOR_SUBTRACT_ASSIGN 5
#define SCRIPT_OPERATOR_NEGATE 6
#define SCRIPT_OPERATOR_MULTIPLY 7
#define SCRIPT_OPERATOR_MULTIPLY_ASSIGN 8
#define SCRIPT_OPERATOR_DIVIDE 9
#define SCRIPT_OPERATOR_DIVIDE_ASSIGN 10
#define SCRIPT_OPERATOR_MODULUS 11
#define SCRIPT_OPERATOR_MODULUS_ASSIGN 12
#define SCRIPT_OPERATOR_BOOLEAN_AND 13
#define SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN 14
#define SCRIPT_OPERATOR_BOOLEAN_OR 15
#define SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN 16
#define SCRIPT_OPERATOR_BOOLEAN_XOR 17
#define SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN 18
#define SCRIPT_OPERATOR_BOOLEAN_NOT 19
#define SCRIPT_OPERATOR_BITWISE_AND 20
#define SCRIPT_OPERATOR_BITWISE_AND_ASSIGN 21
#define SCRIPT_OPERATOR_BITWISE_OR 22
#define SCRIPT_OPERATOR_BITWISE_OR_ASSIGN 23
#define SCRIPT_OPERATOR_BITWISE_XOR 24
#define SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN 25
#define SCRIPT_OPERATOR_BITWISE_NOT 26
#define SCRIPT_OPERATOR_BITSHIFT_LEFT 27
#define SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN 28
#define SCRIPT_OPERATOR_BITSHIFT_RIGHT 29
#define SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN 30
#define SCRIPT_OPERATOR_GREATER 31
#define SCRIPT_OPERATOR_GREATER_OR_EQUAL 32
#define SCRIPT_OPERATOR_LESS 33
#define SCRIPT_OPERATOR_LESS_OR_EQUAL 34
#define SCRIPT_OPERATOR_EQUAL 35
#define SCRIPT_OPERATOR_NOT_EQUAL 36
#define SCRIPT_OPERATOR_IDENTICAL 37
#define SCRIPT_OPERATOR_NOT_IDENTICAL 38
#define SCRIPT_OPERATOR_INCREMENT_PREFIX 39
#define SCRIPT_OPERATOR_INCREMENT_POSTFIX 40
#define SCRIPT_OPERATOR_DECREMENT_PREFIX 41
#define SCRIPT_OPERATOR_DECREMENT_POSTFIX 42

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

typedef struct scriptBody {
    int8_t *path;
    int8_t *text;
    int64_t length;
    vector_t scopeStack;
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

typedef struct scriptOperator {
    int8_t *text;
    int32_t number;
    int8_t type;
    int8_t precedence;
} scriptOperator_t;

typedef struct scriptBuiltInFunction {
    int8_t *name;
    int32_t number;
    int32_t argumentAmount;
} scriptBuiltInFunction_t;

typedef struct customScriptFunction {
    scriptBodyLine_t scriptBodyLine;
} customScriptFunction_t;

typedef struct scriptBranch {
    int8_t type;
    int8_t shouldIgnore;
    int8_t hasExecuted;
    scriptBodyLine_t line;
    scriptBody_t *importScriptBody;
} scriptBranch_t;

int8_t loadScriptBody(scriptBody_t *destination, int8_t *path);
int8_t seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine);
int8_t scriptBodyPosGetCharacter(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSkipWhitespace(scriptBodyPos_t *scriptBodyPos);
int8_t isFirstScriptIdentifierCharacter(int8_t character);
int8_t isScriptIdentifierCharacter(int8_t character);
int8_t isScriptNumberCharacter(int8_t character);
void scriptBodyPosSeekEndOfIdentifier(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSeekEndOfNumber(scriptBodyPos_t *scriptBodyPos);
scriptOperator_t *scriptBodyPosGetOperator(scriptBodyPos_t *scriptBodyPos, int8_t operatorType);
void scriptBodyPosSkipOperator(scriptBodyPos_t *scriptBodyPos, scriptOperator_t *operator);
int8_t scriptBodyPosTextMatchesIdentifier(scriptBodyPos_t *scriptBodyPos, int8_t *text);
int64_t getDistanceToScriptBodyPos(scriptBodyPos_t *startScriptBodyPos, scriptBodyPos_t *endScriptBodyPos);
int8_t *getScriptBodyPosPointer(scriptBodyPos_t *scriptBodyPos);
scriptBuiltInFunction_t *findScriptBuiltInFunctionByName(int8_t *name, int64_t length);
scriptHeapValue_t *createScriptHeapValue();
scriptValue_t convertScriptValueToString(scriptValue_t value);
scriptValue_t convertScriptValueToNumber(scriptValue_t value);
scriptScope_t createEmptyScriptScope();
scriptScope_t *scriptBodyAddScope(scriptBody_t *body, scriptScope_t scope);
scriptVariable_t createEmptyScriptVariable(int8_t *name);
scriptVariable_t *scriptScopeAddVariable(scriptScope_t *scope, scriptVariable_t variable);
scriptVariable_t *scriptScopeFindVariableWithNameLength(scriptScope_t *scope, int8_t *name, int64_t length);
scriptVariable_t *scriptScopeFindVariable(scriptScope_t *scope, int8_t *name);
int8_t scriptValuesAreEqualShallow(scriptValue_t *value1, scriptValue_t *value2);
int8_t scriptValuesAreIdentical(scriptValue_t *value1, scriptValue_t *value2);
scriptValue_t copyScriptValue(scriptValue_t *value);

// SCRIPT_VALUE_HEADER_FILE
#endif
