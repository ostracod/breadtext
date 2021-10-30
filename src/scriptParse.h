
#ifndef SCRIPT_PARSE_HEADER_FILE
#define SCRIPT_PARSE_HEADER_FILE

#include "vector.h"
#include "scriptValue.h"

#define SCRIPT_OPERATOR_ARRANGEMENT_BINARY 1
#define SCRIPT_OPERATOR_ARRANGEMENT_UNARY_PREFIX 2
#define SCRIPT_OPERATOR_ARRANGEMENT_UNARY_POSTFIX 3

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

#define SCRIPT_FUNCTION_TYPE_BUILT_IN 1
#define SCRIPT_FUNCTION_TYPE_CUSTOM 2

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
#define SCRIPT_FUNCTION_PROMPT_CHAR 24
#define SCRIPT_FUNCTION_BIND_KEY 25
#define SCRIPT_FUNCTION_MAP_KEY 26
#define SCRIPT_FUNCTION_BIND_COMMAND 27
#define SCRIPT_FUNCTION_TEST_LOG 28
#define SCRIPT_FUNCTION_RAND 29
#define SCRIPT_FUNCTION_GET_TIMESTAMP 30
#define SCRIPT_FUNCTION_PRESS_KEYS 31
#define SCRIPT_FUNCTION_PUSH 32
#define SCRIPT_FUNCTION_POW 33
#define SCRIPT_FUNCTION_LOG 34
#define SCRIPT_FUNCTION_FILE_EXISTS 35
#define SCRIPT_FUNCTION_OPEN_FILE 36
#define SCRIPT_FUNCTION_GET_FILE_SIZE 37
#define SCRIPT_FUNCTION_SET_FILE_SIZE 38
#define SCRIPT_FUNCTION_READ_FILE 39
#define SCRIPT_FUNCTION_WRITE_FILE 40
#define SCRIPT_FUNCTION_GET_FILE_OFFSET 41
#define SCRIPT_FUNCTION_SET_FILE_OFFSET 42
#define SCRIPT_FUNCTION_CLOSE_FILE 43
#define SCRIPT_FUNCTION_DELETE_FILE 44
#define SCRIPT_FUNCTION_PRINT_TO_CONSOLE 45
#define SCRIPT_FUNCTION_PROMPT_FROM_CONSOLE 46

#define SCRIPT_EXPRESSION_TYPE_NULL 1
#define SCRIPT_EXPRESSION_TYPE_NUMBER 2
#define SCRIPT_EXPRESSION_TYPE_STRING 3
#define SCRIPT_EXPRESSION_TYPE_LIST 4
#define SCRIPT_EXPRESSION_TYPE_FUNCTION 5
#define SCRIPT_EXPRESSION_TYPE_IDENTIFIER 6
#define SCRIPT_EXPRESSION_TYPE_VARIABLE 7
#define SCRIPT_EXPRESSION_TYPE_UNARY 8
#define SCRIPT_EXPRESSION_TYPE_BINARY 9
#define SCRIPT_EXPRESSION_TYPE_INDEX 10
#define SCRIPT_EXPRESSION_TYPE_INVOCATION 11

#define SCRIPT_STATEMENT_TYPE_EXPRESSION 1
#define SCRIPT_STATEMENT_TYPE_IF 2
#define SCRIPT_STATEMENT_TYPE_WHILE 3
#define SCRIPT_STATEMENT_TYPE_BREAK 4
#define SCRIPT_STATEMENT_TYPE_CONTINUE 5
#define SCRIPT_STATEMENT_TYPE_RETURN 6
#define SCRIPT_STATEMENT_TYPE_IMPORT 7

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

typedef struct scriptConstant {
    int8_t *name;
    int32_t value;
} scriptConstant_t;

typedef struct scriptOperator {
    int8_t *text;
    int32_t number;
    int8_t arrangement;
    int8_t precedence;
} scriptOperator_t;

typedef struct scriptVariable {
    int8_t *name;
    int8_t isGlobal;
    int32_t scopeIndex;
} scriptVariable_t;

typedef struct scriptScope {
    // Function argument variables will preceed any other
    // variables in the scope. Argument variables will be
    // populated in the same order as provided by invocation.
    vector_t variableNameList; // Vector of pointer to int8_t.
} scriptScope_t;

typedef struct scriptScopePair {
    scriptScope_t *globalScope;
    scriptScope_t *localScope;
} scriptScopePair_t;

typedef struct scriptBaseFunction {
    int8_t type;
    int32_t argumentAmount;
} scriptBaseFunction_t;

typedef struct scriptBuiltInFunction {
    scriptBaseFunction_t base;
    int8_t *name;
    int32_t number;
} scriptBuiltInFunction_t;

typedef struct script script_t;

typedef struct scriptCustomFunction {
    scriptBaseFunction_t base;
    int8_t isEntryPoint;
    scriptScope_t scope;
    vector_t statementList; // Vector of pointers to scriptBaseStatement_t.
    script_t *script;
} scriptCustomFunction_t;

typedef struct scriptBaseExpression {
    int8_t type;
} scriptBaseExpression_t;

typedef struct scriptNumberExpression {
    scriptBaseExpression_t base;
    double value;
} scriptNumberExpression_t;

typedef struct scriptStringExpression {
    scriptBaseExpression_t base;
    vector_t text; // Vector of int8_t.
} scriptStringExpression_t;

typedef struct scriptListExpression {
    scriptBaseExpression_t base;
    vector_t expressionList; // Vector of pointers to scriptBaseExpression_t.
} scriptListExpression_t;

