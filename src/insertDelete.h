
#include "history.h"

#ifndef INSERT_DELETE_HEADER_FILE
#define INSERT_DELETE_HEADER_FILE

int8_t isStartOfNonconsecutiveEscapeSequence;
int8_t lastIsStartOfNonconsecutiveEscapeSequence;
historyAction_t firstNonconsecutiveEscapeSequenceAction;
historyTextPos_t nonconsecutiveEscapeSequencePreviousCursorTextPos;

void insertCharacterBeforeCursor(int8_t character);
void deleteCharacterBeforeCursor(int8_t shouldRecordHistory);
void deleteCharacterAfterCursor(int8_t shouldAddHistoryFrame);
void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel);
void insertNewlineBeforeCursor();
void promptAndInsertCharacterBeforeCursor();
void promptAndInsertCharacterAfterCursor();
void promptAndReplaceCharacterUnderCursor();

// INSERT_DELETE_HEADER_FILE
#endif
