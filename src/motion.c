
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "motion.h"
#include "insertDelete.h"
#include "breadtext.h"

void moveCursor(textPos_t *pos) {
    if (activityMode == HIGHLIGHT_STATIC_MODE) {
        setActivityMode(COMMAND_MODE);
    }
    historyFrameIsConsecutive = false;
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceAction(true);
    }
    textPos_t tempPreviousTextPos = cursorTextPos;
    textPos_t tempNextPos = *pos;
    if (!equalTextPos(&tempNextPos, &tempPreviousTextPos)) {
        if (activityMode == HIGHLIGHT_LINE_MODE) {
            adjustLineSelectionBoundaries(&tempNextPos);
        }
        if (tempNextPos.line != tempPreviousTextPos.line) {
            eraseLineNumber();
            cursorTextPos.line = tempNextPos.line;
            displayLineNumber();
        }
        cursorTextPos.row = tempNextPos.row;
        cursorTextPos.column = tempNextPos.column;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (isHighlighting) {
                textLine_t *tempLine = tempPreviousTextPos.line;
                displayTextLine(getTextLinePosY(tempLine), tempLine);
                while (textLineIsAfterTextLine(tempLine, cursorTextPos.line)) {
                    tempLine = getPreviousTextLine(tempLine);
                    displayTextLine(getTextLinePosY(tempLine), tempLine);
                }
                while (textLineIsAfterTextLine(cursorTextPos.line, tempLine)) {
                    tempLine = getNextTextLine(tempLine);
                    displayTextLine(getTextLinePosY(tempLine), tempLine);
                }
                
            } else {
                cursorTextPos = tempPreviousTextPos;
                eraseCursor();
                cursorTextPos = tempNextPos;
                displayCursor();
            }
        }
    }
}

void moveCursorLeft(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        if (tempNextTextPos.row <= 0 && tempNextTextPos.column <= 0) {
            textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            setTextPosIndex(&tempNextTextPos, tempLength);
        } else if (tempNextTextPos.column <= 0) {
            tempNextTextPos.column = viewPortWidth - 1;
            tempNextTextPos.row -= 1;
        } else {
            tempNextTextPos.column -= 1;
        }
        tempCount += 1;
    }
    cursorSnapColumn = tempNextTextPos.column;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorRight(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        int64_t tempLength = tempNextTextPos.line->textAllocation.length;
        if (tempNextTextPos.row * viewPortWidth + tempNextTextPos.column >= tempLength) {
            textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            tempNextTextPos.line = tempLine;
            tempNextTextPos.column = 0;
            tempNextTextPos.row = 0;
        } else if (tempNextTextPos.column >= viewPortWidth - 1) {
            tempNextTextPos.column = 0;
            tempNextTextPos.row += 1;
        } else {
            tempNextTextPos.column += 1;
        }
        tempCount += 1;
    }
    cursorSnapColumn = tempNextTextPos.column;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorUp(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        if (tempNextTextPos.row <= 0) {
            textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            int64_t tempColumn = tempLength % viewPortWidth;
            if (tempNextTextPos.column > tempColumn) {
                tempNextTextPos.column = tempColumn;
            }
            tempNextTextPos.row = tempLength / viewPortWidth;
        } else {
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.row -= 1;
        }
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorDown(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        int64_t tempRowCount = getTextLineRowCount(tempNextTextPos.line);
        if (tempNextTextPos.row >= tempRowCount - 1) {
            textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            int64_t tempRowCount2 = getTextLineRowCount(tempNextTextPos.line);
            if (tempRowCount2 <= 1) {
                int64_t tempColumn = tempLength % viewPortWidth;
                if (tempNextTextPos.column > tempColumn) {
                    tempNextTextPos.column = tempColumn;
                }
            }
            tempNextTextPos.row = 0;
        } else {
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.row += 1;
            if (tempNextTextPos.row >= tempRowCount - 1) {
                int64_t tempLength = tempNextTextPos.line->textAllocation.length;
                int64_t tempColumn = tempLength % viewPortWidth;
                if (tempNextTextPos.column > tempColumn) {
                    tempNextTextPos.column = tempColumn;
                }
            }
        }
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void adjustLineSelectionBoundaries(textPos_t *nextCursorPos) {
    if (textLineIsAfterTextLine(nextCursorPos->line, highlightTextPos.line)) {
        highlightTextPos.row = 0;
        highlightTextPos.column = 0;
        int64_t tempLength = nextCursorPos->line->textAllocation.length;
        setTextPosIndex(nextCursorPos, tempLength);
    } else {
        nextCursorPos->row = 0;
        nextCursorPos->column = 0;
        int64_t tempLength = highlightTextPos.line->textAllocation.length;
        setTextPosIndex(&highlightTextPos, tempLength);
    }
}

void moveLineSelectionUp(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
        if (tempLine == NULL) {
            break;
        }
        tempNextTextPos.line = tempLine;
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveLineSelectionDown(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
        if (tempLine == NULL) {
            break;
        }
        tempNextTextPos.line = tempLine;
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToBeginningOfLine() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfLine() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t tempLength = tempNextTextPos.line->textAllocation.length;
    setTextPosIndex(&tempNextTextPos, tempLength);
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

void moveCursorToBeginningOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getLeftmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getRightmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

textPos_t findNextTermInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    while (index <= tempLength - searchTermLength) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempTextPos;
            tempTextPos.line = pos->line;
            setTextPosIndex(&tempTextPos, index);
            *isMissing = false;
            return tempTextPos;
        }
        index += 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findPreviousTermInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (index > tempLength - searchTermLength) {
        index = tempLength - searchTermLength;
    }
    while (index >= 0) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempTextPos;
            tempTextPos.line = pos->line;
            setTextPosIndex(&tempTextPos, index);
            *isMissing = false;
            return tempTextPos;
        }
        index -= 1;
    }
    *isMissing = true;
    return *pos;
}

