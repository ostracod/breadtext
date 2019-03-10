
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "insertDelete.h"
#include "selection.h"
#include "indentation.h"
#include "breadtext.h"

// Set length to -1 for null or newline terminated text.
// Returns -1 if there is no next indentation level.
int64_t seekNextIndentationLevel(int8_t *text, int64_t length, int64_t index) {
    int8_t tempSpaceCount = 0;
    while (index < length || length < 0) {
        int8_t tempCharacter = text[index];
        index += 1;
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                return index;
            }
        } else if (tempCharacter == '\t') {
            return index;
        } else {
            return -1;
        }
    }
    return -1;
}

// Set length to -1 for null or newline terminated text.
int32_t getTextIndentationLevel(int8_t *text, int64_t length) {
    int32_t output = 0;
    int64_t index = 0;
    while (true) {
        index = seekNextIndentationLevel(text, length, index);
        if (index < 0) {
            break;
        }
        output += 1;
    }
    return output;
}

int32_t getTextLineIndentationLevel(textLine_t *line) {
    return getTextIndentationLevel(
        line->textAllocation.text,
        line->textAllocation.length
    );
}

// Set length to -1 for null or newline terminated text.
int64_t getTextIndentationEndIndex(int8_t *text, int64_t length) {
    int64_t index = 0;
    while (index < length || length < 0) {
        int8_t tempCharacter = text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    return index;
}

int64_t getTextLineIndentationEndIndex(textLine_t *line) {
    return getTextIndentationEndIndex(line->textAllocation.text, line->textAllocation.length);
}

int64_t decreaseTextAllocationIndentationLevel(textAllocation_t *allocation) {
    int64_t tempEndIndex = getTextIndentationEndIndex(allocation->text, allocation->length);
    if (tempEndIndex == 0) {
        return 0;
    }
    int32_t tempOldLevel = getTextIndentationLevel(allocation->text, allocation->length);
    int64_t tempStartIndex;
    if (tempOldLevel == 0) {
        tempStartIndex = 0;
    } else {
        int32_t tempLevel = 0;
        int64_t index = 0;
        while (tempLevel < tempOldLevel - 1) {
            index = seekNextIndentationLevel(allocation->text, allocation->length, index);
            if (index < 0) {
                break;
            }
            tempLevel += 1;
        }
        tempStartIndex = index;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    removeTextFromTextAllocation(allocation, tempStartIndex, tempAmount);
    return tempAmount;
}

int64_t increaseTextAllocationIndentationLevel(textAllocation_t *allocation) {
    int64_t tempEndIndex = getTextIndentationEndIndex(allocation->text, allocation->length);
    int32_t tempOldLevel = getTextIndentationLevel(allocation->text, allocation->length);
    int32_t tempLevel = 0;
    int64_t index = 0;
    while (tempLevel < tempOldLevel) {
        index = seekNextIndentationLevel(allocation->text, allocation->length, index);
        if (index < 0) {
            break;
        }
        tempLevel += 1;
    }
    int64_t tempStartIndex = index;
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
    if (tempAmount > 0) {
        removeTextFromTextAllocation(allocation, tempStartIndex, tempAmount);
    }
    insertTextIntoTextAllocation(allocation, tempStartIndex, tempIndentation, tempIndentationLength);
    return tempIndentationLength - tempAmount;
}

void decreaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory) {
    int32_t tempOldLevel = getTextLineIndentationLevel(line);
    if (tempOldLevel <= 0) {
        return;
    }
    if (shouldRecordHistory) {
        recordTextLineDeleted(line);
    }
    int64_t tempOffset = -decreaseTextAllocationIndentationLevel(&(line->textAllocation));
    if (shouldRecordHistory) {
        recordTextLineInserted(line);
    }
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
    cursorSnapColumn = cursorTextPos.column;
    textBufferIsDirty = true;
}

void increaseTextLineIndentationLevelHelper(textLine_t *line, int8_t shouldRecordHistory) {
    if (shouldRecordHistory) {
        recordTextLineDeleted(line);
    }
    int64_t tempOffset = increaseTextAllocationIndentationLevel(&(line->textAllocation));
    if (shouldRecordHistory) {
        recordTextLineInserted(line);
    }
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
    cursorSnapColumn = cursorTextPos.column;
    textBufferIsDirty = true;
}

int64_t getIndentationWidth(int64_t level) {
    if (shouldUseHardTabs) {
        return level;
    }
    return level * indentationWidth;
}

// Returns the new character offset after indentation.
int64_t setTextAllocationIndentationLevel(textAllocation_t *allocation, int32_t level) {
    int64_t output = 0;
    int32_t tempLevel = getTextIndentationLevel(allocation->text, allocation->length);
    while (tempLevel > level) {
        output -= decreaseTextAllocationIndentationLevel(allocation);
        tempLevel -= 1;
    }
    while (tempLevel < level) {
        output += increaseTextAllocationIndentationLevel(allocation);
        tempLevel += 1;
    }
    return output;
}

void increaseSelectionIndentationLevel() {
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceFrame();
    }
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
        fixLineSelection();
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
        fixLineSelection();
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
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceFrame();
    }
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
        fixLineSelection();
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
        fixLineSelection();
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempStartTextPos->line), tempStartTextPos->line);
            displayCursor();
        }
    }
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}



