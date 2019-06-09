
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <regex.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "motion.h"
#include "insertDelete.h"
#include "breadtext.h"

int8_t equalDataWithCaseSensitivity(int8_t *source1, int8_t *source2, int64_t amount) {
    int64_t index = 0;
    while (index < amount) {
        int8_t tempCharacter1 = source1[index];
        int8_t tempCharacter2 = source2[index];
        if (!isCaseSensitive) {
            if (tempCharacter1 >= 'a' && tempCharacter1 <= 'z') {
                tempCharacter1 -= 'a' - 'A';
            }
            if (tempCharacter2 >= 'a' && tempCharacter2 <= 'z') {
                tempCharacter2 -= 'a' - 'A';
            }
        }
        if (tempCharacter1 != tempCharacter2) {
            return false;
        }
        index += 1;
    }
    return true;
}

void moveCursor(textPos_t *pos) {
    if (activityMode == HIGHLIGHT_STATIC_MODE) {
        setActivityMode(COMMAND_MODE);
    }
    historyFrameIsConsecutive = false;
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceFrame();
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

void moveCursorLeftConsecutive() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t index = getTextPosIndex(&tempNextTextPos);
    if (index <= 0) {
        textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        tempNextTextPos.line = tempLine;
        int64_t tempLength = tempNextTextPos.line->textAllocation.length;
        setTextPosIndex(&tempNextTextPos, tempLength);
    } else {
        setTextPosIndex(&tempNextTextPos, index - 1);
    }
    cursorTextPos = tempNextTextPos;
    cursorSnapColumn = tempNextTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayCursor();
    }
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

