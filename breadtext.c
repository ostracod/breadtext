
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define true 1
#define false 0

#define SHOULD_RUN_TESTS true

typedef struct textLine textLine_t;
struct textLine {
    textLine_t *parent;
    textLine_t *leftChild;
    textLine_t *rightChild;
    int8_t *text;
    int64_t textLength;
    int64_t textAllocationSize;
    int8_t depth;
    int64_t lineCount;
};

textLine_t *rootTextLine = NULL;
WINDOW *window;
int32_t windowWidth = -1;
int32_t windowHeight = -1;

textLine_t *createEmptyTextLine() {
    textLine_t *output = malloc(sizeof(textLine_t));
    output->parent = NULL;
    output->leftChild = NULL;
    output->rightChild = NULL;
    output->text = NULL;
    output->textLength = 0;
    output->textAllocationSize = 0;
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
    if (line->text != NULL) {
        free(line->text);
    }
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

void handleResize() {
    int32_t tempWidth;
    int32_t tempHeight;
    getmaxyx(window, tempHeight, tempWidth);
    if (tempWidth == windowWidth && tempHeight == windowHeight) {
        return;
    }
    windowWidth = tempWidth;
    windowHeight = tempHeight;
    
    // TODO: Redraw everything.
    clear();
    printw("%d %d", windowWidth, windowHeight);
}

int main(int argc, const char *argv[]) {
    
    if (SHOULD_RUN_TESTS) {
        runTests();
        return 0;
    }
    
    window = initscr();
    handleResize();
    
    while (true) {
        int32_t tempKey = getch();
        if (tempKey == KEY_RESIZE) {
            handleResize();
        }
        if (tempKey == 'q') {
            break;
        }
    }
    
    endwin();
    
    return 0;
}
