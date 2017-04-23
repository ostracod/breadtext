
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "breadtext.h"

int8_t *helpText[] = {
    (int8_t *)"",
    (int8_t *)"= = = BREADTEXT HELP = = =",
    (int8_t *)"",
    (int8_t *)"SAVE AND QUIT",
    (int8_t *)"",
    (int8_t *)"S = Save",
    (int8_t *)"Shift + S = Save over modified file",
    (int8_t *)"Q = Quit",
    (int8_t *)"Shift + Q = Quit without saving",
    (int8_t *)"",
    (int8_t *)"CHANGING MODES",
    (int8_t *)"",
    (int8_t *)"T = Text-entry mode",
    (int8_t *)", + , (again) or Escape = Command mode",
    (int8_t *)"H = Character highlight mode",
    (int8_t *)"Shift + H = Line highlight mode",
    (int8_t *)"W = Word highlight mode",
    (int8_t *)"e = Exclusive enclosure highlight mode",
    (int8_t *)"E = Inclusive enclosure highlight mode",
    (int8_t *)"/ = Enter command",
    (int8_t *)"",
    (int8_t *)"MOVEMENT",
    (int8_t *)"",
    (int8_t *)"IJKL or Arrow Keys = Scroll one character",
    (int8_t *)"Shift + IJKL = Scroll 10 characters",
    (int8_t *)"AZ = Scroll text one line",
    (int8_t *)"Shift + AZ = Scroll text 10 lines",
    (int8_t *)"` = Scroll text to cursor",
    (int8_t *)"Shift + ` = Scroll cursor to text",
    (int8_t *)"[] = Scroll to beginning or end of line",
    (int8_t *)"{} = Scroll to beginning or end of file",
    (int8_t *)"N = Find next instance",
    (int8_t *)"Shift + N = Find previous instance",
    (int8_t *)"F = Find next instance of word under cursor",
    (int8_t *)"Shift + F = Find next instance of word under cursor",    
    (int8_t *)"Number = Go to mark",
    (int8_t *)"Shift + Number = Set mark",
    (int8_t *)"",
    (int8_t *)"HIGHLIGHT ACTIONS",
    (int8_t *)"",
    (int8_t *)"D = Delete",
    (int8_t *)"Shift + D = Delete and enter text-entry mode",
    (int8_t *)"C = Copy",
    (int8_t *)"X = Cut",
    (int8_t *)"Shift + X = Cut and enter text-entry mode",
    (int8_t *)"P = Paste after cursor",
    (int8_t *)"Shift + P = Paste before cursor",
    (int8_t *)"",
    (int8_t *)"HISTORY",
    (int8_t *)"",    
    (int8_t *)"M = Play macro",
    (int8_t *)"Shift + M = Start or stop recording macro",
    (int8_t *)"U = Undo",
    (int8_t *)"Shift + U = Redo",
    (int8_t *)"",
    (int8_t *)"TEXT MANIPULATION",
    (int8_t *)"",
    (int8_t *)"<> = Indent",
    (int8_t *)"Tab or Shift + Tab = Indent",
    (int8_t *)". = Replace character under cursor",
    (int8_t *)"' = Insert character after cursor",
    (int8_t *)"Shift + ' = Insert character before cursor",
    (int8_t *)"\\ = Insert line after cursor",
    (int8_t *)"Shift + \\ = Insert line before cursor",
    (int8_t *)"; = Toggle semicolon at end of line",
    (int8_t *)": = Toggle comment at beginning of line",
    (int8_t *)"B = Lowercase",
    (int8_t *)"Shift + B = Uppercase",
    (int8_t *)"+ = Increment number under cursor",
    (int8_t *)"- = Decrement number under cursor",
    (int8_t *)"",
    (int8_t *)"COMMANDS",
    (int8_t *)"",
    (int8_t *)"/gotoLine (line number)",
    (int8_t *)"/find (pattern)",
    (int8_t *)"/reverseFind (pattern)",
    (int8_t *)"/replace (pattern) (text)",
    (int8_t *)"/set (config variable) (value)",
    (int8_t *)"/getPath",
    (int8_t *)"/setPath (path)",
    (int8_t *)"/help",
    (int8_t *)"",
    (int8_t *)"CONFIGURATION VARIABLES",
    (int8_t *)"",
    (int8_t *)"colorScheme: 0 is black on white, 1 is white on black.",
    (int8_t *)"shouldUseHardTabs: 0 means no, 1 means yes.",
    (int8_t *)"indentationWidth: The number of spaces to use for soft tabs.",
    (int8_t *)"",
    (int8_t *)"On start-up, breadtext looks for the file ~/.breadtextrc to read configuration variables. Each line of .breadtextrc contains a variable name and a value separated by a space.",
    (int8_t *)"",
    (int8_t *)"Example contents of .breadtextrc file:",
    (int8_t *)"",
    (int8_t *)"colorScheme 0",
    (int8_t *)"shouldUseHardTabs 0",
    (int8_t *)"indentationWidth 4"
};

