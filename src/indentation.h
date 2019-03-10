
#ifndef INDENTATION_HEADER_FILE
#define INDENTATION_HEADER_FILE

int8_t indentationWidth;
int8_t shouldUseHardTabs;

int32_t getTextIndentationLevel(int8_t *text, int64_t length);
int32_t getTextLineIndentationLevel(textLine_t *line);
int64_t getTextLineIndentationEndIndex(textLine_t *line);
int64_t getIndentationWidth(int64_t level);
int64_t setTextAllocationIndentationLevel(textAllocation_t *allocation, int32_t level);
void increaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory);
void increaseSelectionIndentationLevel();
void decreaseSelectionIndentationLevel();

// INDENTATION_HEADER_FILE
#endif
