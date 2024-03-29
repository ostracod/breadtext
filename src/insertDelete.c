
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

void insertCharacterBeforeCursorHelper(int8_t character) {
    int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
    int64_t index = getTextPosIndex(&cursorTextPos);
    insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), index, &character, 1);
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
}

void insertCharacterBeforeCursor(int8_t character, int8_t isConsecutive) {
    lastIsStartOfNonconsecutiveEscapeSequence = isStartOfNonconsecutiveEscapeSequence;
    isStartOfNonconsecutiveEscapeSequence = false;
    if (!isConsecutive) {
        historyFrameIsConsecutive = false;
    }
    if (lastIsStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceFrame();
    }
    if (historyFrameIsConsecutive) {
        insertCharacterBeforeCursorHelper(character);
        updateHistoryFrameInsertAction(cursorTextPos.line);
    } else {
        addHistoryFrame();
        recordTextLineDeleted(cursorTextPos.line);
        insertCharacterBeforeCursorHelper(character);
        recordTextLineInserted(cursorTextPos.line);
        if (isConsecutive) {
            historyFrameIsConsecutive = true;
        }
    }
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
}

void insertTextEntryModeCharacterBeforeCursor(int8_t character) {
    lastIsStartOfNonconsecutiveEscapeSequence = isStartOfNonconsecutiveEscapeSequence;
    isStartOfNonconsecutiveEscapeSequence = (character == ',' && !historyFrameIsConsecutive);
    if (isStartOfNonconsecutiveEscapeSequence) {
        if (nonconsecutiveEscapeSequenceFrameIsSet) {
            cleanUpHistoryFrame(&nonconsecutiveEscapeSequenceFrame);
        }
        nonconsecutiveEscapeSequenceFrame = createEmptyHistoryFrame();
        historyAction_t tempAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
        addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction);
        nonconsecutiveEscapeSequenceFrame.previousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
        nonconsecutiveEscapeSequenceFrameIsSet = true;
        historyFrameIsConsecutive = true;
    } else {
        if (lastIsStartOfNonconsecutiveEscapeSequence) {
            addNonconsecutiveEscapeSequenceFrame();
        } else if (!historyFrameIsConsecutive) {
            addHistoryFrame();
            recordTextLineDeleted(cursorTextPos.line);
        }
    }
    insertCharacterBeforeCursorHelper(character);
    if (isStartOfNonconsecutiveEscapeSequence) {
        historyAction_t tempAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_INSERT);
        addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction);
    } else {
        if (historyFrameIsConsecutive) {
            updateHistoryFrameInsertAction(cursorTextPos.line);
        } else {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = true;
        }
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void deleteCharacterBeforeCursorHelper(int8_t shouldAddHistoryFrame, int8_t shouldRecordHistory) {
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (index > 0 && index == getTextLineIndentationEndIndex(cursorTextPos.line)) {
        decreaseSelectionIndentationLevel();
        return;
    }
    if (shouldRecordHistory) {
        if (isStartOfNonconsecutiveEscapeSequence) {
            addNonconsecutiveEscapeSequenceFrame();
        }
    }
    index -= 1;
    if (index < 0) {
        textLine_t *tempLine = getPreviousTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        if (shouldRecordHistory && shouldAddHistoryFrame) {
            addHistoryFrame();
        }
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
        redrawLineNumber();
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

void deleteCharacterBeforeCursor(int8_t shouldRecordHistory) {
    deleteCharacterBeforeCursorHelper(shouldRecordHistory, shouldRecordHistory);
}

void deleteCharacterAfterCursorHelper(int8_t shouldAddHistoryFrame, int8_t shouldRecordHistory) {
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (index >= tempLength) {
        textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        if (shouldAddHistoryFrame && shouldRecordHistory) {
            addHistoryFrame();
        }
        if (!textLineOnlyContainsWhitespace(tempLine)) {
            if (shouldRecordHistory) {
                recordTextLineDeleted(cursorTextPos.line);
            }
            insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempLength, tempLine->textAllocation.text, tempLine->textAllocation.length);
            if (shouldRecordHistory) {
                recordTextLineInserted(cursorTextPos.line);
            }
        }
        handleTextLineDeleted(tempLine);
        if (shouldRecordHistory) {
            recordTextLineDeleted(tempLine);
        }
        deleteTextLine(tempLine);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        displayCursor();
        redrawLineNumber();
    } else {
        int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
        if (!historyFrameIsConsecutive) {
            if (shouldAddHistoryFrame && shouldRecordHistory) {
                addHistoryFrame();
            }
            if (shouldRecordHistory) {
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
        int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempNewRowCount == tempOldRowCount) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
        displayCursor();
    }
    if (shouldRecordHistory) {
        finishCurrentHistoryFrame();
        textBufferIsDirty = true;
    }
}

void deleteCharacterAfterCursor(int8_t shouldAddHistoryFrame) {
    deleteCharacterAfterCursorHelper(shouldAddHistoryFrame, true);
}

void insertTextReplaceModeCharacter(int8_t character) {
    int8_t isConcatenatingLines = false;
    int64_t index = getTextPosIndex(&cursorTextPos);
    textLine_t *tempNextLine;
    if (index >= cursorTextPos.line->textAllocation.length) {
        tempNextLine = getNextTextLine(cursorTextPos.line);
        isConcatenatingLines = (tempNextLine != NULL);
    }
    lastIsStartOfNonconsecutiveEscapeSequence = isStartOfNonconsecutiveEscapeSequence;
    isStartOfNonconsecutiveEscapeSequence = (character == ',' && !historyFrameIsConsecutive);
    if (isStartOfNonconsecutiveEscapeSequence) {
        if (nonconsecutiveEscapeSequenceFrameIsSet) {
            cleanUpHistoryFrame(&nonconsecutiveEscapeSequenceFrame);
        }
        nonconsecutiveEscapeSequenceFrame = createEmptyHistoryFrame();
        if (isConcatenatingLines) {
            historyAction_t tempAction = createHistoryActionFromTextLine(tempNextLine, HISTORY_ACTION_DELETE);
            addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction);
            historyAction_t tempAction2 = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
            addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction2);
            historyFrameIsConsecutive = false;
        } else {
            historyAction_t tempAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
            addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction);
            historyFrameIsConsecutive = true;
        }
        nonconsecutiveEscapeSequenceFrame.previousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
        nonconsecutiveEscapeSequenceFrameIsSet = true;
    } else {
        if (isConcatenatingLines) {
            addHistoryFrame();
            recordTextLineDeleted(tempNextLine);
            recordTextLineDeleted(cursorTextPos.line);
        } else {
            if (lastIsStartOfNonconsecutiveEscapeSequence) {
                addNonconsecutiveEscapeSequenceFrame();
            } else if (!historyFrameIsConsecutive) {
                addHistoryFrame();
                recordTextLineDeleted(cursorTextPos.line);
            }
        }
    }
    lastCharacterDeletedByTextReplaceMode = getTextPosCharacter(&cursorTextPos);
    deleteCharacterAfterCursorHelper(false, false);
    insertCharacterBeforeCursorHelper(character);
    if (isStartOfNonconsecutiveEscapeSequence) {
        historyAction_t tempAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_INSERT);
        addHistoryActionToHistoryFrame(&nonconsecutiveEscapeSequenceFrame, &tempAction);
    } else {
        if (isConcatenatingLines) {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = false;
        } else {
            if (historyFrameIsConsecutive) {
                updateHistoryFrameInsertAction(cursorTextPos.line);
            } else {
                recordTextLineInserted(cursorTextPos.line);
                historyFrameIsConsecutive = true;
            }
        }
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void trimTextAllocationTrailingWhitespace(textAllocation_t *allocation) {
    int64_t index = allocation->length - 1;
    while (index >= 0) {
        int8_t tempCharacter = allocation->text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index -= 1;
    }
    if (index < 0) {
        return;
    }
    int64_t tempStartIndex = index + 1;
    int64_t tempAmount = allocation->length - tempStartIndex;
    if (tempAmount <= 0) {
        return;
    }
    removeTextFromTextAllocation(allocation, tempStartIndex, tempAmount);
}

void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel, int8_t shouldRecordHistory) {
    textLine_t *tempLine = createEmptyTextLine();
    textLine_t *tempLine2 = cursorTextPos.line;
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempAmount = cursorTextPos.line->textAllocation.length - index;
    insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, cursorTextPos.line->textAllocation.text + index, tempAmount);
    setTextAllocationIndentationLevel(&(tempLine->textAllocation), baseIndentationLevel);
    if (shouldRecordHistory) {
        recordTextLineDeleted(cursorTextPos.line);
    }
    removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, tempAmount);
    trimTextAllocationTrailingWhitespace(&(cursorTextPos.line->textAllocation));
    if (shouldRecordHistory) {
        recordTextLineInserted(cursorTextPos.line);
    }
    insertTextLineRight(cursorTextPos.line, tempLine);
    if (shouldRecordHistory) {
        recordTextLineInserted(tempLine);
    }
    cursorTextPos.line = tempLine;
    int64_t tempIndex = getTextLineIndentationEndIndex(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, tempIndex);
    cursorSnapColumn = cursorTextPos.column;
    if (activityMode != TEXT_ENTRY_MODE && activityMode != TEXT_REPLACE_MODE && activityMode != COMMAND_MODE) {
        setActivityMode(COMMAND_MODE);
    }
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempPosY = getTextLinePosY(tempLine2);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, tempLine2);
        displayCursor();
    }
    redrawLineNumber();
    if (shouldRecordHistory) {
        textBufferIsDirty = true;
    }
}

