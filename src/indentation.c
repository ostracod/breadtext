
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "indentation.h"
#include "breadtext.h"

int32_t getTextLineIndentationLevel(textLine_t *line) {
    int32_t output = 0;
    int8_t tempSpaceCount = 0;
    int64_t tempLength = line->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = line->textAllocation.text[index];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                output += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            output += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        index += 1;
    }
    return output;
}

int64_t getTextLineIndentationEndIndex(textLine_t *line) {
    int64_t tempLength = line->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = line->textAllocation.text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    return index;
}

void decreaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory) {
    int32_t tempOldLevel = getTextLineIndentationLevel(line);
    int64_t tempStartIndex = 0;
    int64_t tempEndIndex = getTextLineIndentationEndIndex(line);
    int32_t tempLevel = 0;
    int8_t tempSpaceCount = 0;
    while (tempStartIndex < tempEndIndex) {
        if (tempLevel >= tempOldLevel - 1) {
            break;
        }
        int8_t tempCharacter = line->textAllocation.text[tempStartIndex];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                tempLevel += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            tempLevel += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        tempStartIndex += 1;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    if (tempAmount > 0) {
        if (shouldRecordHistory) {
            recordTextLineDeleted(line);
        }
        removeTextFromTextAllocation(&(line->textAllocation), tempStartIndex, tempAmount);
        if (shouldRecordHistory) {
            recordTextLineInserted(line);
        }
    }
    int64_t tempOffset = -tempAmount;
    if (line == cursorTextPos.line) {
        int64_t index = getTextPosIndex(&cursorTextPos);
        index += tempOffset;
        if (index < 0) {
            index = 0;
        }
        setTextPosIndex(&cursorTextPos, index);
    }
    if (isHighlighting) {
        if (line == highlightTextPos.line) {
            int64_t index = getTextPosIndex(&highlightTextPos);
            index += tempOffset;
            if (index < 0) {
                index = 0;
            }
            setTextPosIndex(&highlightTextPos, index);
        }
    }
}

void increaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory) {
    int32_t tempOldLevel = getTextLineIndentationLevel(line);
    int64_t tempStartIndex = 0;
    int64_t tempEndIndex = getTextLineIndentationEndIndex(line);
    int32_t tempLevel = 0;
    int8_t tempSpaceCount = 0;
    while (tempStartIndex < tempEndIndex) {
        if (tempLevel >= tempOldLevel) {
            break;
        }
        int8_t tempCharacter = line->textAllocation.text[tempStartIndex];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                tempLevel += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            tempLevel += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        tempStartIndex += 1;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    int8_t tempIndentation[100];
    int8_t tempIndentationLength;
    if (shouldUseHardTabs) {
        tempIndentation[0] = '\t';
        tempIndentationLength = 1;
    } else {
        int8_t index = 0;
        while (index < indentationWidth) {
            tempIndentation[index] = ' ' ;
            index += 1;
        }
        tempIndentationLength = indentationWidth;
    }
    if (shouldRecordHistory) {
        recordTextLineDeleted(line);
    }
    if (tempAmount > 0) {
        removeTextFromTextAllocation(&(line->textAllocation), tempStartIndex, tempAmount);
    }
    insertTextIntoTextAllocation(&(line->textAllocation), tempStartIndex, tempIndentation, tempIndentationLength);
    if (shouldRecordHistory) {
        recordTextLineInserted(line);
    }
    int64_t tempOffset = tempIndentationLength - tempAmount;
    if (line == cursorTextPos.line) {
        int64_t index = getTextPosIndex(&cursorTextPos);
        index += tempOffset;
        if (index < 0) {
            index = 0;
        }
        setTextPosIndex(&cursorTextPos, index);
    }
    if (isHighlighting) {
        if (line == highlightTextPos.line) {
            int64_t index = getTextPosIndex(&highlightTextPos);
            index += tempOffset;
            if (index < 0) {
                index = 0;
            }
            setTextPosIndex(&highlightTextPos, index);
        }
    }
}

int64_t getIndentationWidth(int64_t level) {
    if (shouldUseHardTabs) {
        return level;
    }
    return level * indentationWidth;
}

void increaseSelectionIndentationLevel() {
    addHistoryFrame();
    textPos_t *tempStartTextPos;
    textPos_t *tempEndTextPos;
    if (isHighlighting) {
        tempStartTextPos = getFirstHighlightTextPos();
        tempEndTextPos = getLastHighlightTextPos();
    } else {
        tempStartTextPos = &cursorTextPos;
        tempEndTextPos = &cursorTextPos;
    }
    eraseCursor();
    textLine_t *tempLine = tempStartTextPos->line;
    int8_t tempIsSingleLine = tempStartTextPos->line == tempEndTextPos->line;
    if (tempIsSingleLine) {
        int64_t tempOldRowCount = getTextLineRowCount(tempLine);
        increaseTextLineIndentationLevelHelper(tempLine, true);
        int64_t tempNewRowCount = getTextLineRowCount(tempLine);
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (tempOldRowCount == tempNewRowCount) {
                displayTextLine(getTextLinePosY(tempLine), tempLine);
            } else {
                displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempLine), tempLine);
            }
            displayCursor();
        }
    } else {
        while (true) {
            increaseTextLineIndentationLevelHelper(tempLine, true);
            if (tempLine == tempEndTextPos->line) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
        }
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempStartTextPos->line), tempStartTextPos->line);
            displayCursor();
        }
    }
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void decreaseSelectionIndentationLevel() {
    addHistoryFrame();
    textPos_t *tempStartTextPos;
    textPos_t *tempEndTextPos;
    if (isHighlighting) {
        tempStartTextPos = getFirstHighlightTextPos();
        tempEndTextPos = getLastHighlightTextPos();
    } else {
        tempStartTextPos = &cursorTextPos;
        tempEndTextPos = &cursorTextPos;
    }
    eraseCursor();
    textLine_t *tempLine = tempStartTextPos->line;
    int8_t tempIsSingleLine = tempStartTextPos->line == tempEndTextPos->line;
    if (tempIsSingleLine) {
        int64_t tempOldRowCount = getTextLineRowCount(tempLine);
        decreaseTextLineIndentationLevelHelper(tempLine, true);
        int64_t tempNewRowCount = getTextLineRowCount(tempLine);
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (tempOldRowCount == tempNewRowCount) {
                displayTextLine(getTextLinePosY(tempLine), tempLine);
            } else {
                displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempLine), tempLine);
            }
            displayCursor();
        }
    } else {
        while (true) {
            decreaseTextLineIndentationLevelHelper(tempLine, true);
            if (tempLine == tempEndTextPos->line) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
        }
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempStartTextPos->line), tempStartTextPos->line);
            displayCursor();
        }
    }
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}



