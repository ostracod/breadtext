
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textLineTest.h"
#include "textPos.h"
#include "history.h"
#include "display.h"
#include "cursorMotion.h"
#include "indentation.h"
#include "insertDelete.h"
#include "selection.h"
#include "manipulation.h"
#include "textCommand.h"
#include "breadtext.h"

int32_t macroKeyList[MAXIMUM_MACRO_LENGTH];
int32_t macroKeyListLength = 0;
int8_t isRecordingMacro = false;
int32_t lastKey = 0;
int8_t *filePath;
int8_t *rcFilePath;

void handleTextLineDeleted(textLine_t *lineToBeDeleted) {
    if (lineToBeDeleted == topTextLine) {
        topTextLine = getNextTextLine(lineToBeDeleted);
        if (topTextLine == NULL) {
            topTextLine = getPreviousTextLine(lineToBeDeleted);
        }
        topTextLineRow = 0;
    }
    int8_t index = 0;
    while (index < MARK_AMOUNT) {
        if (markIsSetList[index]) {
            textLine_t **tempTextLine = markList + index;
            if (lineToBeDeleted == *tempTextLine) {
                *tempTextLine = getNextTextLine(lineToBeDeleted);
                if (*tempTextLine == NULL) {
                    *tempTextLine = getPreviousTextLine(lineToBeDeleted);
                }
            }
        }
        index += 1;
    }
}

void setActivityMode(int8_t mode) {
    eraseActivityModeOrNotification();
    if (activityMode == TEXT_ENTRY_MODE && mode != TEXT_ENTRY_MODE) {
        if (isStartOfNonconsecutiveEscapeSequence) {
            addHistoryFrame();
            historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
            tempFrame->previousCursorTextPos = nonconsecutiveEscapeSequencePreviousCursorTextPos;
            addHistoryActionToHistoryFrame(tempFrame, &firstNonconsecutiveEscapeSequenceAction);
            recordTextLineInserted(cursorTextPos.line);
            finishCurrentHistoryFrame();
            textBufferIsDirty = true;
        }
    }
    if (mode == PREVIOUS_MODE) {
        int8_t tempMode = activityMode;
        activityMode = previousActivityMode;
        previousActivityMode = tempMode;
    } else {
        previousActivityMode = activityMode;
        activityMode = mode;
    }
    if (activityMode == HIGHLIGHT_CHARACTER_MODE) {
        if (mode != PREVIOUS_MODE) {
            highlightTextPos.line = cursorTextPos.line;
            highlightTextPos.column = cursorTextPos.column;
            highlightTextPos.row = cursorTextPos.row;
        }
    }
    if (activityMode == HIGHLIGHT_LINE_MODE) {
        if (mode != PREVIOUS_MODE) {
            highlightTextPos.line = cursorTextPos.line;
            int64_t tempLength = cursorTextPos.line->textAllocation.length;
            setTextPosIndex(&highlightTextPos, tempLength);
            cursorTextPos.row = 0;
            cursorTextPos.column = 0;
            scrollCursorOntoScreen();
        }
    }
    if (activityMode == HIGHLIGHT_WORD_MODE) {
        if (mode != PREVIOUS_MODE) {
            int64_t tempStartIndex = getTextPosIndex(&cursorTextPos);
            int64_t tempEndIndex = tempStartIndex;
            int64_t tempLength = cursorTextPos.line->textAllocation.length;
            if (tempStartIndex < tempLength) {
                int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex];
                if (isWordCharacter(tempCharacter)) {
                    while (tempStartIndex > 0) {
                        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex - 1];
                        if (!isWordCharacter(tempCharacter)) {
                            break;
                        }
                        tempStartIndex -= 1;
                    }
                    while (tempEndIndex < tempLength - 1) {
                        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndIndex + 1];
                        if (!isWordCharacter(tempCharacter)) {
                            break;
                        }
                        tempEndIndex += 1;
                    }
                }
            }
            highlightTextPos.line = cursorTextPos.line;
            setTextPosIndex(&highlightTextPos, tempStartIndex);
            setTextPosIndex(&cursorTextPos, tempEndIndex);
            scrollCursorOntoScreen();
        }
    }
    int8_t tempOldIsHighlighting = isHighlighting;
    isHighlighting = (activityMode == HIGHLIGHT_CHARACTER_MODE || activityMode == HIGHLIGHT_WORD_MODE || activityMode == HIGHLIGHT_LINE_MODE);
    if (activityMode != TEXT_COMMAND_MODE && activityMode != HELP_MODE) {
        if (tempOldIsHighlighting || activityMode == HIGHLIGHT_LINE_MODE) {
            displayAllTextLines();
        } else if (activityMode == HIGHLIGHT_CHARACTER_MODE || activityMode == HIGHLIGHT_WORD_MODE) {
            displayTextLine(getTextLinePosY(cursorTextPos.line), cursorTextPos.line);
        }
    }
    if (activityMode == HELP_MODE || previousActivityMode == HELP_MODE) {
        redrawEverything();
    } else if (activityMode == TEXT_COMMAND_MODE) {
        if (mode != PREVIOUS_MODE) {
            textCommandBuffer[0] = 0;
        }
        displayStatusBar();
    } else if (previousActivityMode == TEXT_COMMAND_MODE) {
        displayStatusBar();
    } else {
        displayActivityMode();
    }
    if (activityMode == HELP_MODE) {
        helpScroll = 0;
    }
    historyFrameIsConsecutive = false;
}

