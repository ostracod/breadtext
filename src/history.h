
#include "breadtext.h"

#ifndef HISTORY_HEADER_FILE
#define HISTORY_HEADER_FILE

#define HISTORY_ACTION_INSERT 1
#define HISTORY_ACTION_DELETE 2

#define MAXIMUM_HISTORY_DEPTH 300
#define MAXIMUM_MACRO_LENGTH 100

typedef struct historyTextPos {
    int64_t lineNumber;
    int64_t index;
} historyTextPos_t;

typedef struct historyAction {
    int8_t type;
    int64_t lineNumber;
    int8_t *text;
    int64_t length;
} historyAction_t;

typedef struct historyFrame {
    historyAction_t *historyActionList;
    int64_t length;
    int64_t allocationSize;
    historyTextPos_t previousCursorTextPos;
    historyTextPos_t nextCursorTextPos;
} historyFrame_t;

historyFrame_t historyFrameList[MAXIMUM_HISTORY_DEPTH];
int32_t historyFrameListIndex;
int32_t historyFrameListLength;
int8_t historyFrameIsConsecutive;

textPos_t convertHistoryTextPosToTextPos(historyTextPos_t *pos);
historyTextPos_t convertTextPosToHistoryTextPos(textPos_t *pos);
void performHistoryAction(historyAction_t *action);
void undoHistoryAction(historyAction_t *action);
void cleanUpHistoryAction(historyAction_t *action);
historyFrame_t createEmptyHistoryFrame();
void addHistoryActionToHistoryFrame(historyFrame_t *frame, historyAction_t *action);
void performHistoryFrame(historyFrame_t *frame);
void undoHistoryFrame(historyFrame_t *frame);
void cleanUpHistoryFrame(historyFrame_t *frame);
void addHistoryFrame();
historyAction_t createHistoryActionFromTextLine(textLine_t *line, int8_t actionType);
void recordTextLine(textLine_t *line, int8_t actionType);
void recordTextLineInserted(textLine_t *line);
void recordTextLineDeleted(textLine_t *line);
void updateHistoryFrameInsertAction(textLine_t *line);
void finishCurrentHistoryFrame();
void undoLastAction();
void redoLastAction();

// HISTORY_HEADER_FILE
#endif
