
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "indentation.h"
#include "cursorMotion.h"
#include "insertDelete.h"
#include "selection.h"
#include "manipulation.h"
#include "breadtext.h"

int64_t findAndReplaceAllTerms(int8_t *replacementText) {
    int64_t tempLength = strlen((char *)replacementText);
    int64_t output = 0;
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos.line = getLeftmostTextLine(rootTextLine);
    tempTextPos.row = 0;
    tempTextPos.column = 0;
    while (true) {
        tempTextPos = findNextTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            break;
        }
        if (output == 0) {
            addHistoryFrame();
        }
        int64_t index = getTextPosIndex(&tempTextPos);
        recordTextLineDeleted(tempTextPos.line);
        removeTextFromTextAllocation(&(tempTextPos.line->textAllocation), index, searchTermLength);
        insertTextIntoTextAllocation(&(tempTextPos.line->textAllocation), index, replacementText, tempLength);
        recordTextLineInserted(tempTextPos.line);
        index += tempLength;
        setTextPosIndex(&tempTextPos, index);
        output += 1;
    }
    if (output > 0) {
        finishCurrentHistoryFrame();
        historyFrameIsConsecutive = false;
        displayAllTextLines();
        textBufferIsDirty = true;
    }
    return output;
}

void toggleSemicolonAtEndOfLine() {
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    int64_t tempEndOfLineIndex = tempLength;
    while (tempEndOfLineIndex > 0) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndOfLineIndex - 1];
        if (!isWhitespace(tempCharacter)) {
            break;
        }
        tempEndOfLineIndex -= 1;
    }
    int8_t tempShouldDeleteSemicolon = false;
    if (tempEndOfLineIndex > 0) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndOfLineIndex - 1];
        if (tempCharacter == ';') {
            tempShouldDeleteSemicolon = true;
        }
    }
    textPos_t tempLastTextPos = cursorTextPos;
    textPos_t tempTextPos = cursorTextPos;
    setTextPosIndex(&tempTextPos, tempEndOfLineIndex);
    moveCursor(&tempTextPos);
    if (tempShouldDeleteSemicolon) {
        deleteCharacterBeforeCursor(true);
    } else {
        insertCharacterBeforeCursor(';');
    }
    moveCursor(&tempLastTextPos);
}

void uppercaseSelection() {
    addHistoryFrame();
    textPos_t tempFirstTextPos;
    textPos_t tempLastTextPos;
    if (isHighlighting) {
        tempFirstTextPos = *(getFirstHighlightTextPos());
        tempLastTextPos = *(getLastHighlightTextPos());
    } else {
        tempFirstTextPos = cursorTextPos;
        tempLastTextPos = cursorTextPos;
    };
    textLine_t *tempLine = tempFirstTextPos.line;
    int64_t index = getTextPosIndex(&tempFirstTextPos);
    textLine_t *tempLastLine = tempLastTextPos.line;
    int64_t tempLastIndex = getTextPosIndex(&tempLastTextPos);
    recordTextLineDeleted(tempLine);
    while (true) {
        int64_t tempLength = tempLine->textAllocation.length;
        if (index <= tempLength) {
            if (index < tempLength) {
                int8_t tempCharacter = tempLine->textAllocation.text[index];
                if (tempCharacter >= 'a' && tempCharacter <= 'z') {
                    tempCharacter -= 32;
                    tempLine->textAllocation.text[index] = tempCharacter;
                }
            }
            index += 1;
            if (tempLine == tempLastLine && index > tempLastIndex) {
                recordTextLineInserted(tempLine);
                displayTextLine(getTextLinePosY(tempLine), tempLine);
                break;
            }
        } else {
            recordTextLineInserted(tempLine);
            displayTextLine(getTextLinePosY(tempLine), tempLine);
            if (tempLine == tempLastLine) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
            index = 0;
            recordTextLineDeleted(tempLine);
        }
    }
    displayCursor();
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;    
    textBufferIsDirty = true;
}

void lowercaseSelection() {
    addHistoryFrame();
    textPos_t tempFirstTextPos;
    textPos_t tempLastTextPos;
    if (isHighlighting) {
        tempFirstTextPos = *(getFirstHighlightTextPos());
        tempLastTextPos = *(getLastHighlightTextPos());
    } else {
        tempFirstTextPos = cursorTextPos;
        tempLastTextPos = cursorTextPos;
    };
    textLine_t *tempLine = tempFirstTextPos.line;
    int64_t index = getTextPosIndex(&tempFirstTextPos);
    textLine_t *tempLastLine = tempLastTextPos.line;
    int64_t tempLastIndex = getTextPosIndex(&tempLastTextPos);
    recordTextLineDeleted(tempLine);
    while (true) {
        int64_t tempLength = tempLine->textAllocation.length;
        if (index <= tempLength) {
            if (index < tempLength) {
                int8_t tempCharacter = tempLine->textAllocation.text[index];
                if (tempCharacter >= 'A' && tempCharacter <= 'Z') {
                    tempCharacter += 32;
                    tempLine->textAllocation.text[index] = tempCharacter;
                }
            }
            index += 1;
            if (tempLine == tempLastLine && index > tempLastIndex) {
                recordTextLineInserted(tempLine);
                displayTextLine(getTextLinePosY(tempLine), tempLine);
                break;
            }
        } else {
            recordTextLineInserted(tempLine);
            displayTextLine(getTextLinePosY(tempLine), tempLine);
            if (tempLine == tempLastLine) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
            index = 0;
            recordTextLineDeleted(tempLine);
        }
    }
    displayCursor();
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;    
    textBufferIsDirty = true;
}

