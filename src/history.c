
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "history.h"
#include "breadtext.h"

textPos_t convertHistoryTextPosToTextPos(historyTextPos_t *pos) {
    textPos_t output;
    output.line = getTextLineByNumber(pos->lineNumber);
    setTextPosIndex(&output, pos->index);
    return output;
}

historyTextPos_t convertTextPosToHistoryTextPos(textPos_t *pos) {
    historyTextPos_t output;
    output.lineNumber = getTextLineNumber(pos->line);
    output.index = getTextPosIndex(pos);
    return output;
}

void performHistoryAction(historyAction_t *action) {
    if (action->type == HISTORY_ACTION_INSERT) {
        textLine_t *tempLine = createEmptyTextLine();
        insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, action->text, action->length);
        if (rootTextLine == NULL) {
            rootTextLine = tempLine;
            topTextLine = tempLine;
            topTextLineRow = 0;
        } else if (action->lineNumber == 1) {
            textLine_t *tempLine2 = getTextLineByNumber(1);
            insertTextLineLeft(tempLine2, tempLine);
        } else {
            textLine_t *tempLine2 = getTextLineByNumber(action->lineNumber - 1);
            insertTextLineRight(tempLine2, tempLine);
        }
    }
    if (action->type == HISTORY_ACTION_DELETE) {
        textLine_t *tempLine = getTextLineByNumber(action->lineNumber);
        handleTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
    }
}

void undoHistoryAction(historyAction_t *action) {
    if (action->type == HISTORY_ACTION_INSERT) {
        textLine_t *tempLine = getTextLineByNumber(action->lineNumber);
        handleTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
    }
    if (action->type == HISTORY_ACTION_DELETE) {
        textLine_t *tempLine = createEmptyTextLine();
        insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, action->text, action->length);
        if (rootTextLine == NULL) {
            rootTextLine = tempLine;
            topTextLine = tempLine;
            topTextLineRow = 0;
        } else if (action->lineNumber == 1) {
            textLine_t *tempLine2 = getTextLineByNumber(1);
            insertTextLineLeft(tempLine2, tempLine);
        } else {
            textLine_t *tempLine2 = getTextLineByNumber(action->lineNumber - 1);
            insertTextLineRight(tempLine2, tempLine);
        }
    }
}

void cleanUpHistoryAction(historyAction_t *action) {
    if (action->text != NULL) {
        free(action->text);
    }
}

historyFrame_t createEmptyHistoryFrame() {
    historyFrame_t output;
    output.historyActionList = malloc(0);
    output.length = 0;
    output.allocationSize = 0;
    output.previousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
    return output;
}

void addHistoryActionToHistoryFrame(historyFrame_t *frame, historyAction_t *action) {
    int64_t tempOldSize = frame->length * sizeof(historyAction_t);
    int64_t index = frame->length;
    frame->length += 1;
    int64_t tempSize = frame->length * sizeof(historyAction_t);
    if (tempSize > frame->allocationSize) {
        frame->allocationSize = tempSize * 2;
        historyAction_t *tempList = malloc(frame->allocationSize);
        copyData((int8_t *)tempList, (int8_t *)(frame->historyActionList), tempOldSize);
        free(frame->historyActionList);
        frame->historyActionList = tempList;
    }
    *(frame->historyActionList + index) = *action;
}

void performHistoryFrame(historyFrame_t *frame) {
    int64_t index = 0;
    while (index < frame->length) {
        historyAction_t *tempAction = frame->historyActionList + index;
        performHistoryAction(tempAction);
        index += 1;
    }
    cursorTextPos = convertHistoryTextPosToTextPos(&(frame->nextCursorTextPos));
    setActivityMode(COMMAND_MODE);
    int8_t tempResult = scrollCursorOntoScreen();
    if (tempResult) {
        displayStatusBar();
    } else {
        redrawEverything();
    }
    textBufferIsDirty = true;
}

