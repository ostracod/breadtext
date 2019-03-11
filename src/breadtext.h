
#include "textLine.h"
#include "textPos.h"

#ifndef BREADTEXT_HEADER_FILE
#define BREADTEXT_HEADER_FILE

#define COMMAND_MODE 1
#define TEXT_ENTRY_MODE 2
#define HIGHLIGHT_CHARACTER_MODE 3
#define HIGHLIGHT_STATIC_MODE 4
#define HIGHLIGHT_LINE_MODE 5
#define TEXT_COMMAND_MODE 6
#define HELP_MODE 7
#define PREVIOUS_MODE 8
#define TEXT_REPLACE_MODE 9

#define SINGLE_CHARACTER_ACTION_NONE 0
#define SINGLE_CHARACTER_ACTION_INSERT_BEFORE 1
#define SINGLE_CHARACTER_ACTION_INSERT_AFTER 2
#define SINGLE_CHARACTER_ACTION_REPLACE 3
#define SINGLE_CHARACTER_ACTION_GOTO_INCLUSIVE 4
#define SINGLE_CHARACTER_ACTION_GOTO_EXCLUSIVE 5
#define SINGLE_CHARACTER_ACTION_REVERSE_GOTO_INCLUSIVE 6
#define SINGLE_CHARACTER_ACTION_REVERSE_GOTO_EXCLUSIVE 7
#define SINGLE_CHARACTER_ACTION_ENCLOSURE_INCLUSIVE 8
#define SINGLE_CHARACTER_ACTION_ENCLOSURE_EXCLUSIVE 9
#define SINGLE_CHARACTER_ACTION_HIGHLIGHT_WORD 10

#define TIME_NEVER -1

#define SHOULD_RUN_TEXT_LINE_TEST false
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
int8_t hasLastDisplayHighlight;
textPos_t lastDisplayCursorTextPos;
textPos_t lastDisplayHighlightTextPos;
int8_t textCommandBuffer[1000];
int32_t textCommandCursorIndex;
int32_t helpScroll;
int8_t *filePath;
int8_t *rcFilePath;
int8_t *rcScriptFilePath;
int64_t fileLastModifiedTime;
int8_t shouldUseSystemClipboard;
int8_t isPerformingFuzzTest;
int8_t isPerformingSystematicTest;
int8_t isPerformingScriptTest;
int8_t shouldHighlightSyntax;
int8_t applicationVersion[50];
int8_t shouldUseXclip;
int8_t lastCharacterDeletedByTextReplaceMode;
int8_t isConsumingSingleCharacter;
int8_t singleCharacterAction;

int8_t equalTextPos(textPos_t *pos1, textPos_t *pos2);
int64_t getTextPosIndex(textPos_t *pos);
void setTextPosIndex(textPos_t *pos, int64_t index);
int8_t textPosIsAfterTextPos(textPos_t *textPos1, textPos_t *textPos2);
void handleTextLineDeleted(textLine_t *lineToBeDeleted);
void setActivityMode(int8_t mode);
int8_t getConfigurationVariable(int64_t *destination, int8_t *name);
int8_t setConfigurationVariable(int8_t *name, int64_t value);
void storeInitialFileContents();
void clearInitialFileContents();
int8_t checkInitialFileContents();
int8_t checkTextBufferHygiene();
void addNonconsecutiveEscapeSequenceFrame();
void promptSingleCharacter();
void handleResize();
int8_t handleKey(int32_t key, int8_t shouldUseMappings, int8_t shouldUseBindings, int8_t shouldRecordInMacro);
void resetApplication();
int8_t isTesting();

// BREADTEXT_HEADER_FILE
#endif
