
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>

#define true 1
#define false 0

#define BLACK_ON_WHITE 1
#define WHITE_ON_BLACK 2

#define COMMAND_MODE 1
#define TEXT_ENTRY_MODE 2
#define HIGHLIGHT_CHARACTER_MODE 3
#define HIGHLIGHT_WORD_MODE 4
#define HIGHLIGHT_LINE_MODE 5

#define SHOULD_RUN_TESTS false

typedef struct textAllocation {
    int8_t *text;
    int64_t length;
    int64_t allocationSize;
} textAllocation_t;

typedef struct textLine textLine_t;
struct textLine {
    textLine_t *parent;
    textLine_t *leftChild;
    textLine_t *rightChild;
    textAllocation_t textAllocation;
    int8_t depth;
    int64_t lineCount;
};

textLine_t *rootTextLine = NULL;
textLine_t *topTextLine = NULL;
int64_t topTextLineRow = 0;
textLine_t *cursorTextLine = NULL;
int64_t cursorTextLineRow = 0;
int64_t cursorTextLineColumn = 0;
int64_t cursorTextLineSnapColumn = 0;
int8_t activityMode = COMMAND_MODE;
int32_t activityModeTextLength = 0;
int32_t lineNumberTextLength = 0;
WINDOW *window;
int32_t windowWidth = -1;
int32_t windowHeight = -1;
int32_t viewPortWidth = -1;
int32_t viewPortHeight = -1;
int8_t primaryColorPair = BLACK_ON_WHITE;
int8_t secondaryColorPair = WHITE_ON_BLACK;
int8_t *filePath;

void copyData(int8_t *destination, int8_t *source, int64_t amount) {
    if (destination < source) {
        int64_t index = 0;
        while (index < amount) {
            destination[index] = source[index];
            index += 1;
        }
    } else {
        int64_t index = amount - 1;
        while (index >= 0) {
            destination[index] = source[index];
            index -= 1;
        }
    }
}

void setTextAllocationSize(textAllocation_t *allocation, int64_t size) {
    int8_t *tempText = malloc(size);
    if (allocation->text != NULL) {
        int64_t tempSize;
        if (size < allocation->allocationSize) {
            tempSize = size;
        } else {
            tempSize = allocation->allocationSize;
        }
        copyData(tempText, allocation->text, tempSize);
        free(allocation->text);
    }
    allocation->text = tempText;
}

void insertTextIntoTextAllocation(textAllocation_t *allocation, int64_t index, int8_t *text, int64_t amount) {
    int64_t tempLength = allocation->length + amount;
    if (tempLength > allocation->allocationSize || allocation->text == NULL) {
        setTextAllocationSize(allocation, tempLength * 2);
    }
    int64_t tempAmount = allocation->length - index;
    copyData(allocation->text + index + amount, allocation->text + index, tempAmount);
    copyData(allocation->text + index, text, amount);
    allocation->length = tempLength;
}

void removeTextFromTextAllocation(textAllocation_t *allocation, int64_t index, int64_t amount) {
    int64_t tempLength = allocation->length - amount;
    int64_t tempAmount = allocation->length - index;
    copyData(allocation->text + index, allocation->text + index + amount, tempAmount);
    if (tempLength < allocation->allocationSize / 4) {
        setTextAllocationSize(allocation, tempLength * 2);
    }
    allocation->length = tempLength;
}

void cleanUpTextAllocation(textAllocation_t *allocation) {
    if (allocation->text != NULL) {
        free(allocation->text);
    }
}

textLine_t *createEmptyTextLine() {
    textLine_t *output = malloc(sizeof(textLine_t));
    output->parent = NULL;
    output->leftChild = NULL;
    output->rightChild = NULL;
    output->textAllocation.text = NULL;
    output->textAllocation.length = 0;
    output->textAllocation.allocationSize = 0;
    output->depth = 1;
    output->lineCount = 1;
    return output;
}

