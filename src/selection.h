
#ifndef SELECTION_HEADER_FILE
#define SELECTION_HEADER_FILE

vector_t internalClipboard;

int8_t *allocateStringFromSelection();
void copySelectionHelper();
void copySelection();
void deleteSelectionHelper();
void deleteSelection();
void cutSelection();
void getClipboardIndentationLevels(int32_t *headLevel, int32_t *tailBaseLevel, vector_t *clipboard);
void pasteBeforeCursorHelper(vector_t *clipboard, int8_t shouldIndentFirstLine);
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
