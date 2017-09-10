
#ifndef SELECTION_HEADER_FILE
#define SELECTION_HEADER_FILE

int8_t copySelectionHelper();
void copySelection();
void deleteSelectionHelper();
void deleteSelection();
void cutSelection();
void pasteBeforeCursorHelper(FILE *file, int32_t baseIndentationLevel);
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

// SELECTION_HEADER_FILE
#endif
