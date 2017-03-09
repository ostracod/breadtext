
#include "textLine.h"

#ifndef BREADTEXT_HEADER_FILE
#define BREADTEXT_HEADER_FILE

#define BLACK_ON_WHITE 1
#define WHITE_ON_BLACK 2

#define COMMAND_MODE 1
#define TEXT_ENTRY_MODE 2
#define HIGHLIGHT_CHARACTER_MODE 3
#define HIGHLIGHT_WORD_MODE 4
#define HIGHLIGHT_LINE_MODE 5
#define TEXT_COMMAND_MODE 6
#define HELP_MODE 7
#define PREVIOUS_MODE 8

#define TIME_NEVER -1

#define SHOULD_RUN_TESTS false
#define IS_IN_DEBUG_MODE false

textPos_t cursorTextPos;
int64_t cursorSnapColumn;
int8_t activityMode;
int8_t textBufferIsDirty;
int8_t previousActivityMode;
int32_t activityModeTextLength;
int32_t lineNumberTextLength;
int8_t isShowingNotification;
int32_t notificationTextLength;
int8_t isHighlighting;
textPos_t highlightTextPos;
int8_t textCommandBuffer[1000];
int32_t helpScroll;
int8_t *filePath;
int8_t *rcFilePath;
int64_t fileLastModifiedTime;

int8_t equalTextPos(textPos_t *pos1, textPos_t *pos2);
int64_t getTextPosIndex(textPos_t *pos);
void setTextPosIndex(textPos_t *pos, int64_t index);
int8_t textPosIsAfterTextPos(textPos_t *textPos1, textPos_t *textPos2);
void handleTextLineDeleted(textLine_t *lineToBeDeleted);
void setActivityMode(int8_t mode);
int8_t setConfigurationVariable(int8_t *name, int64_t value);
void storeInitialFileContents();
void clearInitialFileContents();
int8_t checkInitialFileContents();
int8_t checkTextBufferHygiene();
void addNonconsecutiveEscapeSequenceAction(int8_t shouldFinishFrame);

// BREADTEXT_HEADER_FILE
#endif
