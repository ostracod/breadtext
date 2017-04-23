
#include "breadtext.h"

#ifndef CURSOR_MOTION_HEADER_FILE
#define CURSOR_MOTION_HEADER_FILE

#define MARK_AMOUNT 10

int8_t searchTerm[1000];
int64_t searchTermLength;
textLine_t *markList[MARK_AMOUNT];
int8_t markIsSetList[MARK_AMOUNT];

void moveCursor(textPos_t *pos);
void moveCursorLeft(int32_t amount);
void moveCursorRight(int32_t amount);
void moveCursorUp(int32_t amount);
void moveCursorDown(int32_t amount);
void adjustLineSelectionBoundaries(textPos_t *nextCursorPos);
void moveLineSelectionUp(int32_t amount);
void moveLineSelectionDown(int32_t amount);
void moveCursorToBeginningOfLine();
void moveCursorToEndOfLine();
void moveCursorToBeginningOfFile();
void moveCursorToEndOfFile();
textPos_t findNextTermInTextLine(textPos_t *pos, int8_t *isMissing);
textPos_t findPreviousTermInTextLine(textPos_t *pos, int8_t *isMissing);
textPos_t findNextWordInTextLine(textPos_t *pos, int8_t *isMissing);
textPos_t findPreviousWordInTextLine(textPos_t *pos, int8_t *isMissing);
textPos_t findNextTermTextPos(textPos_t *pos, int8_t *isMissing);
textPos_t findPreviousTermTextPos(textPos_t *pos, int8_t *isMissing);
textPos_t findNextWordTextPos(textPos_t *pos, int8_t *isMissing);
textPos_t findPreviousWordTextPos(textPos_t *pos, int8_t *isMissing);
int8_t gotoNextTermHelper();
int8_t gotoPreviousTermHelper();
int8_t gotoNextWordHelper();
int8_t gotoPreviousWordHelper();
void gotoNextTerm();
void gotoPreviousTerm();
void gotoNextWord();
void gotoPreviousWord();
void setMark(int64_t index);
void gotoMark(int64_t index);
void findNextTermUnderCursor();
void findPreviousTermUnderCursor();
void moveTextUp(int32_t amount);
void moveTextDown(int32_t amount);
void moveCursorToVisibleText();
void goToCharacterExclusive();
void goToCharacterInclusive();
void reverseGoToCharacterExclusive();
void reverseGoToCharacterInclusive();

// CURSOR_MOTION_HEADER_FILE
#endif