textLine_t *getLeftmostTextLine(textLine_t *line) {
    textLine_t *tempLine = line;
    while (true) {
        textLine_t *tempChild = tempLine->leftChild;
        if (tempChild == NULL) {
            return tempLine;
        }
        tempLine = tempChild;
    }
}

textLine_t *getRightmostTextLine(textLine_t *line) {
    textLine_t *tempLine = line;
    while (true) {
        textLine_t *tempChild = tempLine->rightChild;
        if (tempChild == NULL) {
            return tempLine;
        }
        tempLine = tempChild;
    }
}

void updateTextLineInfoAboutChildren(textLine_t *line) {
    textLine_t *tempLeftChild = line->leftChild;
    textLine_t *tempRightChild = line->rightChild;
    int8_t tempLeftDepth;
    int8_t tempRightDepth;
    int64_t tempLeftLineCount;
    int64_t tempRightLineCount;
    if (tempLeftChild == NULL) {
        tempLeftDepth = 0;
        tempLeftLineCount = 0;
    } else {
        tempLeftDepth = tempLeftChild->depth;
        tempLeftLineCount = tempLeftChild->lineCount;
    }
    if (tempRightChild == NULL) {
        tempRightDepth = 0;
        tempRightLineCount = 0;
    } else {
        tempRightDepth = tempRightChild->depth;
        tempRightLineCount = tempRightChild->lineCount;
    }
    int8_t tempDepth;
    if (tempRightDepth > tempLeftDepth) {
        tempDepth = tempRightDepth;
    } else {
        tempDepth = tempLeftDepth;
    }
    line->depth = tempDepth + 1;
    line->lineCount = tempLeftLineCount + tempRightLineCount + 1;
}

void replaceTextLine(textLine_t *oldLine, textLine_t *newLine) {
    textLine_t *tempParent = oldLine->parent;
    if (newLine != NULL) {
        newLine->parent = tempParent;
    }
    if (tempParent == NULL) {
        rootTextLine = newLine;
    } else {
        if (tempParent->leftChild == oldLine) {
            tempParent->leftChild = newLine;
        } else {
            tempParent->rightChild = newLine;
        }
    }
}

void rotateTextLineLeft(textLine_t *line) {
    textLine_t *tempChild1 = line->rightChild;
    textLine_t *tempChild2 = tempChild1->leftChild;
    replaceTextLine(line, tempChild1);
    tempChild1->leftChild = line;
    line->parent = tempChild1;
    line->rightChild = tempChild2;
    if (tempChild2 != NULL) {
        tempChild2->parent = line;
    }
    updateTextLineInfoAboutChildren(line);
    updateTextLineInfoAboutChildren(tempChild1);
}

void rotateTextLineRight(textLine_t *line) {
    textLine_t *tempChild1 = line->leftChild;
    textLine_t *tempChild2 = tempChild1->rightChild;
    replaceTextLine(line, tempChild1);
    tempChild1->rightChild = line;
    line->parent = tempChild1;
    line->leftChild = tempChild2;
    if (tempChild2 != NULL) {
        tempChild2->parent = line;
    }
    updateTextLineInfoAboutChildren(line);
    updateTextLineInfoAboutChildren(tempChild1);
}

int8_t getTextLineBalance(textLine_t *line) {
    textLine_t *tempLeftChild = line->leftChild;
    textLine_t *tempRightChild = line->rightChild;
    int8_t tempLeftDepth;
    int8_t tempRightDepth;
    if (tempLeftChild == NULL) {
        tempLeftDepth = 0;
    } else {
        tempLeftDepth = tempLeftChild->depth;
    }
    if (tempRightChild == NULL) {
        tempRightDepth = 0;
    } else {
        tempRightDepth = tempRightChild->depth;
    }
    return tempLeftDepth - tempRightDepth;
}

