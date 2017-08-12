
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
#include "motion.h"
#include "insertDelete.h"
#include "selection.h"
#include "syntax.h"
#include "manipulation.h"
#include "breadtext.h"

int8_t *commentFlag1 = (int8_t *)"//";
int8_t *commentFlag2 = (int8_t *)"#";

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
    int64_t tempSnapColumn = cursorSnapColumn;
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    int8_t tempIsAtEndOfLine = (getTextPosIndex(&cursorTextPos) >= tempLength);
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
    int8_t tempShouldMoveCursorBack;
    if (tempShouldDeleteSemicolon) {
        deleteCharacterBeforeCursor(true);
        tempShouldMoveCursorBack = !tempIsAtEndOfLine;
    } else {
        insertCharacterBeforeCursor(';');
        tempShouldMoveCursorBack = true;
    }
    if (tempShouldMoveCursorBack) {
        moveCursor(&tempLastTextPos);
        cursorSnapColumn = tempSnapColumn;
    }
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
    tempBuffer[tempDigitCount1] = 0;
    int64_t tempIsNegative2 = tempIsNegative1;
    
    if (tempIsHexadecimal) {
        addToHexadecimalText(tempBuffer, offset);
    } else {
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

void toggleLineComment(textLine_t *line, int8_t *commentFlag) {
    recordTextLineDeleted(line);
    int32_t tempCommentFlagLength = strlen((char *)commentFlag);
    textAllocation_t *tempTextAllocation = &(line->textAllocation);
    int64_t index = 0;
    while (index < tempTextAllocation->length) {
        int8_t tempCharacter = tempTextAllocation->text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    int8_t tempIsComment = true;
    int32_t tempOffset = 0;
    while (tempOffset < tempCommentFlagLength) {
        int32_t tempIndex = index + tempOffset;
        if (tempIndex >= tempTextAllocation->length) {
            tempIsComment = false;
            break;
        }
        int8_t tempCharacter1 = commentFlag[tempOffset];
        int8_t tempCharacter2 = tempTextAllocation->text[tempIndex];
        if (tempCharacter1 != tempCharacter2) {
            tempIsComment = false;
            break;
        }
        tempOffset += 1;
    }
    if (tempIsComment) {
        removeTextFromTextAllocation(tempTextAllocation, index, tempCommentFlagLength);
    } else {
        insertTextIntoTextAllocation(tempTextAllocation, index, commentFlag, tempCommentFlagLength);
    }
    if (line == cursorTextPos.line) {
        if (getTextPosIndex(&cursorTextPos) > tempTextAllocation->length) {
            setTextPosIndex(&cursorTextPos, tempTextAllocation->length);
        }
    }
    if (line == highlightTextPos.line) {
        if (getTextPosIndex(&highlightTextPos) > tempTextAllocation->length) {
            setTextPosIndex(&highlightTextPos, tempTextAllocation->length);
        }
    }
    recordTextLineInserted(line);
}

void toggleSelectionComment() {
    addHistoryFrame();
    int8_t *tempExtension = getFileExtension(filePath);
    int8_t *tempCommentFlag;
    if (commentPrefix != NULL) {
        tempCommentFlag = commentPrefix;
    } else {
        if (tempExtension == NULL) {
            tempCommentFlag = commentFlag1;
        } else {
            if (strcmp((char *)tempExtension, "py") == 0) {
                tempCommentFlag = commentFlag2;
            } else if (strcmp((char *)tempExtension, "sh") == 0) {
                tempCommentFlag = commentFlag2;
            } else if (strcmp((char *)tempExtension, "pl") == 0) {
                tempCommentFlag = commentFlag2;
            } else if (strcmp((char *)tempExtension, "rb") == 0) {
                tempCommentFlag = commentFlag2;
            } else {
                tempCommentFlag = commentFlag1;
            }
        }
    }
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
    textLine_t *tempLastLine = tempLastTextPos.line;
    while (true) {
        toggleLineComment(tempLine, tempCommentFlag);
        if (tempLine == tempLastLine) {
            break;
        }
        tempLine = getNextTextLine(tempLine);
    }
    displayAllTextLines();
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
}

void toggleBooleanLiteral() {
    int64_t tempStartIndex = getTextPosIndex(&cursorTextPos);
    int64_t tempEndIndex = tempStartIndex;
    int8_t *tempText = cursorTextPos.line->textAllocation.text;
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    while (tempStartIndex > 0) {
        int8_t tempCharacter = tempText[tempStartIndex - 1];
        if (!((tempCharacter >= 'A' && tempCharacter <= 'Z') || (tempCharacter >= 'a' && tempCharacter <= 'z'))) {
            break;
        }
        tempStartIndex -= 1;
    }
    while (tempEndIndex < tempLength) {
        int8_t tempCharacter = tempText[tempEndIndex];
        if (!((tempCharacter >= 'A' && tempCharacter <= 'Z') || (tempCharacter >= 'a' && tempCharacter <= 'z'))) {
            break;
        }
        tempEndIndex += 1;
    }
    int64_t tempLength2 = tempEndIndex - tempStartIndex;
    if (tempLength2 != 4 && tempLength2 != 5) {
        return;
    }
    int8_t tempBuffer[10];
    copyData(tempBuffer, tempText + tempStartIndex, tempLength2);
    tempBuffer[tempLength2] = 0;
    int8_t tempNewText[10];
    int8_t tempHasSetNewText = false;
    if (strcmp((char *)tempBuffer, "false") == 0) {
        strcpy((char *)tempNewText, "true");
        tempHasSetNewText = true;
    }
    if (strcmp((char *)tempBuffer, "False") == 0) {
        strcpy((char *)tempNewText, "True");
        tempHasSetNewText = true;
    }
    if (strcmp((char *)tempBuffer, "FALSE") == 0) {
        strcpy((char *)tempNewText, "TRUE");
        tempHasSetNewText = true;
    }
    if (strcmp((char *)tempBuffer, "true") == 0) {
        strcpy((char *)tempNewText, "false");
        tempHasSetNewText = true;
    }
    if (strcmp((char *)tempBuffer, "True") == 0) {
        strcpy((char *)tempNewText, "False");
        tempHasSetNewText = true;
    }
    if (strcmp((char *)tempBuffer, "TRUE") == 0) {
        strcpy((char *)tempNewText, "FALSE");
        tempHasSetNewText = true;
    }
    if (!tempHasSetNewText) {
        return;
    }
    int64_t tempLength3 = strlen((char *)tempNewText);
    addHistoryFrame();
    int64_t tempRowCount1 = getTextLineRowCount(cursorTextPos.line);
    recordTextLineDeleted(cursorTextPos.line);
    eraseCursor();
    removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), tempStartIndex, tempLength2);
    insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempStartIndex, tempNewText, tempLength3);
    recordTextLineInserted(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, tempStartIndex);
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

