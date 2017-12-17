
#ifndef SCRIPT_HEADER_FILE
#define SCRIPT_HEADER_FILE

#include "scriptValue.h"

void initializeScriptingEnvironment();
int8_t runScript(int8_t *path);
int8_t invokeKeyBinding(int32_t key);
int32_t invokeKeyMapping(int32_t key);

// SCRIPT_HEADER_FILE
#endif



