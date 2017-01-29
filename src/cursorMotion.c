
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "cursorMotion.h"
#include "breadtext.h"

void moveCursor(textPos_t *pos) {
    if (activityMode == HIGHLIGHT_WORD_MODE) {
        setActivityMode(COMMAND_MODE);
    }
    historyFrameIsConsecutive = false;
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
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfLine() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t tempLength = tempNextTextPos.line->textAllocation.length;
    setTextPosIndex(&tempNextTextPos, tempLength);
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToBeginningOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getLeftmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getRightmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

textPos_t findNextTermInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    while (index <= tempLength - searchTermLength) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempPos;
            tempPos.line = pos->line;
            setTextPosIndex(&tempPos, index);
            *isMissing = false;
            return tempPos;
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
            textPos_t tempPos;
            tempPos.line = pos->line;
            setTextPosIndex(&tempPos, index);
            *isMissing = false;
            return tempPos;
        }
        index -= 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findNextTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempPos2 = findNextTermInTextLine(&tempPos, &tempIsMissing);
        if (tempIsMissing) {
            tempPos.line = getNextTextLine(tempPos.line);
            if (tempPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            tempPos.row = 0;
            tempPos.column = 0;
        } else {
            *isMissing = false;
            return tempPos2;
        }
    }
}

textPos_t findPreviousTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempPos2 = findPreviousTermInTextLine(&tempPos, &tempIsMissing);
        if (tempIsMissing) {
            tempPos.line = getPreviousTextLine(tempPos.line);
            if (tempPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            int64_t tempLength = tempPos.line->textAllocation.length;
            setTextPosIndex(&tempPos, tempLength);
        } else {
            *isMissing = false;
            return tempPos2;
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

void gotoNextTerm() {
    int8_t tempResult = gotoNextTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Could not find term.");
    }
}

void gotoPreviousTerm() {
    int8_t tempResult = gotoPreviousTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Could not find term.");
    }
}

void setMark(int64_t index) {
    if (index < 0 || index >= MARK_AMOUNT) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Error: Bad mark number.");
        return;
    }
    markList[index] = cursorTextPos.line;
    markIsSetList[index] = true;
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Set mark.");
}

void gotoMark(int64_t index) {
    if (index < 0 || index >= MARK_AMOUNT) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Error: Bad mark number.");
        return;
    }
    if (!markIsSetList[index]) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Error: Mark is not set.");
        return;
    }
    textPos_t tempTextPos;
    tempTextPos.line = markList[index];
    tempTextPos.row = 0;
    tempTextPos.column = 0;
    moveCursor(&tempTextPos);
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
}
