
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "cursorMotion.h"
#include "history.h"
#include "indentation.h"
#include "insertDelete.h"
#include "breadtext.h"

void insertCharacterBeforeCursor(int8_t character) {
    int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (isStartOfNonconsecutiveEscapeSequence) {
        cleanUpHistoryAction(&firstNonconsecutiveEscapeSequenceAction);
        firstNonconsecutiveEscapeSequenceAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
        nonconsecutiveEscapeSequencePreviousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
    } else {
        if (!historyFrameIsConsecutive) {
            addHistoryFrame();
            recordTextLineDeleted(cursorTextPos.line);
        }
    }
    insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), index, &character, 1);
    if (!isStartOfNonconsecutiveEscapeSequence) {
        if (historyFrameIsConsecutive) {
            updateHistoryFrameInsertAction(cursorTextPos.line);
        } else {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = true;
        }
    }
    cursorTextPos.column += 1;
    if (cursorTextPos.column >= viewPortWidth) {
        cursorTextPos.column = 0;
        cursorTextPos.row += 1;
    }
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempNewRowCount == tempOldRowCount) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
        displayCursor();
    }
    if (!isStartOfNonconsecutiveEscapeSequence) {
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void deleteCharacterBeforeCursor(int8_t shouldRecordHistory) {
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (index > 0 && index == getTextLineIndentationEndIndex(cursorTextPos.line)) {
        decreaseSelectionIndentationLevel();
        return;
    }
    index -= 1;
    if (index < 0) {
        if (shouldRecordHistory) {
            addHistoryFrame();
        }
        textLine_t *tempLine = getPreviousTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        addHistoryFrame();
        index = tempLine->textAllocation.length;
        if (shouldRecordHistory) {
            recordTextLineDeleted(tempLine);
        }
        insertTextIntoTextAllocation(&(tempLine->textAllocation), index, cursorTextPos.line->textAllocation.text, cursorTextPos.line->textAllocation.length);
        if (shouldRecordHistory) {
            recordTextLineInserted(tempLine);
            recordTextLineDeleted(cursorTextPos.line);
        }
        handleTextLineDeleted(cursorTextPos.line);
        deleteTextLine(cursorTextPos.line);
        setTextPosIndex(&cursorTextPos, index);
        cursorSnapColumn = cursorTextPos.column;
        if (topTextLine == cursorTextPos.line) {
            topTextLine = tempLine;
        }
        cursorTextPos.line = tempLine;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
            displayCursor();
        }
        eraseLineNumber();
        displayLineNumber();
        if (shouldRecordHistory) {
            historyFrameIsConsecutive = false;
        }
    } else {
        int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
        if (shouldRecordHistory) {
            if (!historyFrameIsConsecutive) {
                addHistoryFrame();
                recordTextLineDeleted(cursorTextPos.line);
            }
        }
        removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, 1);
        if (shouldRecordHistory) {
            if (historyFrameIsConsecutive) {
                updateHistoryFrameInsertAction(cursorTextPos.line);
            } else {
                recordTextLineInserted(cursorTextPos.line);
                historyFrameIsConsecutive = true;
            }
        }
        cursorTextPos.column -= 1;
        if (cursorTextPos.column < 0) {
            cursorTextPos.column = viewPortWidth - 1;
            cursorTextPos.row -= 1;
        }
        cursorSnapColumn = cursorTextPos.column;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
            int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
            if (tempNewRowCount == tempOldRowCount) {
                displayTextLine(tempPosY, cursorTextPos.line);
            } else {
                displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
            }
            displayCursor();
        }
    }
    if (shouldRecordHistory) {
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void deleteCharacterAfterCursor() {
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (index >= tempLength) {
        textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        recordTextLineDeleted(cursorTextPos.line);
        insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempLength, tempLine->textAllocation.text, tempLine->textAllocation.length);
        recordTextLineInserted(cursorTextPos.line);
        handleTextLineDeleted(tempLine);
        recordTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        displayCursor();
        eraseLineNumber();
        displayLineNumber();
    } else {
        int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
        if (!historyFrameIsConsecutive) {
            addHistoryFrame();
            recordTextLineDeleted(cursorTextPos.line);
        }
        removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, 1);
        if (historyFrameIsConsecutive) {
            updateHistoryFrameInsertAction(cursorTextPos.line);
        } else {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = true;
        }
        int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempNewRowCount == tempOldRowCount) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
        displayCursor();
    }
    textBufferIsDirty = true;
    finishCurrentHistoryFrame();
}

void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel) {
    textLine_t *tempLine = createEmptyTextLine();
    textLine_t *tempLine2 = cursorTextPos.line;
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempAmount = cursorTextPos.line->textAllocation.length - index;
    insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, cursorTextPos.line->textAllocation.text + index, tempAmount);
    int32_t tempCount = 0;
    while (tempCount < baseIndentationLevel) {
        increaseTextLineIndentationLevelHelper(tempLine, false);
        tempCount += 1;
    }
    recordTextLineDeleted(cursorTextPos.line);
    removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, tempAmount);
    recordTextLineInserted(cursorTextPos.line);
    insertTextLineRight(cursorTextPos.line, tempLine);
    recordTextLineInserted(tempLine);
    cursorTextPos.line = tempLine;
    int64_t tempIndex = getTextLineIndentationEndIndex(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, tempIndex);
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempPosY = getTextLinePosY(tempLine2);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, tempLine2);
        displayCursor();
    }
    eraseLineNumber();
    displayLineNumber();
    textBufferIsDirty = true;
}

void insertNewlineBeforeCursor() {
    addHistoryFrame();
    int32_t tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    insertNewlineBeforeCursorHelper(tempLevel);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void promptAndInsertCharacterBeforeCursor() {
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Type a character.");
    int32_t tempKey = getch();
    if (tempKey >= 32 && tempKey <= 126) {
        insertCharacterBeforeCursor(tempKey);
    }
    eraseActivityModeOrNotification();
    displayActivityMode();    
}

void promptAndInsertCharacterAfterCursor() {
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Type a character.");
    int32_t tempKey = getch();
    if (tempKey >= 32 && tempKey <= 126) {
        moveCursorRight(1);
        insertCharacterBeforeCursor(tempKey);
        moveCursorLeft(2);
    }
    eraseActivityModeOrNotification();
    displayActivityMode();
}

void promptAndReplaceCharacterUnderCursor() {
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Type a character.");
    int32_t tempKey = getch();
    if (tempKey >= 32 && tempKey <= 126) {
        moveCursorRight(1);
        deleteCharacterBeforeCursor(true);
        insertCharacterBeforeCursor(tempKey);
        moveCursorLeft(1);
    }
    eraseActivityModeOrNotification();
    displayActivityMode();
}