void balanceTextLine(textLine_t *line) {
    int8_t tempBalance = getTextLineBalance(line);
    if (tempBalance > 1) {
        textLine_t *tempChild = line->leftChild;
        int8_t tempBalance2 = getTextLineBalance(tempChild);
        if (tempBalance2 < 0) {
            rotateTextLineLeft(tempChild);
        }
        rotateTextLineRight(line);
    } else if (tempBalance < -1) {
        textLine_t *tempChild = line->rightChild;
        int8_t tempBalance2 = getTextLineBalance(tempChild);
        if (tempBalance2 > 0) {
            rotateTextLineRight(tempChild);
        }
        rotateTextLineLeft(line);
    } else {
        updateTextLineInfoAboutChildren(line);
    }
}

void balanceTextLinesToRoot(textLine_t *line) {
    textLine_t *tempLine = line;
    textLine_t *tempParent = tempLine->parent;
    while (true) {
        balanceTextLine(tempLine);
        tempLine = tempParent;
        if (tempLine == NULL) {
            break;
        }
        // Parent must be stored because balanceTextLine changes the parent.
        tempParent = tempLine->parent;
    }
}

void insertTextLineLeft(textLine_t *parent, textLine_t *child) {
    textLine_t *tempLineToBalance;
    if (parent->leftChild == NULL) {
        tempLineToBalance = parent;
        parent->leftChild = child;
        child->parent = parent;
    } else {
        textLine_t *tempLine = getRightmostTextLine(parent->leftChild);
        tempLineToBalance = tempLine;
        tempLine->rightChild = child;
        child->parent = tempLine;
    }
    balanceTextLinesToRoot(tempLineToBalance);
}

void insertTextLineRight(textLine_t *parent, textLine_t *child) {
    textLine_t *tempLineToBalance;
    if (parent->rightChild == NULL) {
        tempLineToBalance = parent;
        parent->rightChild = child;
        child->parent = parent;
    } else {
        textLine_t *tempLine = getLeftmostTextLine(parent->rightChild);
        tempLineToBalance = tempLine;
        tempLine->leftChild = child;
        child->parent = tempLine;
    }
    balanceTextLinesToRoot(tempLineToBalance);
}

void deleteTextLine(textLine_t *line) {
    cleanUpTextAllocation(&(line->textAllocation));
    textLine_t *tempLineToBalance;
    textLine_t *tempChild1 = line->rightChild;
    textLine_t *tempChild2 = line->leftChild;
    if (tempChild2 == NULL) {
        tempLineToBalance = line->parent;
        replaceTextLine(line, tempChild1);
    } else {
        textLine_t *tempChild3 = getLeftmostTextLine(tempChild2);
        if (tempChild3 == tempChild2) {
            tempLineToBalance = tempChild3;
            replaceTextLine(line, tempChild3);
            tempChild3->leftChild = tempChild1;
            if (tempChild1 != NULL) {
                tempChild1->parent = tempChild3;
            }
        } else {
            tempLineToBalance = tempChild3->parent;
            replaceTextLine(tempChild3, tempChild3->rightChild);
            replaceTextLine(line, tempChild3);
            tempChild3->leftChild = tempChild1;
            if (tempChild1 != NULL) {
                tempChild1->parent = tempChild3;
            }
            tempChild3->rightChild = tempChild2;
            tempChild2->parent = tempChild3;
        }
    }
    if (tempLineToBalance != NULL) {
        balanceTextLinesToRoot(tempLineToBalance);
    }
    free(line);
}

textLine_t *getPreviousTextLine(textLine_t *line) {
    textLine_t *tempChild = line->leftChild;
    if (tempChild == NULL) {
        textLine_t *tempLine = line;
        while (true) {
            textLine_t *tempParent = tempLine->parent;
            if (tempParent == NULL) {
                return NULL;
            }
            if (tempParent->rightChild == tempLine) {
                return tempParent;
            }
            tempLine = tempParent;
        }
    } else {
        return getRightmostTextLine(tempChild);
    }
}