int64_t getTextLineRowCount(textLine_t *line) {
    return line->textAllocation.length / viewPortWidth + 1;
}

int64_t getTextLinePosY(textLine_t *line) {
    if (textLineIsAfterTextLine(line, topTextLine)) {
        textLine_t *tempLine = topTextLine;
        int64_t tempPosY = -topTextLineRow;
        while (tempLine != line) {
            tempPosY += getTextLineRowCount(tempLine);
            tempLine = getNextTextLine(tempLine);
        }
        return tempPosY;
    } else {
        textLine_t *tempLine = topTextLine;
        int64_t tempPosY = -topTextLineRow;
        while (tempLine != line) {
            tempLine = getPreviousTextLine(tempLine);
            tempPosY -= getTextLineRowCount(tempLine);
        }
        return tempPosY;
    }
}

int64_t getCursorPosY() {
    return getTextLinePosY(cursorTextPos.line) + cursorTextPos.row;
}

int8_t getCursorCharacter() {
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (index < cursorTextPos.line->textAllocation.length) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[index];
        if (tempCharacter == '\t') {
            return ' ';
        }
        return tempCharacter;
    } else {
        return ' ';
    }
}

void eraseCursor() {
    if (isHighlighting) {
        return;
    }
    int32_t tempPosY = getCursorPosY();
    if (tempPosY < 0 || tempPosY >= viewPortHeight) {
        return;
    }
    attron(COLOR_PAIR(primaryColorPair));
    mvaddch(tempPosY, cursorTextPos.column, (char)getCursorCharacter());
    attroff(COLOR_PAIR(primaryColorPair));
}

void displayCursor() {
    if (isHighlighting) {
        return;
    }
    int32_t tempPosY = getCursorPosY();
    if (tempPosY < 0 || tempPosY >= viewPortHeight) {
        return;
    }
    refresh();
    attron(COLOR_PAIR(secondaryColorPair));
    mvaddch(tempPosY, cursorTextPos.column, (char)getCursorCharacter());
    attroff(COLOR_PAIR(secondaryColorPair));
}

void eraseTextCommandCursor() {
    if (activityMode != TEXT_COMMAND_MODE) {
        return;
    }
    int32_t tempPosX = strlen((char *)textCommandBuffer) + 1;
    attron(COLOR_PAIR(secondaryColorPair));
    mvaddch(windowHeight - 1, tempPosX, ' ');
    attroff(COLOR_PAIR(secondaryColorPair));
}

void displayTextCommandCursor() {
    if (activityMode != TEXT_COMMAND_MODE) {
        return;
    }
    refresh();
    int32_t tempPosX = strlen((char *)textCommandBuffer) + 1;
    attron(COLOR_PAIR(primaryColorPair));
    mvaddch(windowHeight - 1, tempPosX, ' ');
    attroff(COLOR_PAIR(primaryColorPair));
}

