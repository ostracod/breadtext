
#ifndef TEXT_COMMAND_HEADER_FILE
#define TEXT_COMMAND_HEADER_FILE

#include "script.h"

int8_t compileRegexes();
void insertTextCommandCharacter(int8_t character);
void deleteTextCommandCharacter();
void executeTextCommandByTermList(scriptValue_t *destination, int8_t **termList, int32_t termListLength);
void executeTextCommand();
void enterBeginningOfCommand(int8_t *text);
void moveTextCommandCursorLeft();
void moveTextCommandCursorRight();
void pasteClipboardIntoTextCommand();

// TEXT_COMMAND_HEADER_FILE
#endif
