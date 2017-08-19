
#ifndef TEXT_COMMAND_HEADER_FILE
#define TEXT_COMMAND_HEADER_FILE

int8_t compileRegexes();
void insertTextCommandCharacter(int8_t character);
void deleteTextCommandCharacter();
void executeTextCommand();
void enterBeginningOfCommand(int8_t *text);

// TEXT_COMMAND_HEADER_FILE
#endif