void findNextMatchingTermInTextLine(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (searchTermIsRegex) {
        int64_t tempLength2 = tempLength - index;
        int8_t tempBuffer[tempLength2 + 1];
        copyData(tempBuffer, pos->line->textAllocation.text + index, tempLength2);
        tempBuffer[tempLength2] = 0;
        regmatch_t tempMatchList[searchRegexForward.re_nsub];
        int32_t tempResult = regexec(&searchRegexForward, (char *)tempBuffer, searchRegexForward.re_nsub, tempMatchList, 0);
        if (tempResult == 0) {
            startPos->line = pos->line;
            setTextPosIndex(startPos, index + tempMatchList[0].rm_so);
            endPos->line = pos->line;
            setTextPosIndex(endPos, index + tempMatchList[0].rm_eo);
            *isMissing = false;
        } else {
            *isMissing = true;
        }
    } else {
        while (index <= tempLength - searchTermLength) {
            if (equalDataWithCaseSensitivity(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
                startPos->line = pos->line;
                setTextPosIndex(startPos, index);
                endPos->line = pos->line;
                setTextPosIndex(endPos, index + searchTermLength);
                *isMissing = false;
                return;
            }
            index += 1;
        }
        *isMissing = true;
    }
}

void findPreviousMatchingTermInTextLine(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (searchTermIsRegex) {
        int64_t tempLength2 = index + 1;
        if (tempLength2 > tempLength) {
            tempLength2 = tempLength;
        }
        int8_t tempBuffer[tempLength2 + 1];
        copyData(tempBuffer, pos->line->textAllocation.text, tempLength2);
        tempBuffer[tempLength2] = 0;
        regmatch_t tempMatchList[searchRegexBackward.re_nsub + 1];
        int32_t tempResult = regexec(&searchRegexBackward, (char *)tempBuffer, searchRegexBackward.re_nsub + 1, tempMatchList, 0);
        if (tempResult == 0) {
            startPos->line = pos->line;
            setTextPosIndex(startPos, tempMatchList[1].rm_so);
            endPos->line = pos->line;
            setTextPosIndex(endPos, tempMatchList[1].rm_eo);
            *isMissing = false;
        } else {
            *isMissing = true;
        }
    } else {
        if (index > tempLength - searchTermLength) {
            index = tempLength - searchTermLength;
        }
        while (index >= 0) {
            if (equalDataWithCaseSensitivity(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
                startPos->line = pos->line;
                setTextPosIndex(startPos, index);
                endPos->line = pos->line;
                setTextPosIndex(endPos, index + searchTermLength);
                *isMissing = false;
                return;
            }
            index -= 1;
        }
        *isMissing = true;
    }
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

textPos_t findNextMatchingWordInTextLine(int8_t *isMissing, textPos_t *pos) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    while (index <= tempLength - searchTermLength) {
        if (equalDataWithCaseSensitivity(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
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

textPos_t findPreviousMatchingWordInTextLine(int8_t *isMissing, textPos_t *pos) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (index > tempLength - searchTermLength) {
        index = tempLength - searchTermLength;
    }
    while (index >= 0) {
        if (equalDataWithCaseSensitivity(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
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

void findNextMatchingTermTextPos(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        findNextMatchingTermInTextLine(startPos, endPos, &tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            tempTextPos.line = getNextTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return;
            }
            tempTextPos.row = 0;
            tempTextPos.column = 0;
        } else {
            *isMissing = false;
            return;
        }
    }
}

void findPreviousMatchingTermTextPos(textPos_t *startPos, textPos_t *endPos, int8_t *isMissing, textPos_t *pos) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        findPreviousMatchingTermInTextLine(startPos, endPos, &tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            tempTextPos.line = getPreviousTextLine(tempTextPos.line);
            if (tempTextPos.line == NULL) {
                *isMissing = true;
                return;
            }
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
        } else {
            *isMissing = false;
            return;
        }
    }
}

textPos_t findNextMatchingWordTextPos(int8_t *isMissing, textPos_t *pos) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findNextMatchingWordInTextLine(&tempIsMissing, &tempTextPos);
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

textPos_t findPreviousMatchingWordTextPos(int8_t *isMissing, textPos_t *pos) {
    textPos_t tempTextPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempTextPos2 = findPreviousMatchingWordInTextLine(&tempIsMissing, &tempTextPos);
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

int8_t goToNextMatchingTermHelper() {
    moveCursorRight(1);
    int8_t tempIsMissing;
    textPos_t tempStartTextPos;
    textPos_t tempEndTextPos;
    findNextMatchingTermTextPos(&tempStartTextPos, &tempEndTextPos, &tempIsMissing, &cursorTextPos);
    if (tempIsMissing) {
        textPos_t tempTextPos;
        tempTextPos.line = getLeftmostTextLine(rootTextLine);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        findNextMatchingTermTextPos(&tempStartTextPos, &tempEndTextPos, &tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            moveCursorLeft(1);
            return false;
        }
    }
    if (searchTermIsRegex) {
        setActivityMode(COMMAND_MODE);
        eraseCursor();
        int64_t tempStartIndex = getTextPosIndex(&tempStartTextPos);
        int64_t tempEndIndex = getTextPosIndex(&tempEndTextPos);
        if (tempEndIndex > tempStartIndex) {
            tempEndIndex -= 1;
        }
        eraseLineNumber();
        cursorTextPos.line = tempStartTextPos.line;
        displayLineNumber();
        setTextPosIndex(&cursorTextPos, tempStartIndex);
        highlightTextPos.line = tempEndTextPos.line;
        setTextPosIndex(&highlightTextPos, tempEndIndex);
        scrollCursorOntoScreen();
        setActivityMode(HIGHLIGHT_STATIC_MODE);
    } else {
        moveCursor(&tempStartTextPos);
    }
    return true;
}

int8_t goToPreviousMatchingTermHelper() {
    int8_t tempIsMissing;
    textPos_t tempStartTextPos;
    textPos_t tempEndTextPos;
    if (getTextLineNumber(cursorTextPos.line) <= 1 && cursorTextPos.row == 0 && cursorTextPos.column == 0) {
        textPos_t tempTextPos;
        tempTextPos.line = getRightmostTextLine(rootTextLine);
        int64_t tempLength = tempTextPos.line->textAllocation.length;
        setTextPosIndex(&tempTextPos, tempLength);
        findPreviousMatchingTermTextPos(&tempStartTextPos, &tempEndTextPos, &tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            return false;
        }
    } else {
        moveCursorLeft(1);
        findPreviousMatchingTermTextPos(&tempStartTextPos, &tempEndTextPos, &tempIsMissing, &cursorTextPos);
        if (tempIsMissing) {
            textPos_t tempTextPos;
            tempTextPos.line = getRightmostTextLine(rootTextLine);
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
            findPreviousMatchingTermTextPos(&tempStartTextPos, &tempEndTextPos, &tempIsMissing, &tempTextPos);
            if (tempIsMissing) {
                moveCursorRight(1);
                return false;
            }
        }
    }
    if (searchTermIsRegex) {
        setActivityMode(COMMAND_MODE);
        eraseCursor();
        int64_t tempStartIndex = getTextPosIndex(&tempStartTextPos);
        int64_t tempEndIndex = getTextPosIndex(&tempEndTextPos);
        if (tempEndIndex > tempStartIndex) {
            tempEndIndex -= 1;
        }
        eraseLineNumber();
        cursorTextPos.line = tempStartTextPos.line;
        displayLineNumber();
        setTextPosIndex(&cursorTextPos, tempStartIndex);
        highlightTextPos.line = tempEndTextPos.line;
        setTextPosIndex(&highlightTextPos, tempEndIndex);
        scrollCursorOntoScreen();
        setActivityMode(HIGHLIGHT_STATIC_MODE);
    } else {
        moveCursor(&tempStartTextPos);
    }
    return true;
}

int8_t goToNextMatchingWordHelper() {
    moveCursorRight(1);
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos = findNextMatchingWordTextPos(&tempIsMissing, &cursorTextPos);
    if (tempIsMissing) {
        tempTextPos.line = getLeftmostTextLine(rootTextLine);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        tempTextPos = findNextMatchingWordTextPos(&tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            moveCursorLeft(1);
            return false;
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

int8_t goToPreviousMatchingWordHelper() {
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    if (getTextLineNumber(cursorTextPos.line) <= 1 && cursorTextPos.row == 0 && cursorTextPos.column == 0) {
        tempTextPos.line = getRightmostTextLine(rootTextLine);
        int64_t tempLength = tempTextPos.line->textAllocation.length;
        setTextPosIndex(&tempTextPos, tempLength);
        tempTextPos = findPreviousMatchingWordTextPos(&tempIsMissing, &tempTextPos);
        if (tempIsMissing) {
            return false;
        }
    } else {
        moveCursorLeft(1);
        tempTextPos = findPreviousMatchingWordTextPos(&tempIsMissing, &cursorTextPos);
        if (tempIsMissing) {
            tempTextPos.line = getRightmostTextLine(rootTextLine);
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
            tempTextPos = findPreviousMatchingWordTextPos(&tempIsMissing, &tempTextPos);
            if (tempIsMissing) {
                moveCursorRight(1);
                return false;
            }
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

void goToNextMatchingTerm() {
    int8_t tempResult = goToNextMatchingTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find term.");
    }
}

void goToPreviousMatchingTerm() {
    int8_t tempResult = goToPreviousMatchingTermHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find term.");
    }
}

void goToNextMatchingWord() {
    int8_t tempResult = goToNextMatchingWordHelper();
    cursorSnapColumn = cursorTextPos.column;
    historyFrameIsConsecutive = false;
    if (!tempResult) {
        notifyUser((int8_t *)"Could not find word.");
    }
}

void goToPreviousMatchingWord() {
    int8_t tempResult = goToPreviousMatchingWordHelper();
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
    mark_t *tempMark = markList + index;
    tempMark->textLine = cursorTextPos.line;
    tempMark->characterIndex = getTextPosIndex(&cursorTextPos);
    tempMark->isSet = true;
    notifyUser((int8_t *)"Set mark.");
}

void goToMark(int64_t index) {
    if (index < 0 || index >= MARK_AMOUNT) {
        notifyUser((int8_t *)"Error: Bad mark number.");
        return;
    }
    mark_t *tempMark = markList + index;
    if (!tempMark->isSet) {
        notifyUser((int8_t *)"Error: Mark is not set.");
        return;
    }
    textPos_t tempTextPos;
    textLine_t *tempLine = tempMark->textLine;
    tempTextPos.line = tempLine;
    setTextPosIndex(&tempTextPos, tempMark->characterIndex);
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
    searchTermIsRegex = false;
    return true;
}

void findNextMatchingTermUnderCursor() {
    int8_t tempResult = startSearchingForWordUnderCursor();
    if (tempResult) {
        goToNextMatchingWord();
    } else {
        notifyUser((int8_t *)"No word under cursor.");
    }
}

void findPreviousMatchingTermUnderCursor() {
    int8_t tempResult = startSearchingForWordUnderCursor();
    if (tempResult) {
        goToPreviousMatchingWord();
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
    textPos_t tempTextPos;
    tempTextPos.line = topTextLine;
    tempTextPos.row = topTextLineRow;
    tempTextPos.column = 0;
    moveCursor(&tempTextPos);
}

void promptAndGoToCharacterExclusive() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_GOTO_EXCLUSIVE;
}

void promptAndGoToCharacterInclusive() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_GOTO_INCLUSIVE;
}

void promptAndReverseGoToCharacterExclusive() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_REVERSE_GOTO_EXCLUSIVE;
}

void promptAndReverseGoToCharacterInclusive() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_REVERSE_GOTO_INCLUSIVE;
}

int8_t goToCharacterInclusive(int8_t character) {
    textPos_t tempTextPos = cursorTextPos;
    while (true) {
        int8_t tempCharacter2 = getTextPosCharacter(&tempTextPos);
        if (tempCharacter2 == character) {
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

int8_t goToCharacterExclusive(int8_t character) {
    int8_t tempResult = goToCharacterInclusive(character);
    if (!tempResult) {
        return false;
    }
    moveCursorLeft(1);
    return true;
}

int8_t reverseGoToCharacterInclusive(int8_t character) {
    textPos_t tempTextPos = cursorTextPos;
    while (true) {
        int8_t tempCharacter2 = getTextPosCharacter(&tempTextPos);
        if (tempCharacter2 == character) {
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

int8_t reverseGoToCharacterExclusive(int8_t character) {
    int8_t tempResult = reverseGoToCharacterInclusive(character);
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

// An index of -1 indicates a missing index.
void findMatchingCharacterInLine(textLine_t *line, int64_t *index, int8_t direction, int32_t *depth, int8_t startCharacter, int8_t endCharacter) {
    if (*index < 0) {
        if (direction > 0) {
            *index = 0;
        } else {
            *index = line->textAllocation.length - 1;
        }
    }
    int8_t tempIsInString = false;
    int8_t tempStringDelimiter;
    while (*index >= 0 && *index < line->textAllocation.length) {
        int8_t tempCharacter = line->textAllocation.text[*index];
        if (tempIsInString) {
            if (tempCharacter == tempStringDelimiter) {
                if (*index > 0) {
                    int8_t tempCharacter2 = line->textAllocation.text[*index - 1];
                    if (tempCharacter2 != '\\') {
                        tempIsInString = false;
                    }
                } else {
                    tempIsInString = false;
                }
            }
        } else {
            if (tempCharacter == '"' || tempCharacter == '\'') {
                tempIsInString = true;
                tempStringDelimiter = tempCharacter;
            }
            if (tempCharacter == startCharacter) {
                *depth += 1;
            }
            if (tempCharacter == endCharacter) {
                *depth -= 1;
                if (*depth <= 0) {
                    break;
                }
            }
        }
        if (direction > 0) {
            *index += 1;
        } else {
            *index -= 1;
        }
    }
}

void goToMatchingStringDelimiter() {
    int64_t tempCursorIndex = getTextPosIndex(&cursorTextPos);
    int8_t tempIsInString = false;
    int8_t tempStringDelimiter;
    int8_t tempIsEscaped = false;
    int64_t tempLastDelimiterIndex;
    int8_t tempHasFoundMatchingDelimiter = false;
    int64_t tempMatchingDelimiterIndex;
    int64_t index = 0;
    while (index < cursorTextPos.line->textAllocation.length) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[index];
        if (tempIsInString) {
            if (tempIsEscaped) {
                tempIsEscaped = false;
            } else {
                if (tempCharacter == '\\') {
                    tempIsEscaped = true;
                }
                if (tempCharacter == tempStringDelimiter) {
                    if (tempLastDelimiterIndex == tempCursorIndex) {
                        tempHasFoundMatchingDelimiter = true;
                        tempMatchingDelimiterIndex = index;
                        break;
                    }
                    if (index == tempCursorIndex) {
                        tempHasFoundMatchingDelimiter = true;
                        tempMatchingDelimiterIndex = tempLastDelimiterIndex;
                        break;
                    }
                    tempIsInString = false;
                }
            }
        } else {
            if (tempCharacter == '"' || tempCharacter == '\'') {
                tempIsInString = true;
                tempStringDelimiter = tempCharacter;
                tempLastDelimiterIndex = index;
            }
        }
        index += 1;
    }
    if (tempHasFoundMatchingDelimiter) {
        textPos_t tempTextPos;
        tempTextPos.line = cursorTextPos.line;
        setTextPosIndex(&tempTextPos, tempMatchingDelimiterIndex);
        moveCursor(&tempTextPos);
    } else {
        notifyUser((int8_t *)"Could not find matching character.");
    }
}

void goToMatchingCharacter() {
    int8_t tempStartCharacter = getTextPosCharacter(&cursorTextPos);
    if (tempStartCharacter == '"' || tempStartCharacter == '\'') {
        goToMatchingStringDelimiter();
        return;
    }
    int8_t tempEndCharacter = 0;
    int8_t tempDirection;
    if (tempStartCharacter == '(') {
        tempEndCharacter = ')';
        tempDirection = 1;
    }
    if (tempStartCharacter == ')') {
        tempEndCharacter = '(';
        tempDirection = -1;
    }
    if (tempStartCharacter == '[') {
        tempEndCharacter = ']';
        tempDirection = 1;
    }
    if (tempStartCharacter == ']') {
        tempEndCharacter = '[';
        tempDirection = -1;
    }
    if (tempStartCharacter == '{') {
        tempEndCharacter = '}';
        tempDirection = 1;
    }
    if (tempStartCharacter == '}') {
        tempEndCharacter = '{';
        tempDirection = -1;
    }
    if (tempStartCharacter == '<') {
        tempEndCharacter = '>';
        tempDirection = 1;
    }
    if (tempStartCharacter == '>') {
        tempEndCharacter = '<';
        tempDirection = -1;
    }
    if (tempEndCharacter == 0) {
        notifyUser((int8_t *)"Invalid character.");
        return;        
    }
    int32_t tempDepth = 1;
    int64_t index = getTextPosIndex(&cursorTextPos);
    index += tempDirection;
    textLine_t *tempLine = cursorTextPos.line;
    if (index < 0) {
        tempLine = getPreviousTextLine(tempLine);
        index = -1;
    } else if (index >= tempLine->textAllocation.length) {
        tempLine = getNextTextLine(tempLine);
        index = -1;
    }
    while (tempLine != NULL) {
        findMatchingCharacterInLine(tempLine, &index, tempDirection, &tempDepth, tempStartCharacter, tempEndCharacter);
        if (tempDepth <= 0) {
            break;
        }
        if (tempDirection > 0) {
            tempLine = getNextTextLine(tempLine);
        } else {
            tempLine = getPreviousTextLine(tempLine);
        }
        index = -1;
    }
    if (tempDepth <= 0) {
        textPos_t tempTextPos;
        tempTextPos.line = tempLine;
        setTextPosIndex(&tempTextPos, index);
        moveCursor(&tempTextPos);
    } else {
        notifyUser((int8_t *)"Could not find matching character.");
    }
}

void goToStartOfWord() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t index = getTextPosIndex(&tempNextTextPos);
    while (index > 0) {
        int8_t tempCharacter = getTextPosCharacter(&tempNextTextPos);
        if (isWordCharacter(tempCharacter)) {
            break;
        }
        index -= 1;
        setTextPosIndex(&tempNextTextPos, index);
    }
    while (index > 0) {
        textPos_t tempTextPos = tempNextTextPos;
        index -= 1;
        setTextPosIndex(&tempTextPos, index);
        int8_t tempCharacter = getTextPosCharacter(&tempTextPos);
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempNextTextPos = tempTextPos;
    }
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

void goToEndOfWord() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t tempLength = tempNextTextPos.line->textAllocation.length;
    int64_t index = getTextPosIndex(&tempNextTextPos);
    while (index < tempLength) {
        int8_t tempCharacter = getTextPosCharacter(&tempNextTextPos);
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        index += 1;
        setTextPosIndex(&tempNextTextPos, index);
    }
    moveCursor(&tempNextTextPos);
    cursorSnapColumn = tempNextTextPos.column;
    historyFrameIsConsecutive = false;
}

void goToPreviousWord() {
    // TODO: Implement.
    
}

void goToNextWord() {
    // TODO: Implement.
    
}


