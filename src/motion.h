
#include <regex.h>
#include "breadtext.h"

#ifndef CURSOR_MOTION_HEADER_FILE
#define CURSOR_MOTION_HEADER_FILE

#define MARK_AMOUNT 6

typedef struct mark {
    int8_t isSet;
    textLine_t *textLine;
    int64_t characterIndex;
} mark_t;

int8_t searchTerm[1000];
int64_t searchTermLength;
regex_t searchRegexForward;
regex_t searchRegexBackward;
int8_t searchRegexIsEmpty;
int8_t searchTermIsRegex;
mark_t markList[MARK_AMOUNT];
int8_t isCaseSensitive;

void moveCursor(textPos_t *pos);
void moveCursorLeft(int32_t amount);
void moveCursorRight(int32_t amount);
void moveCursorUp(int32_t amount);
void moveCursorDown(int32_t amount);
void moveCursorLeftConsecutive();
void adjustLineSelectionBoundaries(textPos_t *nextCursorPos);
void moveLineSelectionUp(int32_t amount);
void moveLineSelectionDown(int32_t amount);
void moveCursorToBeginningOfLine();
void moveCursorToEndOfLine();
void moveCursorToEndOfIndentation();
void moveCursorToBeginningOfFile();
void moveCursorToEndOfFile();
void findNextTermInTextLine(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos);
void findPreviousTermInTextLine(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos);
textPos_t findNextWordInTextLine(int8_t *isMissing, textPos_t *pos);
textPos_t findPreviousWordInTextLine(int8_t *isMissing, textPos_t *pos);
void findNextTermTextPos(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos);
void findPreviousTermTextPos(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos);
textPos_t findNextWordTextPos(int8_t *isMissing, textPos_t *pos);
textPos_t findPreviousWordTextPos(int8_t *isMissing, textPos_t *pos);
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
void promptAndGoToCharacterExclusive();
void promptAndGoToCharacterInclusive();
void promptAndReverseGoToCharacterExclusive();
void promptAndReverseGoToCharacterInclusive();
int8_t goToCharacterExclusive(int8_t character);
int8_t goToCharacterInclusive(int8_t character);
int8_t reverseGoToCharacterExclusive(int8_t character);
int8_t reverseGoToCharacterInclusive(int8_t character);
void goToMatchingCharacter();

// CURSOR_MOTION_HEADER_FILE
#endif
