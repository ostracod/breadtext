
#include <stdio.h>
#include <stdlib.h>
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
