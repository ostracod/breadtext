
#ifndef SELECTION_HEADER_FILE
#define SELECTION_HEADER_FILE

int8_t *internalClipboard;
int64_t internalClipboardSize;

int8_t *allocateStringFromSelection();
int8_t copySelectionHelper();
void copySelection();
void deleteSelectionHelper();
void deleteSelection();
void cutSelection();
int32_t getClipboardBaseIndentationLevel(vector_t *systemClipboard);
void pasteBeforeCursorHelper(vector_t *systemClipboard, int8_t shouldIndentFirstLine);
void pasteBeforeCursor();
void pasteAfterCursor();
void swapSelection();
void highlightWord();
void promptAndHighlightWordByDelimiter();
void highlightWordByDelimiter(int8_t character);
void promptCharacterAndHighlightEnclosureExclusive();
void promptCharacterAndHighlightEnclosureInclusive();
void highlightEnclosureExclusive(int8_t character);
void highlightEnclosureInclusive(int8_t character);
void highlightLineContents();
void changeLine();
void selectUntilBeginningOfLineInclusive();
void selectUntilBeginningOfLineExclusive();
void selectUntilEndOfLineInclusive();
void selectUntilEndOfLineExclusive();
void fixLineSelection();

// SELECTION_HEADER_FILE
#endif
