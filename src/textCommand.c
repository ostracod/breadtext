
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
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

void insertTextCommandCharacter(int8_t character) {
    int8_t index = strlen((char *)textCommandBuffer);
    if (index >= sizeof(textCommandBuffer) - 1) {
        return;
    }
    eraseTextCommandCursor();
    textCommandBuffer[index] = character;
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvaddch(windowHeight - 1, index + 1, character);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    index += 1;
    textCommandBuffer[index] = 0;
    displayTextCommandCursor();
}

void deleteTextCommandCharacter() {
    int8_t index = strlen((char *)textCommandBuffer);
    if (index <= 0) {
        return;
    }
    eraseTextCommandCursor();
    index -= 1;
    textCommandBuffer[index] = 0;
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvaddch(windowHeight - 1, index + 1, ' ');
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    displayTextCommandCursor();
}

void executeTextCommand() {
    int8_t *tempTermList[20];
    int32_t tempTermListLength;
    parseSpaceSeperatedTerms(tempTermList, &tempTermListLength, textCommandBuffer);
    /*
    endwin();
    int64_t index = 0;
    while (index < tempTermListLength) {
        printf("%s\n", (char *)(tempTermList[index]));
        index += 1;
    }
    exit(0);
    */
    if (tempTermListLength <= 0) {
        setActivityMode(PREVIOUS_MODE);
        notifyUser((int8_t *)"Error: Invalid command.");
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "gotoLine") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempLineNumber = atoi((char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "find") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "reverseFind") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "findWord") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "reverseFindWord") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "regex") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "reverseRegex") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
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
    if (strcmp((char *)(tempTermList[0]), "replace") == 0) {
        if (tempTermListLength != 3) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
        searchTermLength = strlen((char *)searchTerm);
        searchTermIsRegex = false;
        setActivityMode(COMMAND_MODE);
        cursorTextPos.row = 0;
        cursorTextPos.column = 0;
        cursorSnapColumn = 0;
        historyFrameIsConsecutive = false;
        scrollCursorOntoScreen();
        int64_t tempResult = findAndReplaceAllTerms(tempTermList[2]);
        int8_t tempText[1000];
        if (tempResult == 1) {
            sprintf((char *)tempText, "Replaced %lld term.", (long long)(tempResult));
        } else {
            sprintf((char *)tempText, "Replaced %lld terms.", (long long)(tempResult));
        }
        notifyUser(tempText);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "set") == 0) {
        if (tempTermListLength != 3) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempValue = atoi((char *)(tempTermList[2]));
        int8_t tempResult = setConfigurationVariable(tempTermList[1], tempValue);
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
    if (strcmp((char *)(tempTermList[0]), "help") == 0) {
        setActivityMode(HELP_MODE);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "getPath") == 0) {
        if (tempTermListLength != 1) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        setActivityMode(PREVIOUS_MODE);
        notifyUser(filePath);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "setPath") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        filePath = mallocRealpath(tempTermList[1]);
        fileLastModifiedTime = TIME_NEVER;
        clearInitialFileContents();
        updateSyntaxDefinition();
        setActivityMode(PREVIOUS_MODE);
        notifyUser((int8_t *)"Changed file path.");
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "version") == 0) {
        setActivityMode(PREVIOUS_MODE);
        notifyUser(applicationVersion);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "crane") == 0) {
        craneSecret();
        setActivityMode(PREVIOUS_MODE);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "jitter") == 0) {
        jitterSecret();
        setActivityMode(PREVIOUS_MODE);
        return;
    }
    setActivityMode(PREVIOUS_MODE);
    notifyUser((int8_t *)"Error: Unrecognized command name.");
}

void enterBeginningOfCommand(int8_t *text) {
    setActivityMode(TEXT_COMMAND_MODE);
    strcpy((char *)textCommandBuffer, (char *)text);
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 1, "%s", (char *)textCommandBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    displayTextCommandCursor();
}
