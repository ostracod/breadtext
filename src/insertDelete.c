
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "motion.h"
#include "history.h"
#include "indentation.h"
#include "insertDelete.h"
#include "breadtext.h"

void insertCharacterBeforeCursor(int8_t character) {
    int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (isStartOfNonconsecutiveEscapeSequence) {
        if (firstNonconsecutiveEscapeSequenceAction.text != NULL) {
            cleanUpHistoryAction(&firstNonconsecutiveEscapeSequenceAction);
        }
        firstNonconsecutiveEscapeSequenceAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
        nonconsecutiveEscapeSequencePreviousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
    } else {
        if (lastIsStartOfNonconsecutiveEscapeSequence) {
            addNonconsecutiveEscapeSequenceAction(false);
        } else if (!historyFrameIsConsecutive) {
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
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceAction(true);
    }
    index -= 1;
    if (index < 0) {
        textLine_t *tempLine = getPreviousTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        if (shouldRecordHistory) {
            addHistoryFrame();
        }
        addHistoryFrame();
        index = tempLine->textAllocation.length;
        if (!textLineOnlyContainsWhitespace(cursorTextPos.line)) {
            if (shouldRecordHistory) {
                recordTextLineDeleted(tempLine);
            }
            insertTextIntoTextAllocation(&(tempLine->textAllocation), index, cursorTextPos.line->textAllocation.text, cursorTextPos.line->textAllocation.length);
            if (shouldRecordHistory) {
                recordTextLineInserted(tempLine);
            }
        }
        if (shouldRecordHistory) {
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

void deleteCharacterAfterCursor(int8_t shouldAddHistoryFrame) {
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (index >= tempLength) {
        if (shouldAddHistoryFrame) {
            addHistoryFrame();
        }
        textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        if (!textLineOnlyContainsWhitespace(tempLine)) {        
            recordTextLineDeleted(cursorTextPos.line);
            insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempLength, tempLine->textAllocation.text, tempLine->textAllocation.length);
            recordTextLineInserted(cursorTextPos.line);
        }
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
            if (shouldAddHistoryFrame) {
                addHistoryFrame();
            }
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
    if (shouldAddHistoryFrame) {
        finishCurrentHistoryFrame();
    }
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
    if (activityMode != TEXT_ENTRY_MODE && activityMode != COMMAND_MODE) {
        setActivityMode(COMMAND_MODE);
    }
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
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceAction(true);
    }
    addHistoryFrame();
    int32_t tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    insertNewlineBeforeCursorHelper(tempLevel);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void promptAndInsertCharacterBeforeCursor() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter != 0) {
        insertCharacterBeforeCursor(tempCharacter);
    }
}

void promptAndInsertCharacterAfterCursor() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter != 0) {
        moveCursorRight(1);
        insertCharacterBeforeCursor(tempCharacter);
        moveCursorLeft(2);
    }
}

void promptAndReplaceCharacterUnderCursor() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter != 0) {
        moveCursorRight(1);
        deleteCharacterBeforeCursor(true);
        insertCharacterBeforeCursor(tempCharacter);
        moveCursorLeft(1);
    }
}

void insertLineBeforeCursor() {
    addHistoryFrame();
    textLine_t *tempLine = createEmptyTextLine();
    int32_t tempIndentationLevel = getTextLineIndentationLevel(cursorTextPos.line);
    int32_t tempCount = 0;
    while (tempCount < tempIndentationLevel) {
        increaseTextLineIndentationLevelHelper(tempLine, false);
        tempCount += 1;
    }
    insertTextLineLeft(cursorTextPos.line, tempLine);
    recordTextLineInserted(tempLine);
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempPosY = getTextLinePosY(tempLine);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, tempLine);
        displayCursor();
    }
    eraseLineNumber();
    displayLineNumber();
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
}

void insertLineAfterCursor() {
    addHistoryFrame();
    textLine_t *tempLine = createEmptyTextLine();
    int32_t tempIndentationLevel = getTextLineIndentationLevel(cursorTextPos.line);
    int32_t tempCount = 0;
    while (tempCount < tempIndentationLevel) {
        increaseTextLineIndentationLevelHelper(tempLine, false);
        tempCount += 1;
    }
    insertTextLineRight(cursorTextPos.line, tempLine);
    recordTextLineInserted(tempLine);
    int64_t tempPosY = getTextLinePosY(tempLine);
    displayTextLinesUnderAndIncludingTextLine(tempPosY, tempLine);
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
}

void insertAndEditLineBeforeCursor() {
    insertLineBeforeCursor();
    eraseCursor();
    cursorTextPos.line = getPreviousTextLine(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, cursorTextPos.line->textAllocation.length);
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayCursor();
    }
    setActivityMode(TEXT_ENTRY_MODE);
}

void insertAndEditLineAfterCursor() {
    insertLineAfterCursor();
    eraseCursor();
    cursorTextPos.line = getNextTextLine(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, cursorTextPos.line->textAllocation.length);
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayCursor();
    }
    setActivityMode(TEXT_ENTRY_MODE);
}