int8_t wordAtTextPosHasLength(textPos_t *pos, int64_t length) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    int64_t tempPreviousIndex = index;
    while (index >= 0) {
        int8_t tempCharacter = pos->line->textAllocation.text[index];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempPreviousIndex = index;
        index -= 1;
    }
    index = tempPreviousIndex;
    int64_t tempWordLength = 0;
    while (index < tempLength) {
        int8_t tempCharacter = pos->line->textAllocation.text[index];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }        
        tempWordLength += 1;
        index += 1;
    }
    return (length == tempWordLength);
}

textPos_t findNextWordInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    while (index <= tempLength - searchTermLength) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempTextPos;
            tempTextPos.line = pos->line;
            setTextPosIndex(&tempTextPos, index);
            if (wordAtTextPosHasLength(&tempTextPos, searchTermLength)) {
                *isMissing = false;
                return tempTextPos;
            }
        }
        index += 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findPreviousWordInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (index > tempLength - searchTermLength) {
        index = tempLength - searchTermLength;
    }
    while (index >= 0) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempTextPos;
            tempTextPos.line = pos->line;
            setTextPosIndex(&tempTextPos, index);
            if (wordAtTextPosHasLength(&tempTextPos, searchTermLength)) {
                *isMissing = false;
                return tempTextPos;
            }
        }
        index -= 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findNextTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findNextTermInTextLine(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getNextTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            tempTextPos.row = 0;
            tempTextPos.column = 0;
        } else {
            *isMissing = false;
            return tempTextPos2;
        }
    }
}

textPos_t findPreviousTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findPreviousTermInTextLine(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getPreviousTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
        } else {
            *isMissing = false;
            return tempTextPos2;
        }
    }
}

textPos_t findNextWordTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findNextWordInTextLine(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getNextTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            tempTextPos.row = 0;
            tempTextPos.column = 0;
        } else {
            *isMissing = false;
            return tempTextPos2;
        }
    }
}

textPos_t findPreviousWordTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findPreviousWordInTextLine(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getPreviousTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
        } else {
            *isMissing = false;
            return tempTextPos2;
        }
    }
}

