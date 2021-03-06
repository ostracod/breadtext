
#ifndef SCRIPT_HEADER_FILE
#define SCRIPT_HEADER_FILE

#include "vector.h"
#include "scriptValue.h"
#include "scriptParse.h"

int8_t scriptHasError;
scriptBodyLine_t scriptErrorLine;
int8_t scriptErrorHasLine;
vector_t scriptTestLogMessageList;

void initializeScriptingEnvironment();
void reportScriptError(int8_t *message, scriptBodyLine_t *line);
int8_t runScript(int8_t *path);
int8_t runScriptAsText(int8_t *text);
int8_t invokeKeyBinding(int32_t key);
int32_t invokeKeyMapping(int32_t key);
int8_t invokeCommandBinding(scriptValue_t *destination, int8_t **termList, int32_t termListLength);

// SCRIPT_HEADER_FILE
#endif