void addToNumberUnderCursor(int64_t offset) {
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    int64_t tempWordStartIndex = getTextPosIndex(&cursorTextPos);
    int64_t tempWordEndIndex = tempWordStartIndex;
    while (tempWordStartIndex > 0) {
        int64_t tempNextIndex = tempWordStartIndex - 1;
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempNextIndex];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempWordStartIndex = tempNextIndex;
    }
    while (tempWordEndIndex < tempLength) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempWordEndIndex];
        if (!isWordCharacter(tempCharacter)) {
            break;
        }
        tempWordEndIndex += 1;
    }
    int8_t tempIsHexadecimal = false;
    if (tempWordEndIndex - tempWordStartIndex > 2) {
        int8_t tempCharacter1 = cursorTextPos.line->textAllocation.text[tempWordStartIndex];
        int8_t tempCharacter2 = cursorTextPos.line->textAllocation.text[tempWordStartIndex + 1];
        if (tempCharacter1 == '0' && tempCharacter2 == 'x') {
            tempIsHexadecimal = true;
        }
    }
    int64_t tempStartIndex = getTextPosIndex(&cursorTextPos);
    int64_t tempDigitStartIndex = tempStartIndex;
    int64_t tempEndIndex = tempStartIndex;
    int8_t tempIsNegative1 = false;
    int8_t tempHasFoundDigit = false;
    int8_t tempHasFoundX = false;
    while (tempStartIndex > 0) {
        int64_t tempNextIndex = tempStartIndex - 1;
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempNextIndex];
        if (tempCharacter == '-') {
            tempIsNegative1 = true;
            break;
        } else if (tempCharacter == 'x') {
            if (tempIsHexadecimal) {
                tempHasFoundX = true;
            } else {
                break;
            }
        } else if (tempCharacter == '.') {
            tempEndIndex = tempNextIndex;
            if (!tempHasFoundX) {
                tempDigitStartIndex = tempNextIndex;
            }
        } else if (tempCharacter >= '0' && tempCharacter <= '9') {
            if (!tempHasFoundX) {
                tempDigitStartIndex = tempNextIndex;
            }
            tempHasFoundDigit = true;
        } else if ((tempCharacter >= 'A' && tempCharacter <= 'F') || (tempCharacter >= 'a' && tempCharacter <= 'f')) {
            if (!tempIsHexadecimal) {
                break;
            }
        } else {
            break;
        }
        tempStartIndex = tempNextIndex;
    }
    while (tempEndIndex < tempLength) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndIndex];
        if (tempCharacter == 'x' && tempIsHexadecimal) {
            tempDigitStartIndex = tempEndIndex + 1;
        } else if (tempCharacter >= '0' && tempCharacter <= '9') {
            if (!tempHasFoundDigit) {
                tempDigitStartIndex = tempEndIndex;
                tempHasFoundDigit = true;
            }
        } else if ((tempCharacter >= 'A' && tempCharacter <= 'F') || (tempCharacter >= 'a' && tempCharacter <= 'f')) {
            if (!tempIsHexadecimal) {
                break;
            }
        } else if (tempCharacter == '-') {
            if (tempHasFoundDigit) {
                break;
            } else {
                tempIsNegative1 = true;
            }
        } else {
            break;
        }
        tempEndIndex += 1;
    }
    int64_t tempDigitCount1 = tempEndIndex - tempDigitStartIndex;
    if (tempDigitCount1 <= 0) {
        return;
    }
    addHistoryFrame();
    int64_t tempRowCount1 = getTextLineRowCount(cursorTextPos.line);
    recordTextLineDeleted(cursorTextPos.line);
    int8_t tempBuffer[1000];
    copyData(tempBuffer, cursorTextPos.line->textAllocation.text + tempDigitStartIndex, tempDigitCount1);
    int64_t tempIsNegative2 = tempIsNegative1;
    
    if (tempIsHexadecimal) {
        addToHexadecimalText(tempBuffer, offset);
    } else {
        tempBuffer[tempDigitCount1] = 0;
        int64_t tempNumber;
        sscanf((char *)tempBuffer, "%lld", (long long*)&tempNumber);
        if (tempIsNegative1) {
            tempNumber = -tempNumber;
        }
        tempNumber += offset;
        int64_t tempPositiveNumber;
        if (tempNumber >= 0) {
            tempPositiveNumber = tempNumber;
            tempIsNegative2 = false;
        } else {
            tempPositiveNumber = -tempNumber;
            tempIsNegative2 = true;
        }
        sprintf((char *)tempBuffer, "%lld", (long long)tempPositiveNumber);
    }
    
    eraseCursor();
    int64_t tempDigitCount2 = strlen((char *)tempBuffer);
    if (!tempIsNegative2 && tempIsNegative1 && !tempIsHexadecimal) {
        removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), tempDigitStartIndex - 1, 1);
        tempDigitStartIndex -= 1;
    }
    removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), tempDigitStartIndex, tempDigitCount1);
    if (tempIsNegative2 && !tempIsNegative1 && !tempIsHexadecimal) {
        int8_t tempCharacter = '-';
        insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempDigitStartIndex, &tempCharacter, 1);
        tempDigitStartIndex += 1;
    }
    insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempDigitStartIndex, tempBuffer, tempDigitCount2);
    recordTextLineInserted(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, tempDigitStartIndex);
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempRowCount2 = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempRowCount1 == tempRowCount2) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
    }
    displayCursor();
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;    
    textBufferIsDirty = true;
}

void incrementNumberUnderCursor() {
    addToNumberUnderCursor(1);
}

void decrementNumberUnderCursor() {
    addToNumberUnderCursor(-1);
}