int8_t gotoNextTermHelper() {
    moveCursorRight(1);
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos = findNextTermTextPos(&cursorTextPos, &tempIsMissing);
    if (tempIsMissing) {
        tempTextPos.line = getLeftmostTextLine(rootTextLine);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        tempTextPos = findNextTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            moveCursorLeft(1);
            return false;
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

int8_t gotoPreviousTermHelper() {
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    if (getTextLineNumber(cursorTextPos.line) <= 1 && cursorTextPos.row == 0 && cursorTextPos.column == 0) {
        tempTextPos.line = getRightmostTextLine(rootTextLine);
        int64_t tempLength = tempTextPos.line->textAllocation.length;
        setTextPosIndex(&tempTextPos, tempLength);
        tempTextPos = findPreviousTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            return false;
        }
    } else {
        moveCursorLeft(1);
        tempTextPos = findPreviousTermTextPos(&cursorTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getRightmostTextLine(rootTextLine);
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
            tempTextPos = findPreviousTermTextPos(&tempTextPos, &tempIsMissing);
            if (tempIsMissing) {
                moveCursorRight(1);
                return false;
            }
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

int8_t gotoNextWordHelper() {
    moveCursorRight(1);
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos = findNextWordTextPos(&cursorTextPos, &tempIsMissing);
    if (tempIsMissing) {
        tempTextPos.line = getLeftmostTextLine(rootTextLine);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        tempTextPos = findNextWordTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            moveCursorLeft(1);
            return false;
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

int8_t gotoPreviousWordHelper() {
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    if (getTextLineNumber(cursorTextPos.line) <= 1 && cursorTextPos.row == 0 && cursorTextPos.column == 0) {
        tempTextPos.line = getRightmostTextLine(rootTextLine);
        int64_t tempLength = tempTextPos.line->textAllocation.length;
        setTextPosIndex(&tempTextPos, tempLength);
        tempTextPos = findPreviousWordTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            return false;
        }
    } else {
        moveCursorLeft(1);
        tempTextPos = findPreviousWordTextPos(&cursorTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getRightmostTextLine(rootTextLine);
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
            tempTextPos = findPreviousWordTextPos(&tempTextPos, &tempIsMissing);
            if (tempIsMissing) {
                moveCursorRight(1);
                return false;
            }
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

void gotoNextTerm() {
    int8_t tempResult = gotoNextTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find term.");
    }
}

void gotoPreviousTerm() {
    int8_t tempResult = gotoPreviousTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find term.");
    }
}

void gotoNextWord() {
    int8_t tempResult = gotoNextWordHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find word.");
    }
}

void gotoPreviousWord() {
    int8_t tempResult = gotoPreviousWordHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find word.");
    }
}

void setMark(int64_t index) {
    if (index < 0 || index >= MARK_AMOUNT) {
        notifyUser((int8_t *)"Error: Bad mark number.");
        return;
    }
    markList[index] = cursorTextPos.line;
    markIsSetList[index] = true;
    notifyUser((int8_t *)"Set mark.");
}

void gotoMark(int64_t index) {
    if (index < 0 || index >= MARK_AMOUNT) {
        notifyUser((int8_t *)"Error: Bad mark number.");
        return;
    }
    if (!markIsSetList[index]) {
        notifyUser((int8_t *)"Error: Mark is not set.");
        return;
    }
    textPos_t tempTextPos;
    textLine_t *tempLine = markList[index];
    tempTextPos.line = tempLine;
    setTextPosIndex(&tempTextPos, tempLine->textAllocation.length);
    moveCursor(&tempTextPos);
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
}

int8_t startSearchingForWordUnderCursor() {
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (tempLength <= 0) {
        return false;
    }
    int64_t tempStartIndex = getTextPosIndex(&cursorTextPos);
    int64_t tempEndIndex = tempStartIndex;
    if (tempStartIndex >= tempLength) {
        return false;
    }
    int64_t tempLastIndex;
    tempLastIndex = tempStartIndex;
    while (tempStartIndex >= 0) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempLastIndex = tempStartIndex;
        tempStartIndex -= 1;
    }
    tempStartIndex = tempLastIndex;
    tempLastIndex = tempEndIndex;
    while (tempEndIndex < tempLength) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndIndex];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempLastIndex = tempEndIndex;
        tempEndIndex += 1;
    }
    tempEndIndex = tempLastIndex;
    tempEndIndex += 1;
    int64_t tempLength2 = tempEndIndex - tempStartIndex;
    if (tempLength2 == 1) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex];
        if (!isWordCharacter(tempCharacter)) {
            return false;
        }
    }
    searchTermLength = tempLength2;
    copyData(searchTerm, cursorTextPos.line->textAllocation.text + tempStartIndex, searchTermLength);
    searchTerm[searchTermLength] = 0;
    return true;
}

