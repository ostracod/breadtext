
#include "textAllocation.h"

#ifndef SYNTAX_HEADER_FILE
#define SYNTAX_HEADER_FILE

int8_t *commentPrefix;
int8_t **keywordList;
int32_t keywordListLength;
int8_t *syntaxDirectoryPath;

void updateSyntaxDefinition();
void generateSyntaxHighlighting(textAllocation_t *allocation);

// SYNTAX_HEADER_FILE
#endif

