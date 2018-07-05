
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "vector.h"
#include "display.h"
#include "history.h"
#include "indentation.h"
#include "motion.h"
#include "insertDelete.h"
#include "selection.h"
#include "manipulation.h"
#include "secret.h"
#include "syntax.h"
#include "textCommand.h"
#include "scriptValue.h"
#include "script.h"
#include "breadtext.h"

int8_t compileRegexesHelper() {
    if (!searchTermIsRegex) {
        return false;
    }
    if (!searchRegexIsEmpty) {
        regfree(&searchRegexForward);
        regfree(&searchRegexBackward);
        searchRegexIsEmpty = true;
    }
    int32_t tempForwardTermLength = searchTermLength + 2;
    int8_t tempForwardTerm[tempForwardTermLength + 1];
    int32_t tempBackwardTermLength = searchTermLength + 5;
    int8_t tempBackwardTerm[tempBackwardTermLength + 1];
    tempForwardTerm[0] = '(';
    copyData(tempForwardTerm + 1, searchTerm, searchTermLength);
    tempForwardTerm[tempForwardTermLength - 1] = ')';
    tempForwardTerm[tempForwardTermLength] = 0;
    tempBackwardTerm[0] = '^';
    tempBackwardTerm[1] = '.';
    tempBackwardTerm[2] = '*';
    tempBackwardTerm[3] = '(';
    copyData(tempBackwardTerm + 4, searchTerm, searchTermLength);
    tempBackwardTerm[tempBackwardTermLength - 1] = ')';
    tempBackwardTerm[tempBackwardTermLength] = 0;
    int32_t tempFlags = REG_EXTENDED;
    if (!isCaseSensitive) {
        tempFlags |= REG_ICASE;
    }
    int32_t tempResult;
    tempResult = regcomp(&searchRegexForward, (char *)tempForwardTerm, tempFlags);
    if (tempResult != 0) {
        return false;
    }
    regcomp(&searchRegexBackward, (char *)tempBackwardTerm, tempFlags);
    if (tempResult != 0) {
        regfree(&searchRegexForward);
        return false;
    }
    searchRegexIsEmpty = false;
    return true;
}

int8_t compileRegexes() {
    int8_t tempResult = compileRegexesHelper();
    if (!tempResult) {
        searchTerm[0] = 0;
        searchTermLength = 0;
        searchTermIsRegex = false;
    }
    return tempResult;
}

int64_t getMaximumCommandLength() {
    int64_t tempLength1 = sizeof(textCommandBuffer) - 1;
    int64_t tempLength2 = windowWidth - 2;
    if (tempLength1 < tempLength2) {
        return tempLength1;
    } else {
        return tempLength2;
    }
}

void insertTextCommandCharacter(int8_t character) {
    int32_t tempLength = strlen((char *)textCommandBuffer);
    if (tempLength >= getMaximumCommandLength()) {
        return;
    }
    eraseTextCommandCursor();
    copyData(textCommandBuffer + textCommandCursorIndex + 1, textCommandBuffer + textCommandCursorIndex, tempLength - textCommandCursorIndex + 1);
    textCommandBuffer[textCommandCursorIndex] = character;
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 1 + textCommandCursorIndex, "%s", textCommandBuffer + textCommandCursorIndex);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    textCommandCursorIndex += 1;
    displayTextCommandCursor();
}

void deleteTextCommandCharacter() {
    if (textCommandCursorIndex <= 0) {
        return;
    }
    int32_t tempLength = strlen((char *)textCommandBuffer);
    eraseTextCommandCursor();
    copyData(textCommandBuffer + textCommandCursorIndex - 1, textCommandBuffer + textCommandCursorIndex, tempLength - textCommandCursorIndex + 1);
    textCommandCursorIndex -= 1;
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 1 + textCommandCursorIndex, "%s ", textCommandBuffer + textCommandCursorIndex);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    displayTextCommandCursor();
}

