
#ifndef INDENTATION_HEADER_FILE
#define INDENTATION_HEADER_FILE

int8_t indentationWidth;
int8_t shouldUseHardTabs;

int32_t getTextLineIndentationLevel(textLine_t *line);
int64_t getTextLineIndentationEndIndex(textLine_t *line);
int64_t getIndentationWidth(int64_t level);
void increaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory);
void increaseSelectionIndentationLevel();
void decreaseSelectionIndentationLevel();

// INDENTATION_HEADER_FILE
#endif
