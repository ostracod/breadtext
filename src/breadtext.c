
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textLineTest.h"
#include "textPos.h"
#include "history.h"
#include "display.h"
#include "motion.h"
#include "indentation.h"
#include "insertDelete.h"
#include "selection.h"
#include "manipulation.h"
#include "textCommand.h"
#include "fuzz.h"
#include "syntax.h"
#include "systematicTest.h"
#include "script.h"
#include "breadtext.h"

int32_t macroKeyList[MAXIMUM_MACRO_LENGTH];
int32_t macroKeyListLength = 0;
int8_t isRecordingMacro = false;
int32_t macroIndex = 0;
int8_t isPlayingMacro = false;
int32_t lastKey = 0;
int8_t *initialFileContents = NULL;
int64_t initialFileSize;

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
                    if (*tempTextLine == NULL) {
                        markIsSetList[index] = false;
                    }
                }
            }
        }
        index += 1;
    }
}

void addNonconsecutiveEscapeSequenceFrame() {
    addExistingHistoryFrame(&nonconsecutiveEscapeSequenceFrame);
    finishCurrentHistoryFrame();
    nonconsecutiveEscapeSequenceFrameIsSet = false;
}

void setActivityMode(int8_t mode) {
    eraseActivityModeOrNotification();
    if ((activityMode == TEXT_ENTRY_MODE && mode != TEXT_ENTRY_MODE)
            || (activityMode == TEXT_REPLACE_MODE && mode != TEXT_REPLACE_MODE)) {
        if (isStartOfNonconsecutiveEscapeSequence) {
            addNonconsecutiveEscapeSequenceFrame();
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
    int8_t tempOldIsHighlighting = isHighlighting;
    isHighlighting = (activityMode == HIGHLIGHT_CHARACTER_MODE || activityMode == HIGHLIGHT_STATIC_MODE || activityMode == HIGHLIGHT_LINE_MODE);
    if (activityMode != TEXT_COMMAND_MODE && activityMode != HELP_MODE) {
        if (isHighlighting || tempOldIsHighlighting) {
            if (highlightTextPos.line == cursorTextPos.line) {
                displayTextLine(getTextLinePosY(cursorTextPos.line), cursorTextPos.line);
                displayCursor();
            } else {
                displayAllTextLines();
            }
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

void saveFile(int8_t shouldCheckForModifications) {
    if (isPerformingFuzzTest) {
        return;
    }
    struct stat attributes;
    if (shouldCheckForModifications) {
        int32_t tempResult = stat((char *)filePath, &attributes);
        if (tempResult == 0) {
            int64_t tempTime = attributes.st_mtime;
            if (tempTime != fileLastModifiedTime) {
                if (!checkInitialFileContents()) {
                    notifyUser((int8_t *)"ERROR: File modified since last access.");
                    return;
                }
            }
        }
    }
    notifyUser((int8_t *)"Saving...");
    refresh();
    int8_t tempNewline = '\n';
    FILE *tempFile = fopen((char *)filePath, "w");
    if (tempFile == NULL) {
        if (errno == EACCES) {
            notifyUser((int8_t *)"ERROR: Permission denied.");
            return;
        }
        notifyUser((int8_t *)"ERROR: Could not save.");
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
    stat((char *)filePath, &attributes);
    fileLastModifiedTime = attributes.st_mtime;
    storeInitialFileContents();
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

void startRecordingMacro();
void stopRecordingMacro();
void playMacro();
void setShouldUseSystemClipboard(int8_t value);

// Returns true if the user has quit.
int8_t handleKey(int32_t key, int8_t shouldUseMappings, int8_t shouldUseBindings) {
    if (shouldUseBindings) {
        int8_t tempResult = invokeKeyBinding(key);
        if (tempResult) {
            return false;
        }
    }
    if (shouldUseMappings) {
        key = invokeKeyMapping(key);
    }
    if (isRecordingMacro) {
        if (macroKeyListLength >= MAXIMUM_MACRO_LENGTH) {
            stopRecordingMacro();
        } else {
            macroKeyList[macroKeyListLength] = key;
            macroKeyListLength += 1;
        }
    }
    if (isShowingNotification) {
        eraseNotification();
        displayActivityMode();
    }
    if (key == KEY_RESIZE) {
        handleResize();
        return false;
    }
    if (isConsumingSingleCharacter) {
        isConsumingSingleCharacter = false;
        if (key < 32 || key > 126) {
            return false;
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_INSERT_BEFORE) {
            insertCharacterBeforeCursorSimple(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_INSERT_AFTER) {
            insertCharacterAfterCursorSimple(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_REPLACE) {
            replaceCharacterUnderCursorSimple(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_GOTO_INCLUSIVE) {
            goToCharacterInclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_GOTO_EXCLUSIVE) {
            goToCharacterExclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_REVERSE_GOTO_INCLUSIVE) {
            reverseGoToCharacterInclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_REVERSE_GOTO_EXCLUSIVE) {
            reverseGoToCharacterExclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_ENCLOSURE_INCLUSIVE) {
            highlightEnclosureInclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_ENCLOSURE_EXCLUSIVE) {
            highlightEnclosureExclusive(key);
        }
        if (singleCharacterAction == SINGLE_CHARACTER_ACTION_HIGHLIGHT_WORD) {
            highlightWordByDelimiter(key);
        }
        return false;
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
        if (key == KEY_UP || key == 'i' || key == 'a') {
            scrollHelpMessageUp(1);
        }
        if (key == KEY_DOWN || key == 'k' || key == 'z') {
            scrollHelpMessageDown(1);
        }
        if (key == 'I' || key == 'A') {
            scrollHelpMessageUp(10);
        }
        if (key == 'K' || key == 'Z') {
            scrollHelpMessageDown(10);
        }
        if (key == 'q' || key == ',') {
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
        if (activityMode == COMMAND_MODE || activityMode == TEXT_ENTRY_MODE || activityMode == TEXT_REPLACE_MODE || activityMode == HIGHLIGHT_CHARACTER_MODE) {
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
        if (activityMode == HIGHLIGHT_CHARACTER_MODE || activityMode == HIGHLIGHT_STATIC_MODE || activityMode == HIGHLIGHT_LINE_MODE) {
            if (key == ',') {
                setActivityMode(COMMAND_MODE);
            }
        } else {
            if (key == '\n') {
                insertNewlineBeforeCursor(false);
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
        if (key == '\t') {
            increaseSelectionIndentationLevel();
        }
        if (key == KEY_BTAB) {
            decreaseSelectionIndentationLevel();
        }
        // Backspace.
        if (key == 127 || key == 263) {
            if (!isHighlighting) {
                deleteCharacterBeforeCursor(true);
            } else {
                deleteSelection();
            }
        }
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
                insertTextEntryModeCharacterBeforeCursor((int8_t)key);
            } else {
                isStartOfNonconsecutiveEscapeSequence = false;
            }
        } else if (activityMode == TEXT_REPLACE_MODE) {
            if (key == ',' && lastKey == ',') {
                moveCursorLeftConsecutive();
                if (isStartOfNonconsecutiveEscapeSequence) {
                    replaceCharacterAtCursor(lastCharacterDeletedByTextReplaceMode, false);
                } else {
                    replaceCharacterAtCursor(lastCharacterDeletedByTextReplaceMode, true);
                }
                isStartOfNonconsecutiveEscapeSequence = false;
                setActivityMode(COMMAND_MODE);
                moveCursorLeft(1);
            } else if (key >= 32 && key <= 126) {
                insertTextReplaceModeCharacter((int8_t)key);
            } else {
                isStartOfNonconsecutiveEscapeSequence = false;
            }
        } else {
            isStartOfNonconsecutiveEscapeSequence = false;
        }
        if (activityMode != TEXT_ENTRY_MODE && activityMode != TEXT_REPLACE_MODE) {
            switch (key) {
                case 'q':
                {
                    int8_t tempShouldQuit = true;
                    if (textBufferIsDirty) {
                        if (!checkTextBufferHygiene()) {
                            notifyUser((int8_t *)"Unsaved changes. (Shift + Q to quit anyway.)");
                            tempShouldQuit = false;
                        }
                    }
                    if (tempShouldQuit) {
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
                case 'T':
                {
                    setActivityMode(TEXT_REPLACE_MODE);
                    break;
                }
                case 's':
                {
                    saveFile(true);
                    break;
                }
                case 'S':
                {
                    saveFile(false);
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
                case '=':
                {
                    moveCursorToEndOfIndentation();
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
                    highlightWord();
                    break;
                }
                case 'W':
                {
                    promptAndHighlightWordByDelimiter();
                    break;
                }
                case 'e':
                {
                    promptCharacterAndHighlightEnclosureExclusive();
                    break;
                }
                case 'E':
                {
                    promptCharacterAndHighlightEnclosureInclusive();
                    break;
                }
                case 'c':
                {
                    copySelection();
                    break;
                }
                case 'C':
                {
                    changeLine();
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
                        // Test code.
                        /*
                        endwin();
                        int32_t index = 0;
                        while (index < macroKeyListLength) {
                            int32_t tempKey = macroKeyList[index];
                            printf("%c\n", (char)tempKey);
                            index += 1;
                        }
                        exit(0);
                        */
                        stopRecordingMacro();
                    } else {
                        startRecordingMacro();
                    }
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
                case '?':
                {
                    enterBeginningOfCommand((int8_t *)"find ");
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
                case ':':
                {
                    toggleSelectionComment();
                    break;
                }
                case 'a':
                {
                    moveTextUp(1);
                    break;
                }
                case 'z':
                {
                    moveTextDown(1);
                    break;
                }
                case 'A':
                {
                    moveTextUp(10);
                    break;
                }
                case 'Z':
                {
                    moveTextDown(10);
                    break;
                }
                case 'g':
                {
                    promptAndGoToCharacterExclusive();
                    break;
                }
                case 'G':
                {
                    promptAndGoToCharacterInclusive();
                    break;
                }
                case 'r':
                {
                    promptAndReverseGoToCharacterExclusive();
                    break;
                }
                case 'R':
                {
                    promptAndReverseGoToCharacterInclusive();
                    break;
                }
                case '`':
                {
                    scrollCursorOntoScreen();
                    break;
                }
                case '~':
                {
                    moveCursorToVisibleText();
                    break;
                }
                case 'y':
                {
                    setShouldUseSystemClipboard(false);
                    break;
                }
                case 'Y':
                {
                    setShouldUseSystemClipboard(true);
                    break;
                }
                case 'v':
                {
                    highlightLineContents();
                    break;
                }
                case 'V':
                {
                    enterBeginningOfCommand((int8_t *)"gotoLine ");
                    break;
                }
                case '8':
                {
                    goToMatchingCharacter();
                    break;
                }
                case '*':
                {
                    swapSelection();
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
            }
        }
        if (activityMode == COMMAND_MODE) {
            if (key == ' ') {
                insertCharacterBeforeCursor(key, true);
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
            if (key == '+') {
                incrementNumberUnderCursor();
            }
            if (key == '-') {
                decrementNumberUnderCursor();
            }
            if (key == '_') {
                toggleBooleanLiteral();
            }
            if (key == ';') {
                toggleSemicolonAtEndOfLine();
            }
            if (key == '\\') {
                insertLineAfterCursor();
            }
            if (key == '|') {
                insertLineBeforeCursor();
            }
            if (key == 'o') {
                insertAndEditLineAfterCursor();
            }
            if (key == 'O') {
                insertAndEditLineBeforeCursor();
            }
            if (key == '9') {
                selectUntilBeginningOfLineExclusive();
            }
            if (key == '(') {
                selectUntilBeginningOfLineInclusive();
            }
            if (key == '0') {
                selectUntilEndOfLineExclusive();
            }
            if (key == ')') {
                selectUntilEndOfLineInclusive();
            }
        }
    }
    lastKey = key;
    return false;
}

void startRecordingMacro() {
    macroKeyListLength = 0;
    isRecordingMacro = true;
    notifyUser((int8_t *)"Recording macro.");
}

void stopRecordingMacro() {
    if (macroKeyList[macroKeyListLength - 1] == 'M') {
        macroKeyListLength -= 1;
    }
    isRecordingMacro = false;
    notifyUser((int8_t *)"Finished recording.");
}

void playMacro() {
    if (isPlayingMacro) {
        return;
    }
    isPlayingMacro = true;
    macroIndex = 0;
    while (macroIndex < macroKeyListLength) {
        int32_t tempKey = macroKeyList[macroIndex];
        handleKey(tempKey, false, false);
        macroIndex += 1;
    }
    isPlayingMacro = false;
}

void promptSingleCharacter() {
    notifyUser((int8_t *)"Type a character.");
    isConsumingSingleCharacter = true;
    singleCharacterAction = SINGLE_CHARACTER_ACTION_NONE;
}

int8_t getConfigurationVariable(int64_t *destination, int8_t *name) {
    int8_t output = false;
    if (strcmp((char *)name, "colorScheme") == 0) {
        *destination = colorScheme;
        output = true;
    }
    if (strcmp((char *)name, "indentationWidth") == 0) {
        *destination = indentationWidth;
        output = true;
    }
    if (strcmp((char *)name, "shouldUseHardTabs") == 0) {
        *destination = shouldUseHardTabs;
        output = true;
    }
    if (strcmp((char *)name, "isCaseSensitive") == 0) {
        *destination = isCaseSensitive;
        output = true;
    }
    if (strcmp((char *)name, "shouldUseSystemClipboard") == 0) {
        *destination = shouldUseSystemClipboard;
        output = true;
    }
    if (strcmp((char *)name, "shouldHighlightSyntax") == 0) {
        *destination = shouldHighlightSyntax;
        output = true;
    }
    if (strcmp((char *)name, "shouldUseXclip") == 0) {
        *destination = shouldUseXclip;
        output = true;
    }
    return output;
}

int8_t setConfigurationVariable(int8_t *name, int64_t value) {
    int8_t output = false;
    if (strcmp((char *)name, "colorScheme") == 0) {
        setColorScheme(value);
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
    if (strcmp((char *)name, "isCaseSensitive") == 0) {
        isCaseSensitive = value;
        compileRegexes();
        output = true;
    }
    if (strcmp((char *)name, "shouldUseSystemClipboard") == 0) {
        shouldUseSystemClipboard = value;
        output = true;
    }
    if (strcmp((char *)name, "shouldHighlightSyntax") == 0) {
        shouldHighlightSyntax = value;
        updateSyntaxDefinition();
        output = true;
    }
    if (strcmp((char *)name, "shouldUseXclip") == 0) {
        shouldUseXclip = value;
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
    fclose(tempFile);
}

void storeInitialFileContents() {
    if (initialFileContents != NULL) {
        free(initialFileContents);
    }
    FILE *tempFile = fopen((char *)filePath, "r");
    if (tempFile == NULL) {
        initialFileContents = NULL;
        return;
    }
    fseek(tempFile, 0L, SEEK_END);
    initialFileSize = ftell(tempFile);
    fseek(tempFile, 0L, SEEK_SET);
    if (initialFileSize == 0) {
        initialFileContents = NULL;
        fclose(tempFile);
        return;
    }
    initialFileContents = malloc(initialFileSize);
    int64_t index = 0;
    while (index < initialFileSize) {
        int64_t tempAmount = 100;
        if (index + tempAmount > initialFileSize) {
            tempAmount = initialFileSize - index;
        }
        fread(initialFileContents + index, 1, tempAmount, tempFile);
        index += tempAmount;
    }
    fclose(tempFile);
}

void clearInitialFileContents() {
    if (initialFileContents != NULL) {
        free(initialFileContents);
    }
    initialFileContents = NULL;
}

int8_t checkInitialFileContents() {
    FILE *tempFile = fopen((char *)filePath, "r");
    if (tempFile == NULL) {
        return initialFileContents == NULL;
    }
    fseek(tempFile, 0L, SEEK_END);
    int64_t tempSize = ftell(tempFile);
    fseek(tempFile, 0L, SEEK_SET);
    if (tempSize == 0) {
        fclose(tempFile);
        return (initialFileContents == NULL);
    } else if (initialFileContents == NULL) {
        fclose(tempFile);
        return false;
    }
    if (tempSize != initialFileSize) {
        fclose(tempFile);
        return false;
    }
    int64_t index = 0;
    while (index < tempSize) {
        int64_t tempAmount = 100;
        if (index + tempAmount > tempSize) {
            tempAmount = tempSize - index;
        }
        int8_t tempBuffer[tempAmount];
        fread(tempBuffer, 1, tempAmount, tempFile);
        if (!equalData(tempBuffer, initialFileContents + index, tempAmount)) {
            fclose(tempFile);
            return false;
        }
        index += tempAmount;
    }
    fclose(tempFile);
    return true;
}

int8_t checkTextBufferHygiene() {
    if (initialFileContents == NULL) {
        textLine_t *tempTextLine = getLeftmostTextLine(rootTextLine);
        textLine_t *tempTextLine2 = getNextTextLine(tempTextLine);
        return (tempTextLine->textAllocation.length == 0 && tempTextLine2 == NULL);
    }
    int64_t index = 0;
    textLine_t *tempTextLine = getLeftmostTextLine(rootTextLine);
    while (tempTextLine != NULL) {
        int64_t tempLength = tempTextLine->textAllocation.length;
        if (index + tempLength > initialFileSize) {
            return false;
        }
        if (!equalData(initialFileContents + index, tempTextLine->textAllocation.text, tempLength)) {
            return false;
        }
        index += tempLength;
        tempTextLine = getNextTextLine(tempTextLine);
        if (tempTextLine != NULL) {
            if (index >= initialFileSize) {
                return false;
            }
            int8_t tempCharacter = initialFileContents[index];
            if (tempCharacter == '\n') {
                index += 1;
            } else if (tempCharacter == '\r') {
                index += 1;
                if (index < initialFileSize) {
                    int8_t tempCharacter = initialFileContents[index];
                    if (tempCharacter == '\n') {
                        index += 1;
                    }
                }
            } else {
                return false;
            }
        }
    }
    return true;
}

void setShouldUseSystemClipboard(int8_t value) {
    if (isPerformingFuzzTest) {
        shouldUseSystemClipboard = false;
    } else {
        shouldUseSystemClipboard = value;
    }
    if (shouldUseSystemClipboard) {
        notifyUser((int8_t *)"Using system clipboard.");
    } else {
        notifyUser((int8_t *)"Using internal clipboard.");
    }
}

void cleanUpApplication() {
    while (rootTextLine != NULL) {
        deleteTextLine(rootTextLine);
    }
    int32_t index = 0;
    while (index < historyFrameListLength) {
        cleanUpHistoryFrame(historyFrameList + index);
        index += 1;
    }
    index = 0;
    while (index < keywordListLength) {
        free(keywordList + index);
        index += 1;
    }
}

int8_t initializeApplication() {
    FILE *tempFile;
    if (isPerformingFuzzTest) {
        tempFile = NULL;
    } else {
        tempFile = fopen((char *)filePath, "r");
    }
    if (tempFile == NULL) {
        if (errno == EACCES) {
            printf("Permission denied.\n");
            return false;
        }
        fileLastModifiedTime = TIME_NEVER;
        rootTextLine = createEmptyTextLine();
        clearInitialFileContents();
    } else {
        struct stat attributes;
        stat((char *)filePath, &attributes);
        fileLastModifiedTime = attributes.st_mtime;
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
        storeInitialFileContents();
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
    activityModeTextLength = 0;
    lineNumberTextLength = 0;
    isShowingNotification = false;
    notificationTextLength = 0;
    isHighlighting = false;
    searchTerm[0] = 0;
    searchTermLength = 0;
    searchRegexIsEmpty = true;
    searchTermIsRegex = false;
    setColorScheme(0);
    int8_t index = 0;
    while (index < MARK_AMOUNT) {
        markIsSetList[index] = false;
        index += 1;
    }
    indentationWidth = 4;
    shouldUseHardTabs = false;
    isCaseSensitive = true;
    textBufferIsDirty = false;
    isStartOfNonconsecutiveEscapeSequence = false;
    lastIsStartOfNonconsecutiveEscapeSequence = false;
    shouldUseSystemClipboard = true;
    shouldHighlightSyntax = true;
    commentPrefix = NULL;
    keywordList = NULL;
    shouldUseXclip = (applicationPlatform == PLATFORM_LINUX);
    nonconsecutiveEscapeSequenceFrameIsSet = false;
    isConsumingSingleCharacter = false;
    
    int8_t tempIsTesting = (isPerformingFuzzTest || isPerformingSystematicTest);
    if (!tempIsTesting) {
        processRcFile();
    }
    
    init_pair(BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(RED_ON_WHITE, COLOR_RED, COLOR_WHITE);
    init_pair(GREEN_ON_WHITE, COLOR_GREEN, COLOR_WHITE);
    init_pair(CYAN_ON_WHITE, COLOR_CYAN, COLOR_WHITE);
    init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(RED_ON_BLACK, COLOR_RED, COLOR_BLACK);
    init_pair(GREEN_ON_BLACK, COLOR_GREEN, COLOR_BLACK);
    init_pair(CYAN_ON_BLACK, COLOR_CYAN, COLOR_BLACK);
    
    updateSyntaxDefinition();
    
    handleResize();
    
    initializeScriptingEnvironment();
    if (!tempIsTesting) {
        if (access((char *)rcScriptFilePath, F_OK) != -1) {
            runScript(rcScriptFilePath);
        }
    }
    
    return true;
}

void resetApplication() {
    cleanUpApplication();
    initializeApplication();
}

int main(int argc, const char *argv[]) {
    
    strcpy((char *)applicationVersion, "1.1.0");
    
    struct timeval timeValue;
    gettimeofday(&timeValue, NULL);
    srand((unsigned)(timeValue.tv_sec * 1000 + timeValue.tv_usec / 1000));
    
    #ifdef __APPLE__
        applicationPlatform = PLATFORM_MAC;
    #else
        applicationPlatform = PLATFORM_LINUX;
    #endif
    
    if (SHOULD_RUN_TEXT_LINE_TEST) {
        runTextLineTest();
        return 0;
    }
    
    int8_t tempArgumentsAreValid = false;
    isPerformingFuzzTest = false;
    isPerformingSystematicTest = false;
    if (argc == 3) {
        if (strcmp(argv[1], "--test") == 0) {
            isPerformingSystematicTest = true;
            systematicTestDefinitionPath = mallocRealpath((int8_t *)argv[2]);
            systematicTestResultPath = mallocRealpath((int8_t *)"./systematicTestResult.txt");
            tempArgumentsAreValid = true;
        }
    }
    if (argc == 2) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("%s\n", applicationVersion);
            return 0;
        }
        isPerformingFuzzTest = (strcmp(argv[1], "--fuzz") == 0);
        tempArgumentsAreValid = true;
    }
    if (!tempArgumentsAreValid) {
        printf("Usage:\nbreadtext [file path]\nbreadtext --version\n");
        return 0;
    }
    if (isPerformingFuzzTest || isPerformingSystematicTest) {
        filePath = mallocRealpath((int8_t *)"./bupkis.txt");
    } else {
        filePath = mallocRealpath((int8_t *)(argv[1]));
    }
    clipboardFilePath = mallocRealpath((int8_t *)"./.temporaryBreadtextClipboard");
    rcFilePath = mallocRealpath((int8_t *)"~/.breadtextrc");
    rcScriptFilePath = mallocRealpath((int8_t *)"~/.breadtextrc.btsl");
    syntaxDirectoryPath = mallocRealpath((int8_t *)"~/.breadtextsyntax");
    
    window = initscr();
    noecho();
    curs_set(0);
    keypad(window, true);
    ESCDELAY = 50;
    start_color();
    int8_t tempResult = initializeApplication();
    if (!tempResult) {
        endwin();
        return 0;
    }
    
    if (isPerformingFuzzTest) {
        runFuzzTest();
        endwin();
        return 0;
    }
    
    if (isPerformingSystematicTest) {
        int8_t tempResult = runSystematicTest();
        endwin();
        if (tempResult) {
            return 0;
        } else {
            return 1;
        }
    }
    
    while (true) {
        #if IS_IN_DEBUG_MODE
            textLine_t *tempLine1 = getLeftmostTextLine(rootTextLine);
            int64_t tempLength = tempLine1->textAllocation.length;
            int8_t tempBuffer[tempLength];
            copyData(tempBuffer, tempLine1->textAllocation.text, tempLength);
        #endif
        
        int32_t tempKey = getch();
        int8_t tempResult = handleKey(tempKey, true, true);
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