void executeTextCommandByTermList(scriptValue_t *destination, int8_t **termList, int32_t termListLength) {
    if (destination != NULL) {
        destination->type = SCRIPT_VALUE_TYPE_NULL;
    }
    if (activityMode != TEXT_COMMAND_MODE) {
        setActivityMode(TEXT_COMMAND_MODE);
    }
    if (termListLength <= 0) {
        setActivityMode(PREVIOUS_MODE);
        notifyUser((int8_t *)"Error: Invalid command.");
        return;
    }
    int8_t tempResult = invokeCommandBinding(destination, termList, termListLength);
    if (tempResult) {
        return;
    }
    if (strcmp((char *)(termList[0]), "gotoLine") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempLineNumber = atoi((char *)(termList[1]));
        textPos_t tempTextPos;
        tempTextPos.line = getTextLineByNumber(tempLineNumber);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        if (tempTextPos.line == NULL) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Bad line number.");
            return;
        }
        setActivityMode(PREVIOUS_MODE);
        moveCursor(&tempTextPos);
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        return;
    }
    if (strcmp((char *)(termList[0]), "find") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(PREVIOUS_MODE);
        int8_t tempResult = gotoNextTermHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find term.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "reverseFind") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(PREVIOUS_MODE);
        int8_t tempResult = gotoPreviousTermHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find term.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "findWord") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(PREVIOUS_MODE);
        int8_t tempResult = gotoNextWordHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find word.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "reverseFindWord") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(PREVIOUS_MODE);
        int8_t tempResult = gotoPreviousWordHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find word.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "regex") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = true;
        int8_t tempResult = compileRegexes();
        setActivityMode(PREVIOUS_MODE);
        if (!tempResult) {
            notifyUser((int8_t *)"Invalid regex.");
            return;
        }
        tempResult = gotoNextTermHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find term.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "reverseRegex") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = true;
        int8_t tempResult = compileRegexes();
        setActivityMode(PREVIOUS_MODE);
        if (!tempResult) {
            notifyUser((int8_t *)"Invalid regex.");
            return;
        }
        tempResult = gotoPreviousTermHelper();
        cursorSnapColumn = cursorTextPos.column;
        historyFrameIsConsecutive = false;
        if (!tempResult) {
            notifyUser((int8_t *)"Could not find term.");
            return;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "replace") == 0) {
        if (termListLength != 3) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(termList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(COMMAND_MODE);
        cursorTextPos.row = 0;
        cursorTextPos.column = 0;
        cursorSnapColumn = 0;
        historyFrameIsConsecutive = false;
        scrollCursorOntoScreen();
        int64_t tempResult = findAndReplaceAllTerms(termList[2]);
        int8_t tempText[1000];
        if (tempResult == 1) {
            sprintf((char *)tempText, "Replaced %lld term.", (long long)(tempResult));
        } else {
            sprintf((char *)tempText, "Replaced %lld terms.", (long long)(tempResult));
        }
        notifyUser(tempText);
        return;
    }
    if (strcmp((char *)(termList[0]), "get") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempValue;
        int8_t tempResult = getConfigurationVariable(&tempValue, termList[1]);
        if (!tempResult) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Could not get variable.");
            return;
        }
        setActivityMode(PREVIOUS_MODE);
        int8_t tempText[1000];
        sprintf((char *)tempText, "%s: %lld", (char *)termList[1], (long long)(tempValue));
        notifyUser(tempText);
        if (destination != NULL) {
            destination->type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(destination->data) = (double)tempValue;
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "set") == 0) {
        if (termListLength != 3) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempValue = atoi((char *)(termList[2]));
        int8_t tempResult = setConfigurationVariable(termList[1], tempValue);
        if (!tempResult) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Could not set variable.");
            return;
        }
        setActivityMode(PREVIOUS_MODE);
        redrawEverything();
        notifyUser((int8_t *)"Set configuration variable.");
        return;
    }
    if (strcmp((char *)(termList[0]), "help") == 0) {
        setActivityMode(HELP_MODE);
        return;
    }
    if (strcmp((char *)(termList[0]), "getPath") == 0) {
        if (termListLength != 1) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        setActivityMode(PREVIOUS_MODE);
        notifyUser(filePath);
        if (destination != NULL) {
            *destination = convertTextToStringValue(filePath);
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "setPath") == 0) {
        if (termListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        filePath = mallocRealpath(termList[1]);
        fileLastModifiedTime = TIME_NEVER;
        clearInitialFileContents();
        updateSyntaxDefinition();
        setActivityMode(PREVIOUS_MODE);
        notifyUser((int8_t *)"Changed file path.");
        return;
    }
    if (strcmp((char *)(termList[0]), "version") == 0) {
        setActivityMode(PREVIOUS_MODE);
        notifyUser(applicationVersion);
        if (destination != NULL) {
            *destination = convertTextToStringValue(applicationVersion);
        }
        return;
    }
    if (strcmp((char *)(termList[0]), "crane") == 0) {
        craneSecret();
        setActivityMode(PREVIOUS_MODE);
        return;
    }
    if (strcmp((char *)(termList[0]), "jitter") == 0) {
        jitterSecret();
        setActivityMode(PREVIOUS_MODE);
        return;
    }
    setActivityMode(PREVIOUS_MODE);
    notifyUser((int8_t *)"Error: Unrecognized command name.");
}

