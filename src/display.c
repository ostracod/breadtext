
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "syntax.h"
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
    (int8_t *)"O = Insert line before cursor and enter text-entry mode",
    (int8_t *)"Shift + O = Insert line after cursor and enter text-entry mode",
    (int8_t *)", + , (again) or Escape = Command mode",
    (int8_t *)"H = Character highlight mode",
    (int8_t *)"Shift + H = Line highlight mode",
    (int8_t *)"W = Word highlight mode",
    (int8_t *)"Shift + W = Highlight word with magic",
    (int8_t *)"E = Exclusive enclosure highlight mode",
    (int8_t *)"Shift + E = Inclusive enclosure highlight mode",
    (int8_t *)"9 = Select until beginning of line exclusive",
    (int8_t *)"Shift + 9 = Select until beginning of line inclusive",
    (int8_t *)"0 = Select until end of line exclusive",
    (int8_t *)"Shift + 0 = Select until end of line inclusive",
    (int8_t *)"/ = Enter command",
    (int8_t *)"Shift + / = Enter find command",
    (int8_t *)"V = Highlight line without indentation or newline",
    (int8_t *)"Shift + V = Enter gotoLine command",
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
    (int8_t *)"= = Scroll to end of indentation",
    (int8_t *)"G = Go to character exclusive",
    (int8_t *)"Shift + G = Go to character inclusive",
    (int8_t *)"R = Reverse go to character exclusive",
    (int8_t *)"Shift + R = Reverse go to character inclusive",
    (int8_t *)"N = Find next instance",
    (int8_t *)"Shift + N = Find previous instance",
    (int8_t *)"F = Find next instance of word under cursor",
    (int8_t *)"Shift + F = Find previous instance of word under cursor",
    (int8_t *)"1-8 = Go to mark",
    (int8_t *)"Shift + 1-8 = Set mark",
    (int8_t *)"",
    (int8_t *)"HIGHLIGHT ACTIONS",
    (int8_t *)"",
    (int8_t *)"D = Delete",
    (int8_t *)"Shift + D = Delete and enter text-entry mode",
    (int8_t *)"C = Copy",
    (int8_t *)"Shift + C = Change lines",
    (int8_t *)"X = Cut",
    (int8_t *)"Shift + X = Cut and enter text-entry mode",
    (int8_t *)"P = Paste after cursor",
    (int8_t *)"Shift + P = Paste before cursor",
    (int8_t *)"Y = Use internal clipboard",
    (int8_t *)"Shift + Y = Use system clipboard",
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
    (int8_t *)"_ = Toggle boolean literal",
    (int8_t *)"",
    (int8_t *)"COMMANDS",
    (int8_t *)"",
    (int8_t *)"/gotoLine (line number)",
    (int8_t *)"/find (pattern)",
    (int8_t *)"/reverseFind (pattern)",
    (int8_t *)"/findWord (word)",
    (int8_t *)"/reverseFindWord (word)",
    (int8_t *)"/regex (regex)",
    (int8_t *)"/reverseRegex (regex)",
    (int8_t *)"/replace (pattern) (text)",
    (int8_t *)"/set (config variable) (value)",
    (int8_t *)"/getPath",
    (int8_t *)"/setPath (path)",
    (int8_t *)"/version",
    (int8_t *)"/help",
    (int8_t *)"",
    (int8_t *)"CONFIGURATION VARIABLES",
    (int8_t *)"",
    (int8_t *)"colorScheme: 0 is black on white, 1 is white on black.",
    (int8_t *)"shouldUseHardTabs: 0 means no, 1 means yes.",
    (int8_t *)"indentationWidth: The number of spaces to use for soft tabs.",
    (int8_t *)"isCaseSensitive: 0 means no, 1 means yes.",
    (int8_t *)"shouldUseSystemClipboard: 0 means no, 1 means yes.",
    (int8_t *)"shouldHighlightSyntax: 0 means no, 1 means yes.",
    (int8_t *)"",
    (int8_t *)"On start-up, BreadText looks for the file ~/.breadtextrc to read configuration variables. Each line of .breadtextrc contains a variable name and a value separated by a space.",
    (int8_t *)"",
    (int8_t *)"Example contents of .breadtextrc file:",
    (int8_t *)"",
    (int8_t *)"colorScheme 0",
    (int8_t *)"shouldUseHardTabs 0",
    (int8_t *)"indentationWidth 4"
};