textLine_t *getNextTextLine(textLine_t *line) {
    textLine_t *tempChild = line->rightChild;
    if (tempChild == NULL) {
        textLine_t *tempLine = line;
        while (true) {
            textLine_t *tempParent = tempLine->parent;
            if (tempParent == NULL) {
                return NULL;
            }
            if (tempParent->leftChild == tempLine) {
                return tempParent;
            }
            tempLine = tempParent;
        }
    } else {
        return getLeftmostTextLine(tempChild);
    }
}

int64_t getTextLineNumber(textLine_t *line) {
    int64_t output = 1;
    textLine_t *tempChild = line->leftChild;
    if (tempChild != NULL) {
        output += tempChild->lineCount;
    }
    tempChild = line;
    textLine_t *tempParent = line->parent;
    while (tempParent != NULL) {
        textLine_t *tempChild2 = tempParent->leftChild;
        if (tempChild2 != tempChild) {
            output += 1;
            if (tempChild2 != NULL) {
                output += tempChild2->lineCount;
            }
        }
        tempChild = tempParent;
        tempParent = tempChild->parent;
    }
    return output;
}

int8_t textLineTreeIsBalanced(textLine_t *line) {
    textLine_t *tempChild;
    tempChild = line->leftChild;
    if (tempChild != NULL) {
        if (!textLineTreeIsBalanced(tempChild)) {
            return false;
        }
    }
    tempChild = line->rightChild;
    if (tempChild != NULL) {
        if (!textLineTreeIsBalanced(tempChild)) {
            return false;
        }
    }
    updateTextLineInfoAboutChildren(line);
    int8_t tempBalance = getTextLineBalance(line);
    return (tempBalance >= -1 && tempBalance <= 1);
}

void printTextLineStructure(textLine_t *line) {
    if (line == NULL) {
        return;
    }
    textLine_t *tempChild;
    tempChild = line->leftChild;
    if (tempChild != NULL) {
        printTextLineStructure(tempChild);
    }
    int8_t tempCount = 0;
    while (tempCount < line->depth) {
        printf("|");
        tempCount += 1;
    }
    printf("O\n");
    tempChild = line->rightChild;
    if (tempChild != NULL) {
        printTextLineStructure(tempChild);
    }
}

void runTests() {
    int32_t tempCount;
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        } else {
            textLine_t *tempLine = getRightmostTextLine(rootTextLine);
            textLine_t *tempLine2 = createEmptyTextLine();
            insertTextLineRight(tempLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 1.\n");
                return;
            }
        }
        tempCount += 1;
    }
    printf("Passed test 1.\n");
    while (rootTextLine != NULL) {
        textLine_t *tempLine = getRightmostTextLine(rootTextLine);
        deleteTextLine(tempLine);
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 2.\n");
                return;
            }
        }
    }
    printf("Passed test 2.\n");
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        } else {
            textLine_t *tempLine = getLeftmostTextLine(rootTextLine);
            textLine_t *tempLine2 = createEmptyTextLine();
            insertTextLineLeft(tempLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 3.\n");
                return;
            }
        }
        tempCount += 1;
    }
    printf("Passed test 3.\n");
    while (rootTextLine != NULL) {
        textLine_t *tempLine = getLeftmostTextLine(rootTextLine);
        deleteTextLine(tempLine);
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 4.\n");
                return;
            }
        }
    }
    printf("Passed test 4.\n");
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        } else {
            textLine_t *tempLine2 = createEmptyTextLine();
            insertTextLineLeft(rootTextLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 5.\n");
                return;
            }
        }
        tempCount += 1;
    }
    printf("Passed test 5.\n");
    while (rootTextLine != NULL) {
        deleteTextLine(rootTextLine);
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 6.\n");
                return;
            }
        }
    }
    printf("Passed test 6.\n");
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        } else {
            textLine_t *tempLine2 = createEmptyTextLine();
            insertTextLineRight(rootTextLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 7.\n");
                return;
            }
        }
        tempCount += 1;
    }
    printf("Passed test 7.\n");
    while (rootTextLine != NULL) {
        deleteTextLine(rootTextLine);
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 8.\n");
                return;
            }
        }
    }
    printf("Passed test 8.\n");
}

