
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
#include "cursorMotion.h"
#include "insertDelete.h"
#include "selection.h"
#include "manipulation.h"
#include "textCommand.h"
#include "breadtext.h"

void insertTextCommandCharacter(int8_t character) {
    int8_t index = strlen((char *)textCommandBuffer);
    if (index >= sizeof(textCommandBuffer) - 1) {
        return;
    }
    eraseTextCommandCursor();
    textCommandBuffer[index] = character;
    attron(COLOR_PAIR(secondaryColorPair));
    mvaddch(windowHeight - 1, index + 1, character);
    attroff(COLOR_PAIR(secondaryColorPair));
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
    attron(COLOR_PAIR(secondaryColorPair));
    mvaddch(windowHeight - 1, index + 1, ' ');
    attroff(COLOR_PAIR(secondaryColorPair));
    displayTextCommandCursor();
}

void executeTextCommand() {
    int8_t *tempTermList[20];
    int8_t tempTermIndex = 0;
    int8_t tempIsInQuotes = false;
    int8_t *tempText1 = textCommandBuffer;
    int8_t *tempText2 = textCommandBuffer;
    int8_t tempIsStartOfTerm = true;
    while (true) {
        int8_t tempCharacter = *tempText1;
        tempText1 += 1;
        if (tempCharacter == 0) {
            *tempText2 = 0;
            tempText2 += 1;
            break;
        }
        int8_t tempIsWhitespace = isWhitespace(tempCharacter);
        if (tempCharacter == '\\') {
            int8_t tempCharacter = *tempText1;
            tempText1 += 1;
            *tempText2 = tempCharacter;
            tempText2 += 1;
        } else if (tempIsStartOfTerm && !tempIsWhitespace) {
            if (tempCharacter == '"') {
                tempTermList[tempTermIndex] = tempText2;
                tempTermIndex += 1;
                tempIsInQuotes = true;
            } else {
                tempTermList[tempTermIndex] = tempText2;
                *tempText2 = tempCharacter;
                tempText2 += 1;
                tempTermIndex += 1;
            }
            tempIsStartOfTerm = false;
        } else {
            if (tempCharacter == '"') {
                *tempText2 = 0;
                tempText2 += 1;
                tempIsInQuotes = false;
                tempIsStartOfTerm = true;
            } else if (!tempIsWhitespace || tempIsInQuotes) {
                *tempText2 = tempCharacter;
                tempText2 += 1;
            } else {
                *tempText2 = 0;
                tempText2 += 1;
                tempText1 = skipWhitespace(tempText1);
                tempIsStartOfTerm = true;
            }
        }
    }
    int8_t tempTermListLength = tempTermIndex;
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
    if (strcmp((char *)(tempTermList[0]), "replace") == 0) {
        if (tempTermListLength != 3) {
            setActivityMode(PREVIOUS_MODE);
            notifyUser((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
        searchTermLength = strlen((char *)searchTerm);
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
        setActivityMode(PREVIOUS_MODE);
        notifyUser((int8_t *)"Changed file path.");
        return;
    }
    setActivityMode(PREVIOUS_MODE);
    notifyUser((int8_t *)"Error: Unrecognized command name.");
}