void insertNewlineBeforeCursor(int8_t shouldIgnoreIndentation) {
    if (isStartOfNonconsecutiveEscapeSequence) {
        addNonconsecutiveEscapeSequenceFrame();
    }
    addHistoryFrame();
    int32_t tempLevel;
    if (shouldIgnoreIndentation) {
        tempLevel = 0;
    } else {
        tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    }
    insertNewlineBeforeCursorHelper(tempLevel, true);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void replaceCharacterAtCursor(int8_t character, int8_t shouldRecordHistory) {
    if (shouldRecordHistory) {
        deleteCharacterAfterCursor(true);
        if (character == '\n') {
            insertNewlineBeforeCursor(true);
        } else {
            insertCharacterBeforeCursor(character, true);
        }
    } else {
        deleteCharacterAfterCursorHelper(false, false);
        if (character == '\n') {
            insertNewlineBeforeCursorHelper(0, false);
        } else {
            insertCharacterBeforeCursorHelper(character);
        }
    }
}

void promptAndInsertCharacterBeforeCursor() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_INSERT_BEFORE;
}

void promptAndInsertCharacterAfterCursor() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_INSERT_AFTER;
}

void promptAndReplaceCharacterUnderCursor() {
    promptSingleCharacter();
    singleCharacterAction = SINGLE_CHARACTER_ACTION_REPLACE;
}