void setColorScheme(int32_t number) {
    if (number == 1) {
        colorSet[DEFAULT_COLOR] = WHITE_ON_BLACK;
        colorSet[COMMENT_COLOR] = RED_ON_BLACK;
        colorSet[LITERAL_COLOR] = GREEN_ON_BLACK;
        colorSet[KEYWORD_COLOR] = CYAN_ON_BLACK;
        colorSet[HIGHLIGHTED_DEFAULT_COLOR] = BLACK_ON_WHITE;
        colorSet[HIGHLIGHTED_COMMENT_COLOR] = RED_ON_WHITE;
        colorSet[HIGHLIGHTED_LITERAL_COLOR] = GREEN_ON_WHITE;
        colorSet[HIGHLIGHTED_KEYWORD_COLOR] = CYAN_ON_WHITE;
        return;
    }
    colorSet[DEFAULT_COLOR] = BLACK_ON_WHITE;
    colorSet[COMMENT_COLOR] = RED_ON_WHITE;
    colorSet[LITERAL_COLOR] = GREEN_ON_WHITE;
    colorSet[KEYWORD_COLOR] = CYAN_ON_WHITE;
    colorSet[HIGHLIGHTED_DEFAULT_COLOR] = WHITE_ON_BLACK;
    colorSet[HIGHLIGHTED_COMMENT_COLOR] = RED_ON_BLACK;
    colorSet[HIGHLIGHTED_LITERAL_COLOR] = GREEN_ON_BLACK;
    colorSet[HIGHLIGHTED_KEYWORD_COLOR] = CYAN_ON_BLACK;
}

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