void findNextTermUnderCursor() {
    int8_t tempResult = startSearchingForWordUnderCursor();
    if (tempResult) {
        gotoNextWord();
    } else {
        notifyUser((int8_t *)"No word under cursor.");
    }
}

void findPreviousTermUnderCursor() {
    int8_t tempResult = startSearchingForWordUnderCursor();
    if (tempResult) {
        gotoPreviousWord();
    } else {
        notifyUser((int8_t *)"No word under cursor.");
    }
}

void moveTextUp(int32_t amount) {
    int32_t tempCount = 0;
    while (tempCount < amount) {
        if (topTextLineRow > 0) {
            topTextLineRow -= 1;
        } else {
            textLine_t *tempLine = getPreviousTextLine(topTextLine);
            if (tempLine == NULL) {
                break;
            }
            topTextLine = tempLine;
            topTextLineRow = getTextLineRowCount(topTextLine) - 1;
        }
        tempCount += 1;
    }
    displayAllTextLines();
}

void moveTextDown(int32_t amount) {
    int32_t tempCount = 0;
    while (tempCount < amount) {
        int64_t tempRowCount = getTextLineRowCount(topTextLine);
        if (topTextLineRow + 1 < tempRowCount) {
            topTextLineRow += 1;
        } else {
            textLine_t *tempLine = getNextTextLine(topTextLine);
            if (tempLine == NULL) {
                break;
            }
            topTextLine = tempLine;
            topTextLineRow = 0;
        }
        tempCount += 1;
    }
    displayAllTextLines();
}

void moveCursorToVisibleText() {
    eraseCursor();
    cursorTextPos.line = topTextLine;
    cursorTextPos.row = topTextLineRow;
    cursorTextPos.column = 0;
    displayCursor();
}

int8_t goToCharacterInclusive() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter == 0) {
        return false;
    }
    textPos_t tempTextPos = cursorTextPos;
    while (true) {
        int8_t tempCharacter2 = getTextPosCharacter(&tempTextPos);
        if (tempCharacter2 == tempCharacter) {
            moveCursor(&tempTextPos);
            cursorSnapColumn = cursorTextPos.column;
            historyFrameIsConsecutive = false;
            return true;
        }
        int8_t tempResult = moveTextPosForward(&tempTextPos);
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find character.");
            return false;
        }
    }
}

int8_t goToCharacterExclusive() {
    int8_t tempResult = goToCharacterInclusive();
    if (!tempResult) {
        return false;
    }
    moveCursorLeft(1);
    return true;
}

int8_t reverseGoToCharacterInclusive() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter == 0) {
        return false;
    }
    textPos_t tempTextPos = cursorTextPos;
    while (true) {
        int8_t tempCharacter2 = getTextPosCharacter(&tempTextPos);
        if (tempCharacter2 == tempCharacter) {
            moveCursor(&tempTextPos);
            cursorSnapColumn = cursorTextPos.column;
            historyFrameIsConsecutive = false;
            return true;
        }
        int8_t tempResult = moveTextPosBackward(&tempTextPos);
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find character.");
            return false;
        }
    }
}

int8_t reverseGoToCharacterExclusive() {
    int8_t tempResult = reverseGoToCharacterInclusive();
    if (!tempResult) {
        return false;
    }
    moveCursorRight(1);
    return true;
}

void moveCursorToEndOfIndentation() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t index = 0;
    while (index < tempNextTextPos.line->textAllocation.length) {
        int8_t tempCharacter = tempNextTextPos.line->textAllocation.text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    setTextPosIndex(&tempNextTextPos, index);
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}