void saveFile() {
    notifyUser((int8_t *)"Saving...");
    refresh();
    int8_t tempNewline = '\n';
    FILE *tempFile = fopen((char *)filePath, "w");
    if (tempFile == NULL) {
        if (errno == EACCES) {
            notifyUser((int8_t *)"ERROR: Permission denied.\n");
            return;
        }
        notifyUser((int8_t *)"ERROR: Could not save.\n");
        return;
    }
    textLine_t *tempLine = getLeftmostTextLine(rootTextLine);
    while (tempLine != NULL) {
        textLine_t *tempNextLine = getNextTextLine(tempLine);
        fwrite(tempLine->textAllocation.text, 1, tempLine->textAllocation.length, tempFile);
        if (tempNextLine != NULL) {
            fwrite(&tempNewline, 1, 1, tempFile);
        }
        tempLine = tempNextLine;
    }
    fclose(tempFile);
    notifyUser((int8_t *)"Saved file.");
    textBufferIsDirty = false;
}

void handleResize() {
    int32_t tempWidth;
    int32_t tempHeight;
    getmaxyx(window, tempHeight, tempWidth);
    if (tempWidth == windowWidth && tempHeight == windowHeight) {
        return;
    }
    windowWidth = tempWidth;
    windowHeight = tempHeight;
    viewPortWidth = windowWidth;
    viewPortHeight = windowHeight - 1;
    topTextLineRow = 0;
    cursorTextPos.line = topTextLine;
    cursorTextPos.row = 0;
    cursorTextPos.column = 0;
    cursorSnapColumn = 0;
    
    if (activityMode != HELP_MODE) {
        setActivityMode(COMMAND_MODE);
    }
    redrawEverything();
}

void playMacro();