int8_t getCursorColorIndex() {
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (index < cursorTextPos.line->textAllocation.length) {
        if (cursorTextPos.line->textAllocation.syntaxHighlighting == NULL) {
            return DEFAULT_COLOR;
        } else {
            return cursorTextPos.line->textAllocation.syntaxHighlighting[index];
        }
    } else {
        return DEFAULT_COLOR;
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
    int8_t tempColorIndex = getCursorColorIndex();
    attron(COLOR_PAIR(colorSet[tempColorIndex]));
    mvaddch(tempPosY, cursorTextPos.column, (char)getCursorCharacter());
    attroff(COLOR_PAIR(colorSet[tempColorIndex]));
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
    int8_t tempColorIndex = getCursorColorIndex() + HIGHLIGHTED_COLOR_OFFSET;
    attron(COLOR_PAIR(colorSet[tempColorIndex]));
    mvaddch(tempPosY, cursorTextPos.column, (char)getCursorCharacter());
    attroff(COLOR_PAIR(colorSet[tempColorIndex]));
}

void eraseTextCommandCursor() {
    if (activityMode != TEXT_COMMAND_MODE) {
        return;
    }
    int32_t tempPosX = strlen((char *)textCommandBuffer) + 1;
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvaddch(windowHeight - 1, tempPosX, ' ');
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void displayTextCommandCursor() {
    if (activityMode != TEXT_COMMAND_MODE) {
        return;
    }
    refresh();
    int32_t tempPosX = strlen((char *)textCommandBuffer) + 1;
    attron(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
    mvaddch(windowHeight - 1, tempPosX, ' ');
    attroff(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
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
    if (line->textAllocation.syntaxHighlighting == NULL) {
        generateSyntaxHighlighting(&(line->textAllocation));
    }
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
    int64_t tempBufferIndex = 0;
    textPos_t *tempFirstHighlightTextPos;
    textPos_t *tempLastHighlightTextPos;
    int64_t tempFirstHighlightIndex;
    int64_t tempLastHighlightIndex;
    int8_t tempFirstHighlightTextPosIsAfterLine;
    int8_t tempLastHighlightTextPosIsAfterLine;
    int8_t tempFirstHighlightTextPosIsOnLine;
    int8_t tempLastHighlightTextPosIsOnLine;
    int8_t tempFirstHighlightTextPosIsBeforeLine;
    int8_t tempLastHighlightTextPosIsBeforeLine;
    if (isHighlighting) {
        tempFirstHighlightTextPos = getFirstHighlightTextPos();
        tempLastHighlightTextPos = getLastHighlightTextPos();
        tempFirstHighlightIndex = getTextPosIndex(tempFirstHighlightTextPos);
        tempLastHighlightIndex = getTextPosIndex(tempLastHighlightTextPos);
        tempFirstHighlightTextPosIsAfterLine = textLineIsAfterTextLine(tempFirstHighlightTextPos->line, line);
        tempLastHighlightTextPosIsAfterLine = textLineIsAfterTextLine(tempLastHighlightTextPos->line, line);
        tempFirstHighlightTextPosIsOnLine = (tempFirstHighlightTextPos->line == line);
        tempLastHighlightTextPosIsOnLine = (tempLastHighlightTextPos->line == line);
        tempFirstHighlightTextPosIsBeforeLine = (!tempFirstHighlightTextPosIsAfterLine && !tempFirstHighlightTextPosIsOnLine);
        tempLastHighlightTextPosIsBeforeLine = (!tempLastHighlightTextPosIsAfterLine && !tempLastHighlightTextPosIsOnLine);
    }
    int8_t tempColor = -1;
    textPos_t tempStartTextPos;
    textPos_t tempTextPos;
    tempTextPos.line = line;
    tempTextPos.column = 0;
    tempTextPos.row = tempStartRow;
    while (tempTextPos.row < tempEndRow) {
        int64_t index = getTextPosIndex(&tempTextPos);
        int64_t tempIndexToHighlight;
        int8_t tempCharacter;
        int8_t tempNextColorIndex;
        if (index < line->textAllocation.length) {
            tempCharacter = line->textAllocation.text[index];
            if (line->textAllocation.syntaxHighlighting != NULL) {
                tempNextColorIndex = line->textAllocation.syntaxHighlighting[index];
            } else {
                tempNextColorIndex = DEFAULT_COLOR;
            }
            tempIndexToHighlight = index;
        } else {
            tempIndexToHighlight = line->textAllocation.length;
            tempCharacter = ' ';
            tempNextColorIndex = DEFAULT_COLOR;
        }
        if (isHighlighting && !tempFirstHighlightTextPosIsAfterLine && !tempLastHighlightTextPosIsBeforeLine) {
            if (tempFirstHighlightTextPosIsBeforeLine
                    || (tempFirstHighlightTextPosIsOnLine && tempFirstHighlightIndex <= tempIndexToHighlight)) {
                if (tempLastHighlightTextPosIsAfterLine
                        || (tempLastHighlightTextPosIsOnLine && tempLastHighlightIndex >= tempIndexToHighlight)) {
                    tempNextColorIndex += HIGHLIGHTED_COLOR_OFFSET;
                }
            }
        }
        int8_t tempNextColor = colorSet[tempNextColorIndex];
        if (tempNextColor != tempColor) {
            if (tempBufferIndex != 0) {
                tempBuffer[tempBufferIndex] = 0;
                convertTabsToSpaces(tempBuffer);
                attron(COLOR_PAIR(tempColor));
                mvprintw(posY + tempStartTextPos.row, tempStartTextPos.column, "%s", tempBuffer);
                attroff(COLOR_PAIR(tempColor));
            }
            tempStartTextPos = tempTextPos;
            tempColor = tempNextColor;
            tempBufferIndex = 0;
        }
        tempBuffer[tempBufferIndex] = tempCharacter;
        tempBufferIndex += 1;
        setTextPosIndex(&tempTextPos, index + 1);
    }
    if (tempBufferIndex != 0) {
        tempBuffer[tempBufferIndex] = 0;
        convertTabsToSpaces(tempBuffer);
        attron(COLOR_PAIR(tempColor));
        mvprintw(posY + tempStartTextPos.row, tempStartTextPos.column, "%s", tempBuffer);
        attroff(COLOR_PAIR(tempColor));
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
        attron(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
        mvprintw(tempPosY, 0, "%s", tempBuffer);
        attroff(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
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
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 0, "%s", tempBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void eraseActivityMode() {
    int8_t tempBuffer[activityModeTextLength + 1];
    tempBuffer[activityModeTextLength] = 0;
    int32_t index = 0;
    while (index < activityModeTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 0, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void displayActivityMode() {
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
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
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void eraseLineNumber() {
    int8_t tempBuffer[lineNumberTextLength + 1];
    tempBuffer[lineNumberTextLength] = 0;
    int32_t index = 0;
    while (index <= lineNumberTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, windowWidth - lineNumberTextLength, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void displayLineNumber() {
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    int8_t tempMessage[100];
    sprintf((char *)tempMessage, "Line %lld", (long long)(getTextLineNumber(cursorTextPos.line)));
    lineNumberTextLength = (int32_t)strlen((char *)tempMessage);
    mvprintw(windowHeight - 1, windowWidth - lineNumberTextLength, "%s", (char *)tempMessage);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
}

void eraseNotification() {
    int8_t tempBuffer[notificationTextLength + 1];
    tempBuffer[notificationTextLength] = 0;
    int32_t index = 0;
    while (index < notificationTextLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    mvprintw(windowHeight - 1, 0, "%s", (char *)tempBuffer);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    isShowingNotification = false;
}

void displayNotification(int8_t *message) {
    attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
    notificationTextLength = (int32_t)strlen((char *)message);
    mvprintw(windowHeight - 1, 0, "%s", (char *)message);
    attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
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
        attron(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
        mvprintw(windowHeight - 1, 0, "/");
        mvprintw(windowHeight - 1, 1, "%s", textCommandBuffer);
        attroff(COLOR_PAIR(colorSet[HIGHLIGHTED_DEFAULT_COLOR]));
        displayTextCommandCursor();
    } else {
        displayActivityMode();
        displayLineNumber();
    }
}

void displayHelpMessage() {
    bkgd(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
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
        attron(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
        mvprintw(tempPosY, 0, "%s", tempBuffer);
        attroff(COLOR_PAIR(colorSet[DEFAULT_COLOR]));
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