typedef struct scriptFunctionExpression {
    scriptBaseExpression_t base;
    scriptBaseFunction_t *function;
} scriptFunctionExpression_t;

typedef struct scriptIdentifierExpression {
    scriptBaseExpression_t base;
    int8_t *name;
} scriptIdentifierExpression_t;

typedef struct scriptVariableExpression {
    scriptBaseExpression_t base;
    scriptVariable_t variable;
} scriptVariableExpression_t;

typedef struct scriptUnaryExpression {
    scriptBaseExpression_t base;
    scriptOperator_t *operator;
    scriptBaseExpression_t *operand;
} scriptUnaryExpression_t;

typedef struct scriptBinaryExpression {
    scriptBaseExpression_t base;
    scriptOperator_t *operator;
    scriptBaseExpression_t *operand1;
    scriptBaseExpression_t *operand2;
} scriptBinaryExpression_t;

typedef struct scriptIndexExpression {
    scriptBaseExpression_t base;
    scriptBaseExpression_t *list;
    scriptBaseExpression_t *index;
} scriptIndexExpression_t;

typedef struct scriptInvocationExpression {
    scriptBaseExpression_t base;
    scriptBaseExpression_t *function;
    vector_t argumentList; // Vector of pointers to scriptBaseExpression_t.
} scriptInvocationExpression_t;

typedef struct scriptIfClause {
    scriptBodyLine_t scriptBodyLine;
    scriptBaseExpression_t *condition; // May be null for the final else clause.
    vector_t statementList; // Vector of pointers to scriptBaseStatement_t.
} scriptIfClause_t;

typedef struct scriptBaseStatement {
    int8_t type;
    scriptBodyLine_t scriptBodyLine;
} scriptBaseStatement_t;

typedef struct scriptExpressionStatement {
    scriptBaseStatement_t base;
    scriptBaseExpression_t *expression;
} scriptExpressionStatement_t;

typedef struct scriptIfStatement {
    scriptBaseStatement_t base;
    vector_t clauseList; // Vector of pointers to scriptIfClause_t.
} scriptIfStatement_t;

typedef struct scriptWhileStatement {
    scriptBaseStatement_t base;
    scriptBaseExpression_t *condition;
    vector_t statementList; // Vector of pointers to scriptBaseStatement_t.
} scriptWhileStatement_t;

typedef struct scriptReturnStatement {
    scriptBaseStatement_t base;
    scriptBaseExpression_t *expression; // May be null for no return value.
} scriptReturnStatement_t;

typedef struct scriptImportStatement {
    scriptBaseStatement_t base;
    scriptBaseExpression_t *path;
    vector_t variableList; // Vector of scriptVariable_t.
    // variableList will be populated in parseScriptStatement
    // (not in resolveScriptStatementIdentifiers).
} scriptImportStatement_t;

typedef struct script {
    scriptBody_t *scriptBody;
    scriptCustomFunction_t *entryPointFunction;
    vector_t customFunctionList; // Vector of pointers to scriptCustomFunction_t.
    scriptFrame_t globalFrame;
} script_t;

typedef struct scriptParser {
    script_t *script;
    scriptCustomFunction_t *function;
    vector_t *statementList; // Vector of pointers to scriptBaseStatement_t.
    scriptBodyLine_t *scriptBodyLine;
    vector_t *ifClauseList; // Vector of pointers to scriptIfClause_t.
    vector_t *importVariableList; // Vector of scriptVariable_t.
    int8_t isExpectingEndStatement;
    int8_t isInWhileLoop;
} scriptParser_t;

void initializeScriptParsingEnvironment();
int8_t loadScriptBody(scriptBody_t **destination, int8_t *path);
void loadScriptBodyFromText(scriptBody_t **destination, int8_t *text);
int8_t seekNextScriptBodyLine(scriptBodyLine_t *scriptBodyLine);
int8_t scriptBodyPosGetCharacter(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSkipWhitespace(scriptBodyPos_t *scriptBodyPos);
int8_t isFirstScriptIdentifierCharacter(int8_t character);
int8_t isScriptIdentifierCharacter(int8_t character);
int8_t isScriptNumberCharacter(int8_t character);
void scriptBodyPosSeekEndOfIdentifier(scriptBodyPos_t *scriptBodyPos);
void scriptBodyPosSeekEndOfNumber(scriptBodyPos_t *scriptBodyPos);
scriptOperator_t *scriptBodyPosGetOperator(scriptBodyPos_t *scriptBodyPos, int8_t operatorArrangement);
void scriptBodyPosSkipOperator(scriptBodyPos_t *scriptBodyPos, scriptOperator_t *operator);
int8_t scriptBodyPosTextMatchesIdentifier(scriptBodyPos_t *scriptBodyPos, int8_t *text);
int64_t getDistanceToScriptBodyPos(scriptBodyPos_t *startScriptBodyPos, scriptBodyPos_t *endScriptBodyPos);
int8_t *getScriptBodyPosPointer(scriptBodyPos_t *scriptBodyPos);
scriptBuiltInFunction_t *findScriptBuiltInFunctionByName(int8_t *name, int64_t length);
scriptConstant_t *getScriptConstantByName(int8_t *name, int64_t length);
int32_t scriptScopeFindVariable(scriptScope_t *scope, int8_t *name);
int8_t parseScriptBody(script_t **destination, scriptBody_t *scriptBody);

// SCRIPT_PARSE_HEADER_FILE
#endif