// Returns true if the user has quit.
int8_t handleKey(int32_t key) {
    if (isRecordingMacro && key != 'M') {
        if (macroKeyListLength >= MAXIMUM_MACRO_LENGTH) {
            isRecordingMacro = false;
        } else {
            if (key != 'm') {
                macroKeyList[macroKeyListLength] = key;
                macroKeyListLength += 1;
            }
        }
    }
    if (isShowingNotification) {
        eraseNotification();
        displayActivityMode();
    }
    if (key == KEY_RESIZE) {
        handleResize();
    }
    // Escape.
    if (key == 27) {
        if (activityMode == TEXT_COMMAND_MODE) {
            setActivityMode(PREVIOUS_MODE);
        } else {
            setActivityMode(COMMAND_MODE);
        }
    }
    if (activityMode == HELP_MODE) {
        if (key == KEY_UP || key == 'i') {
            scrollHelpMessageUp(1);
        }
        if (key == KEY_DOWN || key == 'k') {
            scrollHelpMessageDown(1);
        }
        if (key == 'I') {
            scrollHelpMessageUp(10);
        }
        if (key == 'K') {
            scrollHelpMessageDown(10);
        }
        if (key == 'q') {
            setActivityMode(COMMAND_MODE);
        }
    } else if (activityMode == TEXT_COMMAND_MODE) {
        if (key >= 32 && key <= 126) {
            insertTextCommandCharacter((int8_t)key);
        }
        // Backspace.
        if (key == 127 || key == 263) {
            deleteTextCommandCharacter();
        }
        if (key == '\n') {
            executeTextCommand();
        }
    } else {
        if (activityMode == TEXT_ENTRY_MODE) {
            if (key == ',' && lastKey == ',') {
                if (isStartOfNonconsecutiveEscapeSequence) {
                    deleteCharacterBeforeCursor(false);
                } else {
                    deleteCharacterBeforeCursor(true);
                }
                isStartOfNonconsecutiveEscapeSequence = false;
                setActivityMode(COMMAND_MODE);
            } else if (key >= 32 && key <= 126) {
                isStartOfNonconsecutiveEscapeSequence = (key == ',' && !historyFrameIsConsecutive);
                insertCharacterBeforeCursor((int8_t)key);
            }
        } else {
            isStartOfNonconsecutiveEscapeSequence = false;
        }
        if (key == '\t') {
            increaseSelectionIndentationLevel();
        }
        if (key == KEY_BTAB) {
            decreaseSelectionIndentationLevel();
        }
        if (activityMode == COMMAND_MODE || activityMode == TEXT_ENTRY_MODE || activityMode == HIGHLIGHT_CHARACTER_MODE) {
            if (key == KEY_LEFT) {
                moveCursorLeft(1);
            }
            if (key == KEY_RIGHT) {
                moveCursorRight(1);
            }
            if (key == KEY_UP) {
                moveCursorUp(1);
            }
            if (key == KEY_DOWN) {
                moveCursorDown(1);
            }
        }
        if (activityMode == COMMAND_MODE || activityMode == HIGHLIGHT_CHARACTER_MODE) {
            if (key == 'j') {
                moveCursorLeft(1);
            }
            if (key == 'l') {
                moveCursorRight(1);
            }
            if (key == 'i') {
                moveCursorUp(1);
            }
            if (key == 'k') {
                moveCursorDown(1);
            }
            if (key == 'J') {
                moveCursorLeft(10);
            }
            if (key == 'L') {
                moveCursorRight(10);
            }
            if (key == 'I') {
                moveCursorUp(10);
            }
            if (key == 'K') {
                moveCursorDown(10);
            }
        }
        if (activityMode == HIGHLIGHT_LINE_MODE) {
            if (key == 'i' || key == KEY_UP) {
                moveLineSelectionUp(1);
            }
            if (key == 'k' || key == KEY_DOWN) {
                moveLineSelectionDown(1);
            }
            if (key == 'I') {
                moveLineSelectionUp(10);
            }
            if (key == 'K') {
                moveLineSelectionDown(10);
            }
        }
        if (activityMode != TEXT_ENTRY_MODE) {
            switch (key) {
                case 'q':
                {
                    if (textBufferIsDirty) {
                        notifyUser((int8_t *)"Unsaved changes. (Shift + Q to quit anyway.)");
                    } else {
                        return true;
                    }
                    break;
                }
                case 'Q':
                {
                    return true;
                    break;
                }
                case 't':
                {
                    setActivityMode(TEXT_ENTRY_MODE);
                    break;
                }
                case 's':
                {
                    saveFile();
                    break;
                }
                case '[':
                {
                    moveCursorToBeginningOfLine();
                    break;
                }
                case ']':
                {
                    moveCursorToEndOfLine();
                    break;
                }
                case '{':
                {
                    moveCursorToBeginningOfFile();
                    break;
                }
                case '}':
                {
                    moveCursorToEndOfFile();
                    break;
                }
                case 'h':
                {
                    setActivityMode(HIGHLIGHT_CHARACTER_MODE);
                    break;
                }
                case 'H':
                {
                    setActivityMode(HIGHLIGHT_LINE_MODE);
                    break;
                }
                case 'w':
                {
                    setActivityMode(HIGHLIGHT_WORD_MODE);
                    break;
                }
                case 'c':
                {
                    copySelection();
                    break;
                }
                case 'x':
                {
                    cutSelection();
                    break;
                }
                case 'X':
                {
                    cutSelection();
                    setActivityMode(TEXT_ENTRY_MODE);
                    break;
                }
                case 'p':
                {
                    pasteAfterCursor();
                    break;
                }
                case 'P':
                {
                    pasteBeforeCursor();
                    break;
                }
                case 'd':
                {
                    deleteSelection();
                    break;
                }
                case 'D':
                {
                    deleteSelection();
                    setActivityMode(TEXT_ENTRY_MODE);
                    break;
                }
                case 'u':
                {
                    undoLastAction();
                    break;
                }
                case 'U':
                {
                    redoLastAction();
                    break;
                }
                case 'm':
                {
                    if (isRecordingMacro) {
                        notifyUser((int8_t *)"Don't be naughty. ;-)");
                    } else {
                        playMacro();
                    }
                    break;
                }
                case 'M':
                {
                    if (isRecordingMacro) {
                        notifyUser((int8_t *)"Finished recording.");
                    } else {
                        macroKeyListLength = 0;
                        notifyUser((int8_t *)"Recording macro.");
                    }
                    isRecordingMacro = !isRecordingMacro;
                    break;
                }
                case '>':
                {
                    increaseSelectionIndentationLevel();
                    break;
                }
                case '<':
                {
                    decreaseSelectionIndentationLevel();
                    break;
                }
                case '/':
                {
                    setActivityMode(TEXT_COMMAND_MODE);
                    break;
                }
                case 'n':
                {
                    gotoNextTerm();
                    break;
                }
                case 'N':
                {
                    gotoPreviousTerm();
                    break;
                }
                case 'f':
                {
                    findNextTermUnderCursor();
                    break;
                }
                case 'F':
                {
                    findPreviousTermUnderCursor();
                    break;
                }
                case 'b':
                {
                    lowercaseSelection();
                    break;
                }
                case 'B':
                {
                    uppercaseSelection();
                    break;
                }
                case '0':
                {
                    gotoMark(0);
                    break;
                }
                case '1':
                {
                    gotoMark(1);
                    break;
                }
                case '2':
                {
                    gotoMark(2);
                    break;
                }
                case '3':
                {
                    gotoMark(3);
                    break;
                }
                case '4':
                {
                    gotoMark(4);
                    break;
                }
                case '5':
                {
                    gotoMark(5);
                    break;
                }
                case '6':
                {
                    gotoMark(6);
                    break;
                }
                case '7':
                {
                    gotoMark(7);
                    break;
                }
                case '8':
                {
                    gotoMark(8);
                    break;
                }
                case '9':
                {
                    gotoMark(9);
                    break;
                }
                case ')':
                {
                    setMark(0);
                    break;
                }
                case '!':
                {
                    setMark(1);
                    break;
                }
                case '@':
                {
                    setMark(2);
                    break;
                }
                case '#':
                {
                    setMark(3);
                    break;
                }
                case '$':
                {
                    setMark(4);
                    break;
                }
                case '%':
                {
                    setMark(5);
                    break;
                }
                case '^':
                {
                    setMark(6);
                    break;
                }
                case '&':
                {
                    setMark(7);
                    break;
                }
                case '*':
                {
                    setMark(8);
                    break;
                }
                case '(':
                {
                    setMark(9);
                    break;
                }
            }
        }
        // Backspace.
        if (key == 127 || key == 263) {
            if (!isHighlighting) {
                deleteCharacterBeforeCursor(true);
            } else {
                deleteSelection();
            }
        }
        if (activityMode != HIGHLIGHT_LINE_MODE && activityMode != HIGHLIGHT_WORD_MODE) {
            if (key == '\n') {
                insertNewlineBeforeCursor();
            }
        }
        if (activityMode == COMMAND_MODE || activityMode == HIGHLIGHT_CHARACTER_MODE) {
            if (key == ' ') {
                insertCharacterBeforeCursor(key);
            }
            if (key == '\'') {
                promptAndInsertCharacterAfterCursor();
            }
            if (key == '"') {
                promptAndInsertCharacterBeforeCursor();
            }
            if (key == '.') {
                promptAndReplaceCharacterUnderCursor();
            }
            if (key == ';') {
                toggleSemicolonAtEndOfLine();
            }            
        }
        if (activityMode == COMMAND_MODE) {
            if (key == '+') {
                incrementNumberUnderCursor();
            }
            if (key == '-') {
                decrementNumberUnderCursor();
            }
        }
    }
    lastKey = key;
    return false;
}