int64_t getTextLineRowCount(textLine_t *line) {
    return line->textAllocation.length / viewPortWidth + 1;
}

int64_t getCursorPosY() {
    textLine_t *tempLine = topTextLine;
    int64_t tempPosY = -topTextLineRow;
    while (tempLine != cursorTextLine) {
        tempPosY += getTextLineRowCount(tempLine);
        tempLine = getNextTextLine(tempLine);
    }
    return tempPosY + cursorTextLineRow;
}

int8_t getCursorCharacter() {
    int64_t index = cursorTextLineRow * viewPortWidth + cursorTextLineColumn;
    if (index < cursorTextLine->textAllocation.length) {
        return cursorTextLine->textAllocation.text[index];
    } else {
        return ' ';
    }
}

void eraseCursor() {
    attron(COLOR_PAIR(primaryColorPair));
    mvprintw(getCursorPosY(), cursorTextLineColumn, "%c", (char)getCursorCharacter());
    attroff(COLOR_PAIR(primaryColorPair));
}

void displayCursor() {
    attron(COLOR_PAIR(secondaryColorPair));
    mvprintw(getCursorPosY(), cursorTextLineColumn, "%c", (char)getCursorCharacter());
    attroff(COLOR_PAIR(secondaryColorPair));
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
    int64_t tempSize = tempLength + 1;
    int8_t tempBuffer[tempSize];
    tempBuffer[tempSize - 1] = 0;
    int64_t tempStartIndex = tempStartRow * viewPortWidth;
    int64_t tempEndIndex = tempEndRow * viewPortWidth;
    if (tempEndIndex > line->textAllocation.length) {
        tempEndIndex = line->textAllocation.length;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    copyData(tempBuffer, line->textAllocation.text + tempStartIndex, tempAmount);
    int64_t index = tempAmount;
    while (index < tempLength) {
        tempBuffer[index] = ' ';
        index += 1;
    }
    attron(COLOR_PAIR(primaryColorPair));
    mvprintw(posY + tempStartRow, 0, "%s", tempBuffer);
    attroff(COLOR_PAIR(primaryColorPair));
    return posY + tempEndRow;
}

void displayAllTextLines() {
    textLine_t *tempLine = topTextLine;
    int64_t tempPosY = -topTextLineRow;
    while (tempPosY < viewPortHeight && tempLine != NULL) {
        tempPosY = displayTextLine(tempPosY, tempLine);
        tempLine = getNextTextLine(tempLine);
    }
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
    while (index <= activityModeTextLength) {
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
    sprintf((char *)tempMessage, "Line %lld", getTextLineNumber(cursorTextLine));
    lineNumberTextLength = (int32_t)strlen((char *)tempMessage);
    mvprintw(windowHeight - 1, windowWidth - lineNumberTextLength, "%s", (char *)tempMessage);
    attroff(COLOR_PAIR(secondaryColorPair));
}

void displayStatusBar() {
    eraseStatusBar();
    displayActivityMode();
    displayLineNumber();
}

void redrawEverything() {
    displayAllTextLines();
    displayStatusBar();
    displayCursor();
}

void moveCursorLeft() {
    textLine_t *tempNextTextLine = cursorTextLine;
    int64_t tempNextColumn = cursorTextLineColumn;
    int64_t tempNextRow = cursorTextLineRow;
    if (tempNextRow <= 0 && tempNextColumn <= 0) {
        textLine_t *tempLine = getPreviousTextLine(tempNextTextLine);
        if (tempLine == NULL) {
            return;
        }
        eraseLineNumber();
        tempNextTextLine = tempLine;
        int64_t tempLength = tempNextTextLine->textAllocation.length;
        tempNextColumn = tempLength % viewPortWidth;
        tempNextRow = tempLength / viewPortWidth;
    } else if (tempNextColumn <= 0) {
        tempNextColumn = viewPortWidth - 1;
        tempNextRow -= 1;
    } else {
        tempNextColumn -= 1;
    }
    if (tempNextTextLine != cursorTextLine || tempNextColumn != cursorTextLineColumn || tempNextRow != cursorTextLineRow) {
        eraseCursor();
        if (tempNextTextLine != cursorTextLine) {
            eraseLineNumber();
            cursorTextLine = tempNextTextLine;
            displayLineNumber();
        }
        cursorTextLineColumn = tempNextColumn;
        cursorTextLineRow = tempNextRow;
        cursorTextLineSnapColumn = cursorTextLineColumn;
        displayCursor();
    }
}

void moveCursorRight() {
    textLine_t *tempNextTextLine = cursorTextLine;
    int64_t tempNextColumn = cursorTextLineColumn;
    int64_t tempNextRow = cursorTextLineRow;
    int64_t tempLength = tempNextTextLine->textAllocation.length;
    if (tempNextRow * viewPortWidth + tempNextColumn >= tempLength) {
        textLine_t *tempLine = getNextTextLine(tempNextTextLine);
        if (tempLine == NULL) {
            return;
        }
        eraseLineNumber();
        tempNextTextLine = tempLine;
        tempNextColumn = 0;
        tempNextRow = 0;
    } else if (tempNextColumn >= viewPortWidth - 1) {
        tempNextColumn = 0;
        tempNextRow += 1;
    } else {
        tempNextColumn += 1;
    }
    if (tempNextTextLine != cursorTextLine || tempNextColumn != cursorTextLineColumn || tempNextRow != cursorTextLineRow) {
        eraseCursor();
        if (tempNextTextLine != cursorTextLine) {
            eraseLineNumber();
            cursorTextLine = tempNextTextLine;
            displayLineNumber();
        }
        cursorTextLineColumn = tempNextColumn;
        cursorTextLineRow = tempNextRow;
        cursorTextLineSnapColumn = cursorTextLineColumn;
        displayCursor();
    }
}

void moveCursorUp() {
    textLine_t *tempNextTextLine = cursorTextLine;
    int64_t tempNextColumn = cursorTextLineColumn;
    int64_t tempNextRow = cursorTextLineRow;
    if (tempNextColumn < cursorTextLineSnapColumn) {
        tempNextColumn = cursorTextLineSnapColumn;
    }
    if (tempNextRow <= 0) {
        textLine_t *tempLine = getPreviousTextLine(tempNextTextLine);
        if (tempLine == NULL) {
            return;
        }
        eraseLineNumber();
        tempNextTextLine = tempLine;
        int64_t tempLength = tempNextTextLine->textAllocation.length;
        int64_t tempColumn = tempLength % viewPortWidth;
        if (tempNextColumn > tempColumn) {
            tempNextColumn = tempColumn;
        }
        tempNextRow = tempLength / viewPortWidth;
    } else {
        tempNextRow -= 1;
    }
    if (tempNextTextLine != cursorTextLine || tempNextColumn != cursorTextLineColumn || tempNextRow != cursorTextLineRow) {
        eraseCursor();
        if (tempNextTextLine != cursorTextLine) {
            eraseLineNumber();
            cursorTextLine = tempNextTextLine;
            displayLineNumber();
        }
        cursorTextLineColumn = tempNextColumn;
        cursorTextLineRow = tempNextRow;
        displayCursor();
    }
}

void moveCursorDown() {
    textLine_t *tempNextTextLine = cursorTextLine;
    int64_t tempNextColumn = cursorTextLineColumn;
    int64_t tempNextRow = cursorTextLineRow;
    if (tempNextColumn < cursorTextLineSnapColumn) {
        tempNextColumn = cursorTextLineSnapColumn;
    }
    int64_t tempRowCount = getTextLineRowCount(tempNextTextLine);
    if (tempNextRow >= tempRowCount - 1) {
        textLine_t *tempLine = getNextTextLine(tempNextTextLine);
        if (tempLine == NULL) {
            return;
        }
        eraseLineNumber();
        tempNextTextLine = tempLine;
        int64_t tempLength = tempNextTextLine->textAllocation.length;
        int64_t tempRowCount2 = getTextLineRowCount(tempNextTextLine);
        if (tempRowCount2 <= 1) {
            int64_t tempColumn = tempLength % viewPortWidth;
            if (tempNextColumn > tempColumn) {
                tempNextColumn = tempColumn;
            }
        }
        tempNextRow = 0;
    } else {
        tempNextRow += 1;
        if (tempNextRow >= tempRowCount - 1) {
            int64_t tempLength = tempNextTextLine->textAllocation.length;
            int64_t tempColumn = tempLength % viewPortWidth;
            if (tempNextColumn > tempColumn) {
                tempNextColumn = tempColumn;
            }
        }
    }
    if (tempNextTextLine != cursorTextLine || tempNextColumn != cursorTextLineColumn || tempNextRow != cursorTextLineRow) {
        eraseCursor();
        if (tempNextTextLine != cursorTextLine) {
            eraseLineNumber();
            cursorTextLine = tempNextTextLine;
            displayLineNumber();
        }
        cursorTextLineColumn = tempNextColumn;
        cursorTextLineRow = tempNextRow;
        displayCursor();
    }
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
    cursorTextLine = topTextLine;
    cursorTextLineRow = 0;
    cursorTextLineColumn = 0;
    cursorTextLineSnapColumn = 0;
    
    redrawEverything();
}

int main(int argc, const char *argv[]) {
    
    if (SHOULD_RUN_TESTS) {
        runTests();
        return 0;
    }
    
    if (argc != 2) {
        printf("Usage: breadtext [file path]\n");
        return 0;
    }
    
    int8_t *tempText = malloc(50000);
    realpath(argv[1], (char *)tempText);
    filePath = malloc(strlen((char *)tempText) + 1);
    strcpy((char *)filePath, (char *)tempText);
    free(tempText);
    
    FILE *tempFile = fopen((char *)filePath, "r");
    if (tempFile == NULL) {
        rootTextLine = createEmptyTextLine();
    } else {
        rootTextLine = NULL;
        while (true) {
            int8_t *tempText = NULL;
            size_t tempSize = 0;
            int64_t tempCount = getline((char **)&tempText, &tempSize, tempFile);
            if (tempCount < 0) {
                break;
            }
            if (tempText[tempCount - 1] == '\n') {
                tempCount -= 1;
            }
            textLine_t *tempTextLine = createEmptyTextLine();
            insertTextIntoTextAllocation(&(tempTextLine->textAllocation), 0, tempText, tempCount);
            free(tempText);
            if (rootTextLine == NULL) {
                rootTextLine = tempTextLine;
            } else {
                textLine_t *tempTextLine2 = getRightmostTextLine(rootTextLine);
                insertTextLineRight(tempTextLine2, tempTextLine);
            }
        }
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
        }
        fclose(tempFile);
    }
    topTextLine = getLeftmostTextLine(rootTextLine);
    topTextLineRow = 0;
    cursorTextLine = topTextLine;
    cursorTextLineRow = 0;
    cursorTextLineColumn = 0;
    cursorTextLineSnapColumn = 0;
    
    window = initscr();
    noecho();
    curs_set(0);
    start_color();
    init_pair(BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
    handleResize();
    
    while (true) {
        int32_t tempKey = getch();
        if (tempKey == KEY_RESIZE) {
            handleResize();
        }
        if (tempKey == 'q') {
            break;
        }
        if (tempKey == 'j') {
            moveCursorLeft();
        }
        if (tempKey == 'l') {
            moveCursorRight();
        }
        if (tempKey == 'i') {
            moveCursorUp();
        }
        if (tempKey == 'k') {
            moveCursorDown();
        }
    }
    
    endwin();
    
    return 0;
}
