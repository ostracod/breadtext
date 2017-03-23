
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
void highlightWord();
void highlightEnclosureExclusive();
void highlightEnclosureInclusive();

// SELECTION_HEADER_FILE
#endif
