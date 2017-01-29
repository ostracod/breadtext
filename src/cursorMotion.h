
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
textPos_t findNextTermTextPos(textPos_t *pos, int8_t *isMissing);
textPos_t findPreviousTermTextPos(textPos_t *pos, int8_t *isMissing);
int8_t gotoNextTermHelper();
int8_t gotoPreviousTermHelper();
void gotoNextTerm();
void gotoPreviousTerm();
void setMark(int64_t index);
void gotoMark(int64_t index);

// CURSOR_MOTION_HEADER_FILE
#endif
