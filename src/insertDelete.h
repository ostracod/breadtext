
#include "history.h"

#ifndef INSERT_DELETE_HEADER_FILE
#define INSERT_DELETE_HEADER_FILE

int8_t isStartOfNonconsecutiveEscapeSequence;
historyAction_t firstNonconsecutiveEscapeSequenceAction;
historyTextPos_t nonconsecutiveEscapeSequencePreviousCursorTextPos;

void insertCharacterUnderCursor(int8_t character);
void deleteCharacterBeforeCursor(int8_t shouldRecordHistory);
void deleteCharacterAfterCursor();
void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel);
void insertNewlineBeforeCursor();

// INSERT_DELETE_HEADER_FILE
#endif