void playMacro() {
    int64_t index = 0;
    while (index < macroKeyListLength) {
        int32_t tempKey = macroKeyList[index];
        handleKey(tempKey);
        index += 1;
    }
}

int8_t setConfigurationVariable(int8_t *name, int64_t value) {
    int8_t output = false;
    if (strcmp((char *)name, "colorScheme") == 0) {
        if (value == 0) {
            primaryColorPair = BLACK_ON_WHITE;
            secondaryColorPair = WHITE_ON_BLACK;
        }
        if (value == 1) {
            primaryColorPair = WHITE_ON_BLACK;
            secondaryColorPair = BLACK_ON_WHITE;
        }
        output = true;
    }
    if (strcmp((char *)name, "indentationWidth") == 0) {
        indentationWidth = value;
        output = true;
    }
    if (strcmp((char *)name, "shouldUseHardTabs") == 0) {
        shouldUseHardTabs = value;
        output = true;
    }
    return output;
}

void processRcFile() {
    FILE *tempFile = fopen((char *)rcFilePath, "r");
    if (tempFile == NULL) {
        return;
    }
    while (true) {
        int8_t *tempText = NULL;
        size_t tempSize = 0;
        int64_t tempCount = getline((char **)&tempText, &tempSize, tempFile);
        if (tempCount < 0) {
            break;
        }
        int8_t *tempText2 = findWhitespace(tempText);
        if (tempText2 != tempText) {
            *tempText2 = 0;
            tempText2 = skipWhitespace(tempText2 + 1);
            int64_t tempValue = atoi((char *)tempText2);
            setConfigurationVariable(tempText, tempValue);
        }
        free(tempText);
    }
}