void executeTextCommand() {
    int8_t *tempTermList[20];
    int32_t tempTermListLength;
    parseSpaceSeperatedTerms(tempTermList, &tempTermListLength, textCommandBuffer);
    executeTextCommandByTermList(NULL, tempTermList, tempTermListLength);
}

void enterBeginningOfCommand(int8_t *text) {
    setActivityMode(TEXT_COMMAND_MODE);
    strcpy((char *)textCommandBuffer, (char *)text);
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 1, "%s", (char *)textCommandBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    textCommandCursorIndex = strlen((char *)textCommandBuffer);
    displayTextCommandCursor();
}

void moveTextCommandCursorLeft() {
    if (textCommandCursorIndex <= 0) {
        return;
    }
    eraseTextCommandCursor();
    textCommandCursorIndex -= 1;
    displayTextCommandCursor();
}

void moveTextCommandCursorRight() {
    if (textCommandCursorIndex >= strlen((char *)textCommandBuffer)) {
        return;
    }
    eraseTextCommandCursor();
    textCommandCursorIndex += 1;
    displayTextCommandCursor();
}

void pasteClipboardIntoTextCommand() {
    vector_t tempSystemClipboard;
    while (true) {
        int8_t *tempText;
        if (shouldUseSystemClipboard) {
            systemPasteClipboard(&tempSystemClipboard);
            if (tempSystemClipboard.length <= 0) {
                cleanUpSystemClipboardAllocation(&tempSystemClipboard);
                break;
            }
            getVectorElement(&tempText, &tempSystemClipboard, 0);
            int8_t tempContainsNewline;
            removeBadCharacters(tempText, &tempContainsNewline);
        } else {
            if (internalClipboard == NULL) {
                break;
            }
            tempText = internalClipboard;
        }
        int64_t tempLength = strlen((char *)textCommandBuffer);
        int64_t tempMaximumLength = getMaximumCommandLength();
        int64_t tempClipboardLength = 0;
        while (tempLength < tempMaximumLength) {
            int8_t tempCharacter = tempText[tempClipboardLength];
            if (tempCharacter == 0 || tempCharacter == '\n') {
                break;
            }
            tempLength += 1;
            tempClipboardLength += 1;
        }
        tempLength = strlen((char *)textCommandBuffer);
        eraseTextCommandCursor();
        copyData(textCommandBuffer + textCommandCursorIndex + tempClipboardLength, textCommandBuffer + textCommandCursorIndex, tempLength - textCommandCursorIndex + 1);
        copyData(textCommandBuffer + textCommandCursorIndex, tempText, tempClipboardLength);
        attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
        mvprintw(windowHeight - 1, 1 + textCommandCursorIndex, "%s", textCommandBuffer + textCommandCursorIndex);
        attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
        textCommandCursorIndex += tempClipboardLength;
        displayTextCommandCursor();
        break;
    }
    if (shouldUseSystemClipboard) {
        cleanUpSystemClipboardAllocation(&tempSystemClipboard);
    }
}


