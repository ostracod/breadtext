
#ifndef SELECTION_HEADER_FILE
#define SELECTION_HEADER_FILE

void copySelectionHelper();
void copySelection();
void deleteSelectionHelper();
void deleteSelection();
void cutSelection();
void pasteBeforeCursorHelper(FILE *file, int32_t baseIndentationLevel);
void pasteBeforeCursor();
void pasteAfterCursor();

// SELECTION_HEADER_FILE
#endif