int main(int argc, const char *argv[]) {
    
    #ifdef __APPLE__
        applicationPlatform = PLATFORM_MAC;
    #else
        applicationPlatform = PLATFORM_LINUX;
    #endif
    
    if (SHOULD_RUN_TESTS) {
        runTests();
        return 0;
    }
    
    if (argc != 2) {
        printf("Usage: breadtext [file path]\n");
        return 0;
    }
    filePath = mallocRealpath((int8_t *)(argv[1]));
    clipboardFilePath = mallocRealpath((int8_t *)"./.temporaryBreadtextClipboard");
    rcFilePath = mallocRealpath((int8_t *)"~/.breadtextrc");
        
    FILE *tempFile = fopen((char *)filePath, "r");
    if (tempFile == NULL) {
        if (errno == EACCES) {
            printf("Permission denied.\n");
            return 0;
        }
        rootTextLine = createEmptyTextLine();
    } else {
        int8_t tempLastContainsNewline = true;
        rootTextLine = NULL;
        while (true) {
            int8_t *tempText = NULL;
            size_t tempSize = 0;
            int64_t tempCount = getline((char **)&tempText, &tempSize, tempFile);
            if (tempCount < 0) {
                if (tempLastContainsNewline) {
                    textLine_t *tempTextLine = createEmptyTextLine();
                    if (rootTextLine == NULL) {
                        rootTextLine = tempTextLine;
                    } else {
                        textLine_t *tempTextLine2 = getRightmostTextLine(rootTextLine);
                        insertTextLineRight(tempTextLine2, tempTextLine);
                    }
                }
                break;
            }
            int8_t tempContainsNewline;
            tempCount = removeBadCharacters(tempText, &tempContainsNewline);
            textLine_t *tempTextLine = createEmptyTextLine();
            insertTextIntoTextAllocation(&(tempTextLine->textAllocation), 0, tempText, tempCount);
            free(tempText);
            if (rootTextLine == NULL) {
                rootTextLine = tempTextLine;
            } else {
                textLine_t *tempTextLine2 = getRightmostTextLine(rootTextLine);
                insertTextLineRight(tempTextLine2, tempTextLine);
            }
            tempLastContainsNewline = tempContainsNewline;
        }
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        }
        fclose(tempFile);
    }
    topTextLine = getLeftmostTextLine(rootTextLine);
    topTextLineRow = 0;
    activityMode = COMMAND_MODE;
    cursorTextPos.line = topTextLine;
    cursorTextPos.row = 0;
    cursorTextPos.column = 0;
    cursorSnapColumn = 0;
    historyFrameListIndex = 0;
    historyFrameListLength = 0;
    historyFrameIsConsecutive = false;
    firstNonconsecutiveEscapeSequenceAction.text = NULL;
    activityModeTextLength = 0;
    lineNumberTextLength = 0;
    isShowingNotification = false;
    notificationTextLength = 0;
    isHighlighting = false;
    primaryColorPair = BLACK_ON_WHITE;
    secondaryColorPair = WHITE_ON_BLACK;
    int8_t index = 0;
    while (index < MARK_AMOUNT) {
        markIsSetList[index] = false;
        index += 1;
    }
    indentationWidth = 4;
    shouldUseHardTabs = false;
    textBufferIsDirty = false;
    isStartOfNonconsecutiveEscapeSequence = false;
    
    processRcFile();    
    
    window = initscr();
    noecho();
    curs_set(0);
    keypad(window, true);
    ESCDELAY = 50;
    start_color();
    init_pair(BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
    handleResize();
    
    while (true) {
        #if IS_IN_DEBUG_MODE
            textLine_t *tempLine1 = getLeftmostTextLine(rootTextLine);
            int64_t tempLength = tempLine1->textAllocation.length;
            int8_t tempBuffer[tempLength];
            copyData(tempBuffer, tempLine1->textAllocation.text, tempLength);
        #endif
        
        int32_t tempKey = getch();
        int8_t tempResult = handleKey(tempKey);
        if (tempResult) {
            break;
        }
        
        #if IS_IN_DEBUG_MODE
            textLine_t *tempLine2 = getLeftmostTextLine(rootTextLine);
            if (tempLength != tempLine2->textAllocation.length
                || !equalData(tempBuffer, tempLine2->textAllocation.text, tempLength)) {
                endwin();
                printf("FIRST LINE CHANGED.\n");
                printf("CHARACTER TYPED: %c\n", tempKey);
                printf("ACTIVITY MODE: %d\n", activityMode);
                exit(0);
            }
        #endif
    }
    
    endwin();
    
    return 0;
}