void undoHistoryFrame(historyFrame_t *frame) {
    int64_t index = frame->length - 1;
    while (index >= 0) {
        historyAction_t *tempAction = frame->historyActionList + index;
        undoHistoryAction(tempAction);
        index -= 1;
    }
    cursorTextPos = convertHistoryTextPosToTextPos(&(frame->previousCursorTextPos));
    setActivityMode(COMMAND_MODE);
    int8_t tempResult = scrollCursorOntoScreen();
    if (tempResult) {
        displayStatusBar();
    } else {
        redrawEverything();
    }
    textBufferIsDirty = true;
}

void cleanUpHistoryFrame(historyFrame_t *frame) {
    int64_t index = 0;
    while (index < frame->length) {
        historyAction_t *tempAction = frame->historyActionList + index;
        cleanUpHistoryAction(tempAction);
        index += 1;
    }
    if (frame->historyActionList != NULL) {
        free(frame->historyActionList);
    }
}

void addHistoryFrame() {
    int32_t index = 0;
    while (index < historyFrameListIndex) {
        historyFrame_t *tempFrame = historyFrameList + index;
        cleanUpHistoryFrame(tempFrame);
        index += 1;
    }
    if (historyFrameListIndex > 1) {
        int32_t index = 1;
        while (historyFrameListIndex < historyFrameListLength) {
            historyFrameList[index] = historyFrameList[historyFrameListIndex];
            index += 1;
            historyFrameListIndex += 1;
        }
        historyFrameListLength = index;
    } else if (historyFrameListIndex == 0) {
        int32_t index = historyFrameListLength - 1;
        if (index + 1 >= MAXIMUM_HISTORY_DEPTH) {
            index = MAXIMUM_HISTORY_DEPTH - 2;
            historyFrameListLength = MAXIMUM_HISTORY_DEPTH;
        } else {
            historyFrameListLength += 1;
        }
        while (index >= 0) {
            historyFrameList[index + 1] = historyFrameList[index];
            index -= 1;
        }
    }
    historyFrameListIndex = 0;
    historyFrameList[historyFrameListIndex] = createEmptyHistoryFrame();
}

historyAction_t createHistoryActionFromTextLine(textLine_t *line, int8_t actionType) {
    historyAction_t output;
    output.type = actionType;
    output.lineNumber = getTextLineNumber(line);
    int64_t tempLength = line->textAllocation.length;
    output.text = malloc(tempLength);
    copyData(output.text, line->textAllocation.text, tempLength);
    output.length = tempLength;
    return output;
}

void recordTextLine(textLine_t *line, int8_t actionType) {
    historyAction_t tempAction = createHistoryActionFromTextLine(line, actionType);
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    addHistoryActionToHistoryFrame(tempFrame, &tempAction);
}

void recordTextLineInserted(textLine_t *line) {
    recordTextLine(line, HISTORY_ACTION_INSERT);
}

void recordTextLineDeleted(textLine_t *line) {
    recordTextLine(line, HISTORY_ACTION_DELETE);
}

void updateHistoryFrameInsertAction(textLine_t *line) {
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    int64_t index = 0;
    while (index < tempFrame->length) {
        historyAction_t *tempAction = tempFrame->historyActionList + index;
        if (tempAction->type == HISTORY_ACTION_INSERT) {
            int64_t tempLength = line->textAllocation.length;
            if (tempAction->text != NULL) {
                free(tempAction->text);
            }
            tempAction->text = malloc(tempLength);
            copyData(tempAction->text, line->textAllocation.text, tempLength);
            tempAction->length = tempLength;
            break;
        }
        index += 1;
    }
}

void finishCurrentHistoryFrame() {
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    tempFrame->nextCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
}

void undoLastAction() {
    if (historyFrameListIndex >= historyFrameListLength) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"At oldest state.");
        return;
    }
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    undoHistoryFrame(tempFrame);
    historyFrameListIndex += 1;
}

void redoLastAction() {
    if (historyFrameListIndex <= 0) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"At newest state.");
        return;
    }
    historyFrameListIndex -= 1;
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    performHistoryFrame(tempFrame);
}
