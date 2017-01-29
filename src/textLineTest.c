
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textLineTest.h"

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

int8_t textLineTreeIsInOrder(textLine_t *line) {
    textLine_t *tempChild;
    tempChild = line->leftChild;
    if (tempChild != NULL) {
        if (tempChild->textAllocation.length > line->textAllocation.length) {
            return false;
        }
        if (!textLineTreeIsInOrder(tempChild)) {
            return false;
        }
    }
    tempChild = line->rightChild;
    if (tempChild != NULL) {
        if (tempChild->textAllocation.length < line->textAllocation.length) {
            return false;
        }
        if (!textLineTreeIsInOrder(tempChild)) {
            return false;
        }
    }
    return true;
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
    printf("%lld\n", (long long)(line->textAllocation.length));
    tempChild = line->rightChild;
    if (tempChild != NULL) {
        printTextLineStructure(tempChild);
    }
}

int8_t runFuzzTest() {
    textLine_t *tempLine = createEmptyTextLine();
    rootTextLine = tempLine;
    int32_t maximumLineCount = 100;
    textLine_t *lineList[maximumLineCount];
    lineList[0] = tempLine;
    int32_t lineCount = 1;
    int32_t tempIterationCount = 0;
    while (tempIterationCount < 1000000) {
        if (tempIterationCount % 20000 == 0) {
            printf("%d (%d)\n", tempIterationCount, lineCount);
        }
        int8_t shouldInsert;
        if (lineCount < 10) {
            shouldInsert = true;
        } else if (lineCount >= maximumLineCount) {
            shouldInsert = false;
        } else {
            shouldInsert = ((rand() % 100) >= 50);
        }
        int64_t tempLineNumber = rand() % lineCount + 1;
        textLine_t *tempSelectedLine = getTextLineByNumber(tempLineNumber);
        if (shouldInsert) {
            textLine_t *tempLine = createEmptyTextLine();
            int32_t index;
            if ((rand() % 100) >= 50) {
                insertTextLineRight(tempSelectedLine, tempLine);
                index = tempLineNumber;
            } else {
                insertTextLineLeft(tempSelectedLine, tempLine);
                index = tempLineNumber - 1;
            }
            int32_t tempIndex = lineCount;
            while (tempIndex > index) {
                lineList[tempIndex] = lineList[tempIndex - 1];
                tempIndex -= 1;
            }
            lineList[index] = tempLine;
            lineCount += 1;
        } else {
            deleteTextLine(tempSelectedLine);
            int32_t index = tempLineNumber - 1;
            int32_t tempIndex = index;
            while (tempIndex < lineCount - 1) {
                lineList[tempIndex] = lineList[tempIndex + 1];
                tempIndex += 1;
            }
            lineCount -= 1;
        }
        textLine_t *tempLine = getLeftmostTextLine(rootTextLine);
        int32_t index = 0;
        while (index < lineCount) {
            textLine_t *tempLine2 = lineList[index];
            if (tempLine != tempLine2) {
                return false;
            }
            tempLine = getNextTextLine(tempLine);
            index += 1;
        }
        tempIterationCount += 1;
    }
    return true;
}

void runTests() {
    srand((unsigned)time(NULL));
    int32_t tempCount;
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
            rootTextLine->textAllocation.length = tempCount;
        } else {
            textLine_t *tempLine = getRightmostTextLine(rootTextLine);
            textLine_t *tempLine2 = createEmptyTextLine();
            tempLine2->textAllocation.length = tempCount;
            insertTextLineRight(tempLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 1.\n");
                return;
            }
            if (!textLineTreeIsInOrder(rootTextLine)) {
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
            if (!textLineTreeIsInOrder(rootTextLine)) {
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
            rootTextLine->textAllocation.length = 100000 - tempCount;
        } else {
            textLine_t *tempLine = getLeftmostTextLine(rootTextLine);
            textLine_t *tempLine2 = createEmptyTextLine();
            tempLine2->textAllocation.length = 100000 - tempCount;
            insertTextLineLeft(tempLine, tempLine2);
        }
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 3.\n");
                return;
            }
            if (!textLineTreeIsInOrder(rootTextLine)) {
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
            if (!textLineTreeIsInOrder(rootTextLine)) {
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
    tempCount = 0;
    while (tempCount < 100) {
        if (rootTextLine == NULL) {
            rootTextLine = createEmptyTextLine();
            rootTextLine->textAllocation.length = tempCount;
        } else {
            textLine_t *tempLine = getRightmostTextLine(rootTextLine);
            textLine_t *tempLine2 = createEmptyTextLine();
            tempLine2->textAllocation.length = tempCount;
            insertTextLineRight(tempLine, tempLine2);
        }
        tempCount += 1;
    }
    while (rootTextLine != NULL) {
        deleteTextLine(rootTextLine);
        if (rootTextLine != NULL) {
            if (!textLineTreeIsBalanced(rootTextLine)) {
                printf("FAILED TEST 9.\n");
                return;
            }
            if (!textLineTreeIsInOrder(rootTextLine)) {
                printf("FAILED TEST 9.\n");
                return;
            }
        }
    }
    printf("Passed test 9.\n");
    int8_t tempResult = runFuzzTest();
    if (tempResult) {
        printf("Passed test 10.\n");
    } else {
        printf("FAILED TEST 10.\n");
        return;
    }
}
