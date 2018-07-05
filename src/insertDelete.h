
#include "history.h"

#ifndef INSERT_DELETE_HEADER_FILE
#define INSERT_DELETE_HEADER_FILE

int8_t isStartOfNonconsecutiveEscapeSequence;
int8_t lastIsStartOfNonconsecutiveEscapeSequence;
historyFrame_t nonconsecutiveEscapeSequenceFrame;
int8_t nonconsecutiveEscapeSequenceFrameIsSet;

void insertCharacterBeforeCursor(int8_t character, int8_t isConsecutive);
void insertTextEntryModeCharacterBeforeCursor(int8_t character);
void insertTextReplaceModeCharacter(int8_t character);
void deleteCharacterBeforeCursor(int8_t shouldRecordHistory);
void deleteCharacterAfterCursor(int8_t shouldAddHistoryFrame);
void replaceCharacterAtCursor(int8_t character, int8_t shouldRecordHistory);
void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel, int8_t shouldRecordHistory);
void insertNewlineBeforeCursor(int8_t shouldIgnoreIndentation);
void promptAndInsertCharacterBeforeCursor();
void promptAndInsertCharacterAfterCursor();
void promptAndReplaceCharacterUnderCursor();
void insertCharacterBeforeCursorSimple(int8_t character);
void insertCharacterAfterCursorSimple(int8_t character);
void replaceCharacterUnderCursorSimple(int8_t character);
void insertLineBeforeCursor();
void insertLineAfterCursor();
void insertAndEditLineBeforeCursor();
void insertAndEditLineAfterCursor();
void deleteUntilBeginningOfLineInclusive();
void deleteUntilBeginningOfLineExclusive();
void deleteUntilEndOfLineInclusive();
void deleteUntilEndOfLineExclusive();
void joinCurrentLineToPreviousLine();
void joinCurrentLineToNextLine();

// INSERT_DELETE_HEADER_FILE
#endif