textPos_t *getFirstHighlightTextPos() {
    if (textPosIsAfterTextPos(&cursorTextPos, &highlightTextPos)) {
        return &highlightTextPos;
    }
    return &cursorTextPos;
}

textPos_t *getLastHighlightTextPos() {
    if (textPosIsAfterTextPos(&cursorTextPos, &highlightTextPos)) {
        return &cursorTextPos;
    }
    return &highlightTextPos;
}

// Returns the next Y position.
int64_t displayTextLine(int64_t posY, textLine_t *line) {
    int64_t tempRowCount = getTextLineRowCount(line);
    if (posY + tempRowCount <= 0 || posY >= viewPortHeight) {
        return posY + tempRowCount;
    }
    int64_t tempStartRow;
    if (posY < 0) {
        tempStartRow = -posY;
    } else {
        tempStartRow = 0;
    }
    int64_t tempEndRow;
    if (posY + tempRowCount > viewPortHeight) {
        tempEndRow = viewPortHeight - posY;
    } else {
        tempEndRow = tempRowCount;
    }
    int64_t tempLength = (tempEndRow - tempStartRow) * viewPortWidth;
    int8_t tempBuffer[tempLength + 1];
    int64_t tempStartIndex = tempStartRow * viewPortWidth;
    int64_t tempEndIndex = tempEndRow * viewPortWidth;
    if (tempEndIndex > line->textAllocation.length) {
        tempEndIndex = line->textAllocation.length;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    if (isHighlighting) {
        int64_t tempStartIndex2;
        int64_t tempEndIndex2;
        int64_t tempAmount2;
        textPos_t *tempFirstTextPos = getFirstHighlightTextPos();
        textPos_t *tempLastTextPos = getLastHighlightTextPos();
        textPos_t tempStartTextPos;
        textPos_t tempEndTextPos;
        tempStartTextPos.line = line;
        tempStartTextPos.row = tempStartRow;
        tempStartTextPos.column = 0;
        tempEndTextPos.line = line;
        if (tempFirstTextPos->line == line) {
            if (getTextPosIndex(tempFirstTextPos) < tempStartIndex) {
                setTextPosIndex(&tempEndTextPos, tempStartIndex);
            } else if (getTextPosIndex(tempFirstTextPos) > tempEndIndex) {
                setTextPosIndex(&tempEndTextPos, tempEndIndex);
            } else {
                tempEndTextPos.row = tempFirstTextPos->row;
                tempEndTextPos.column = tempFirstTextPos->column;
            }
        } else if (textLineIsAfterTextLine(tempFirstTextPos->line, line)) {
            setTextPosIndex(&tempEndTextPos, tempEndIndex);
        } else {
            tempEndTextPos = tempStartTextPos;
        }
        tempStartIndex2 = getTextPosIndex(&tempStartTextPos);
        tempEndIndex2 = getTextPosIndex(&tempEndTextPos);
        tempAmount2 = tempEndIndex2 - tempStartIndex2;
        if (tempAmount2 > 0) {
            copyData(tempBuffer, line->textAllocation.text + tempStartIndex2, tempAmount2);
            tempBuffer[tempAmount2] = 0;
            convertTabsToSpaces(tempBuffer);
            attron(COLOR_PAIR(primaryColorPair));
            mvprintw(posY + tempStartTextPos.row, tempStartTextPos.column, "%s", tempBuffer);
            attroff(COLOR_PAIR(primaryColorPair));
        }
        tempStartTextPos = tempEndTextPos;
        if (tempLastTextPos->line == line) {
            if (getTextPosIndex(tempLastTextPos) < tempStartIndex) {
                setTextPosIndex(&tempEndTextPos, tempStartIndex);
            } else if (getTextPosIndex(tempLastTextPos) + 1 > tempEndIndex) {
                setTextPosIndex(&tempEndTextPos, tempEndIndex);
            } else {
                tempEndTextPos.row = tempLastTextPos->row;
                tempEndTextPos.column = tempLastTextPos->column;
                tempEndTextPos.column += 1;
                if (tempEndTextPos.column >= viewPortWidth) {
                    tempEndTextPos.column = 0;
                    tempEndTextPos.row += 1;
                }
            }
        } else if (textLineIsAfterTextLine(tempLastTextPos->line, line)) {
            setTextPosIndex(&tempEndTextPos, tempEndIndex);
        } else {
            tempEndTextPos = tempStartTextPos;
        }
        tempStartIndex2 = getTextPosIndex(&tempStartTextPos);
        tempEndIndex2 = getTextPosIndex(&tempEndTextPos);
        tempAmount2 = tempEndIndex2 - tempStartIndex2;
        if (tempAmount2 > 0) {
            copyData(tempBuffer, line->textAllocation.text + tempStartIndex2, tempAmount2);
            tempBuffer[tempAmount2] = 0;
            attron(COLOR_PAIR(secondaryColorPair));
            convertTabsToSpaces(tempBuffer);
            mvprintw(posY + tempStartTextPos.row, tempStartTextPos.column, "%s", tempBuffer);
            attroff(COLOR_PAIR(secondaryColorPair));
        }
        tempStartTextPos = tempEndTextPos;
        setTextPosIndex(&tempEndTextPos, tempEndIndex);
        tempStartIndex2 = getTextPosIndex(&tempStartTextPos);
        tempEndIndex2 = getTextPosIndex(&tempEndTextPos);
        tempAmount2 = tempEndIndex2 - tempStartIndex2;
        if (tempAmount2 > 0) {
            copyData(tempBuffer, line->textAllocation.text + tempStartIndex2, tempAmount2);
            tempBuffer[tempAmount2] = 0;
            attron(COLOR_PAIR(primaryColorPair));
            convertTabsToSpaces(tempBuffer);
            mvprintw(posY + tempStartTextPos.row, tempStartTextPos.column, "%s", tempBuffer);
            attroff(COLOR_PAIR(primaryColorPair));
        }
        tempAmount2 = tempLength - tempAmount;
        if (tempAmount2 > 0) {
            int64_t index = 0;
            while (index < tempAmount2) {
                tempBuffer[index] = ' ';
                index += 1;
            }
            tempBuffer[tempAmount2] = 0;
            textPos_t tempTextPos;
            tempTextPos.line = line;
            setTextPosIndex(&tempTextPos, tempEndIndex);
            int8_t tempColor;
            if (!textPosIsAfterTextPos(tempFirstTextPos, &tempTextPos) && !textPosIsAfterTextPos(&tempTextPos, tempLastTextPos)) {
                tempColor = secondaryColorPair;
            } else {
                tempColor = primaryColorPair;
            }
            attron(COLOR_PAIR(tempColor));
            mvprintw(posY + tempTextPos.row, tempTextPos.column, "%s", tempBuffer);
            attroff(COLOR_PAIR(tempColor));
        }
    } else {
        copyData(tempBuffer, line->textAllocation.text + tempStartIndex, tempAmount);
        int64_t index = tempAmount;
        while (index < tempLength) {
            tempBuffer[index] = ' ';
            index += 1;
        }
        tempBuffer[tempLength] = 0;
        attron(COLOR_PAIR(primaryColorPair));
        convertTabsToSpaces(tempBuffer);
        mvprintw(posY + tempStartRow, 0, "%s", tempBuffer);
        attroff(COLOR_PAIR(primaryColorPair));
    }
    return posY + tempEndRow;
}

void displayTextLinesUnderAndIncludingTextLine(int64_t posY, textLine_t *line) {
    textLine_t *tempLine = line;
    int64_t tempPosY = posY;
    while (tempPosY < viewPortHeight && tempLine != NULL) {
        tempPosY = displayTextLine(tempPosY, tempLine);
        tempLine = getNextTextLine(tempLine);
    }
    if (tempPosY < viewPortHeight) {
        int64_t tempLength = (viewPortHeight - tempPosY) * viewPortWidth;
        int64_t tempSize = tempLength + 1;
        int8_t tempBuffer[tempSize];
        tempBuffer[tempSize - 1] = 0;
        int64_t index = 0;
        while (index < tempLength) {
            tempBuffer[index] = ' ';
            index += 1;
        }
        attron(COLOR_PAIR(primaryColorPair));
        mvprintw(tempPosY, 0, "%s", tempBuffer);
        attroff(COLOR_PAIR(primaryColorPair));
    }
}

void displayAllTextLines() {
    displayTextLinesUnderAndIncludingTextLine(-topTextLineRow, topTextLine);
    displayCursor();
}

void eraseStatusBar() {
    int32_t tempLength = windowWidth;
    int32_t tempSize = tempLength + 1;
    int8_t tempBuffer[tempSize];
    tempBuffer[tempSize - 1] = 0;
    int32_t index = 0;
    while (index < tempLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(secondaryColorPair));
    mvprintw(windowHeight - 1, 0, "%s", tempBuffer);
    attroff(COLOR_PAIR(secondaryColorPair));
}

void eraseActivityMode() {
    int8_t tempBuffer[activityModeTextLength + 1];
    tempBuffer[activityModeTextLength] = 0;
    int32_t index = 0;
    while (index < activityModeTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(secondaryColorPair));
    mvprintw(windowHeight - 1, 0, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(secondaryColorPair));
}

void displayActivityMode() {
    attron(COLOR_PAIR(secondaryColorPair));
    if (activityMode == COMMAND_MODE) {
        int8_t tempMessage[] = "Command Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == TEXT_ENTRY_MODE) {
        int8_t tempMessage[] = "Text Entry Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == HIGHLIGHT_CHARACTER_MODE) {
        int8_t tempMessage[] = "Character Highlight Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == HIGHLIGHT_LINE_MODE) {
        int8_t tempMessage[] = "Line Highlight Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == HIGHLIGHT_STATIC_MODE) {
        int8_t tempMessage[] = "Static Highlight Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == TEXT_COMMAND_MODE) {
        activityModeTextLength = 0;
    }
    attroff(COLOR_PAIR(secondaryColorPair));
}

void eraseLineNumber() {
    int8_t tempBuffer[lineNumberTextLength + 1];
    tempBuffer[lineNumberTextLength] = 0;
    int32_t index = 0;
    while (index <= lineNumberTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(secondaryColorPair));
    mvprintw(windowHeight - 1, windowWidth - lineNumberTextLength, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(secondaryColorPair));
}

void displayLineNumber() {
    attron(COLOR_PAIR(secondaryColorPair));
    int8_t tempMessage[100];
    sprintf((char *)tempMessage, "Line %lld", (long long)(getTextLineNumber(cursorTextPos.line)));
    lineNumberTextLength = (int32_t)strlen((char *)tempMessage);
    mvprintw(windowHeight - 1, windowWidth - lineNumberTextLength, "%s", (char *)tempMessage);
    attroff(COLOR_PAIR(secondaryColorPair));
}

void eraseNotification() {
    int8_t tempBuffer[notificationTextLength + 1];
    tempBuffer[notificationTextLength] = 0;
    int32_t index = 0;
    while (index < notificationTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(secondaryColorPair));
    mvprintw(windowHeight - 1, 0, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(secondaryColorPair));
    isShowingNotification = false;
}

void displayNotification(int8_t *message) {
    attron(COLOR_PAIR(secondaryColorPair));
    notificationTextLength = (int32_t)strlen((char *)message);
    mvprintw(windowHeight - 1, 0, "%s", (char *)message);
    attroff(COLOR_PAIR(secondaryColorPair));
    isShowingNotification = true;
}

void eraseActivityModeOrNotification() {
    if (isShowingNotification) {
        eraseNotification();
    } else {
        eraseActivityMode();
    }
}

void notifyUser(int8_t *message) {
    eraseActivityModeOrNotification();
    displayNotification(message);
}

void displayStatusBar() {
    eraseStatusBar();
    if (activityMode == TEXT_COMMAND_MODE) {
        attron(COLOR_PAIR(secondaryColorPair));
        mvprintw(windowHeight - 1, 0, "/");
        mvprintw(windowHeight - 1, 1, "%s", textCommandBuffer);
        attroff(COLOR_PAIR(secondaryColorPair));
        displayTextCommandCursor();
    } else {
        displayActivityMode();
        displayLineNumber();
    }
}

void displayHelpMessage() {
    bkgd(COLOR_PAIR(primaryColorPair));
    clear();
    int64_t tempPosY = 0;
    int64_t tempLength = sizeof(helpText) / sizeof(*helpText);
    int64_t index = helpScroll;
    while (index < tempLength && tempPosY < windowHeight) {
        int64_t tempLength2 = strlen((char *)(helpText[index]));
        int64_t tempRowCount = tempLength2 / windowWidth + 1;
        int8_t tempBuffer[tempRowCount * windowWidth + 1];
        strcpy((char *)tempBuffer, (char *)(helpText[index]));
        if (tempPosY + tempRowCount >= windowHeight) {
            int64_t tempRowCount2 = windowHeight - tempPosY;
            tempBuffer[tempRowCount2 * windowWidth] = 0;
        }
        attron(COLOR_PAIR(primaryColorPair));
        mvprintw(tempPosY, 0, "%s", tempBuffer);
        attroff(COLOR_PAIR(primaryColorPair));
        tempPosY += tempRowCount;
        index += 1;
    }
}

void scrollHelpMessageUp(int64_t amount) {
    int64_t tempCount = 0;
    while (tempCount < amount) {
        if (helpScroll > 0) {
            helpScroll -= 1;
            redrawEverything();
        } else {
            break;
        }
        tempCount += 1;
    }
}

void scrollHelpMessageDown(int64_t amount) {
    int64_t tempLength = sizeof(helpText) / sizeof(*helpText);
    int64_t tempCount = 0;
    while (tempCount < amount) {
        if (helpScroll < tempLength - 1) {
            helpScroll += 1;
            redrawEverything();
        } else {
            break;
        }
        tempCount += 1;
    }
}

void redrawEverything() {
    if (activityMode == HELP_MODE) {
        displayHelpMessage();
    } else {
        displayAllTextLines();
        displayStatusBar();
    }
}

int8_t scrollCursorOntoScreen() {
    int64_t tempPosY = getCursorPosY();
    int64_t tempScrollOffset = 8;
    if (tempScrollOffset > viewPortHeight / 2) {
        tempScrollOffset = viewPortHeight / 2;
    }
    if (tempPosY < 0) {
        while (tempPosY < tempScrollOffset) {
            if (topTextLineRow <= 0) {
                textLine_t *tempLine = getPreviousTextLine(topTextLine);
                if (tempLine == NULL) {
                    break;
                }
                topTextLine = tempLine;
                topTextLineRow = getTextLineRowCount(topTextLine) - 1;
            } else {
                topTextLineRow -= 1;
            }
            tempPosY += 1;
        }
        displayAllTextLines();
        return true;
    }
    if (tempPosY >= viewPortHeight) {
        int64_t tempRowCount = getTextLineRowCount(topTextLine);
        while (tempPosY >= viewPortHeight - tempScrollOffset) {
            if (topTextLineRow >= tempRowCount - 1) {
                textLine_t *tempLine = getNextTextLine(topTextLine);
                if (tempLine == NULL) {
                    break;
                }
                topTextLine = tempLine;
                topTextLineRow = 0;
                tempRowCount = getTextLineRowCount(topTextLine);
            } else {
                topTextLineRow += 1;
            }
            tempPosY -= 1;
        }
        displayAllTextLines();
        return true;
    }
    return false;
}