void insertCharacterBeforeCursorSimple(int8_t character) {
    insertCharacterBeforeCursor(character, false);
}

void insertCharacterAfterCursorSimple(int8_t character) {
    moveCursorRight(1);
    insertCharacterBeforeCursor(character, false);
    moveCursorLeft(2);
}

void replaceCharacterUnderCursorSimple(int8_t character) {
    moveCursorRight(1);
    deleteCharacterBeforeCursor(true);
    insertCharacterBeforeCursor(character, false);
    moveCursorLeft(1);
}

textLine_t *insertLineBeforeCursor() {
    addHistoryFrame();
    textLine_t *output = createEmptyTextLine();
    int32_t tempIndentationLevel = getTextLineIndentationLevel(cursorTextPos.line);
    int32_t tempCount = 0;
    while (tempCount < tempIndentationLevel) {
        increaseTextLineIndentationLevelHelper(output, false);
        tempCount += 1;
    }
    insertTextLineLeft(cursorTextPos.line, output);
    recordTextLineInserted(output);
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempPosY = getTextLinePosY(output);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, output);
        displayCursor();
    }
    redrawLineNumber();
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
    return output;
}

textLine_t *insertLineAfterCursor() {
    addHistoryFrame();
    textLine_t *output = createEmptyTextLine();
    int32_t tempIndentationLevel = getTextLineIndentationLevel(cursorTextPos.line);
    int32_t tempCount = 0;
    while (tempCount < tempIndentationLevel) {
        increaseTextLineIndentationLevelHelper(output, false);
        tempCount += 1;
    }
    insertTextLineRight(cursorTextPos.line, output);
    recordTextLineInserted(output);
    int64_t tempPosY = getTextLinePosY(output);
    displayTextLinesUnderAndIncludingTextLine(tempPosY, output);
    finishCurrentHistoryFrame();
    textBufferIsDirty = true;
    return output;
}

void insertAndEditLineHelper(textLine_t *line) {
    setActivityMode(TEXT_ENTRY_MODE);
    eraseCursor();
    cursorTextPos.line = line;
    setTextPosIndex(&cursorTextPos, cursorTextPos.line->textAllocation.length);
    redrawLineNumber();
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayCursor();
    }
}

void insertAndEditLineBeforeCursor() {
    textLine_t *tempLine = insertLineBeforeCursor();
    insertAndEditLineHelper(tempLine);
}

void insertAndEditLineAfterCursor() {
    textLine_t *tempLine = insertLineAfterCursor();
    insertAndEditLineHelper(tempLine);
}

void removeTextLineIndentation(textLine_t *line) {
    int64_t tempAmount = getTextLineIndentationEndIndex(line);
    if (tempAmount <= 0) {
        return;
    }
    recordTextLineDeleted(line);
    removeTextFromTextAllocation(&(line->textAllocation), 0, tempAmount);
    recordTextLineInserted(line);
}

void joinCurrentLineToPreviousLine() {
    textLine_t *tempLine = getPreviousTextLine(cursorTextPos.line);
    if (tempLine == NULL) {
        notifyUser((int8_t *)"Could not join line.");
        return;
    }
    addHistoryFrame();
    removeTextLineIndentation(cursorTextPos.line);
    moveCursorToBeginningOfLine();
    deleteCharacterBeforeCursorHelper(false, true);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
    textBufferIsDirty = true;
}

void joinCurrentLineToNextLine() {
    textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
    if (tempLine == NULL) {
        notifyUser((int8_t *)"Could not join line.");
        return;
    }
    addHistoryFrame();
    removeTextLineIndentation(tempLine);
    moveCursorToEndOfLine();
    deleteCharacterAfterCursorHelper(false, true);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
    textBufferIsDirty = true;
}


