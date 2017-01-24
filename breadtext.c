
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <wordexp.h>

#define true 1
#define false 0

#define PLATFORM_MAC 1
#define PLATFORM_LINUX 2

#define BLACK_ON_WHITE 1
#define WHITE_ON_BLACK 2

#define COMMAND_MODE 1
#define TEXT_ENTRY_MODE 2
#define HIGHLIGHT_CHARACTER_MODE 3
#define HIGHLIGHT_WORD_MODE 4
#define HIGHLIGHT_LINE_MODE 5
#define TEXT_COMMAND_MODE 6

#define HISTORY_ACTION_INSERT 1
#define HISTORY_ACTION_DELETE 2

#define MAXIMUM_HISTORY_DEPTH 300
#define MAXIMUM_MACRO_LENGTH 100

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

typedef struct textPos {
    textLine_t *line;
    int64_t column;
    int64_t row;
} textPos_t;

typedef struct historyTextPos {
    int64_t lineNumber;
    int64_t index;
} historyTextPos_t;

typedef struct historyAction {
    int8_t type;
    int64_t lineNumber;
    int8_t *text;
    int64_t length;
} historyAction_t;

typedef struct historyFrame {
    historyAction_t *historyActionList;
    int64_t length;
    int64_t allocationSize;
    historyTextPos_t previousCursorTextPos;
    historyTextPos_t nextCursorTextPos;
} historyFrame_t;

textLine_t *rootTextLine = NULL;
textLine_t *topTextLine = NULL;
int64_t topTextLineRow = 0;
textPos_t cursorTextPos;
int64_t cursorSnapColumn = 0;
int8_t activityMode = COMMAND_MODE;
int32_t activityModeTextLength = 0;
int32_t lineNumberTextLength = 0;
int8_t isShowingNotification = false;
int32_t notificationTextLength = 0;
int8_t isHighlighting = false;
textPos_t highlightTextPos;
historyFrame_t historyFrameList[MAXIMUM_HISTORY_DEPTH];
int32_t historyFrameListIndex = 0;
int32_t historyFrameListLength = 0;
int8_t historyFrameIsConsecutive = false;
int8_t isStartOfNonconsecutiveEscapeSequence = false;
historyAction_t firstNonconsecutiveEscapeSequenceAction;
historyTextPos_t nonconsecutiveEscapeSequencePreviousCursorTextPos;
int32_t macroKeyList[MAXIMUM_MACRO_LENGTH];
int32_t macroKeyListLength = 0;
int8_t isRecordingMacro = false;
int8_t indentationWidth = 4;
int8_t shouldUseHardTabs = false;
int8_t textCommandBuffer[1000];
int8_t searchTerm[1000];
int64_t searchTermLength;
WINDOW *window;
int32_t windowWidth = -1;
int32_t windowHeight = -1;
int32_t viewPortWidth = -1;
int32_t viewPortHeight = -1;
int32_t lastKey = 0;
int8_t primaryColorPair = BLACK_ON_WHITE;
int8_t secondaryColorPair = WHITE_ON_BLACK;
int8_t *filePath;
int8_t textBufferIsDirty = false;
int8_t *clipboardFilePath;
int8_t *rcFilePath;
int8_t applicationPlatform;

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

int8_t equalData(int8_t *source1, int8_t *source2, int64_t amount) {
    int64_t index = 0;
    while (index < amount) {
        if (source1[index] != source2[index]) {
            return false;
        }
        index += 1;
    }
    return true;
}

int8_t isWhitespace(int8_t character) {
    return (character == ' ' || character == '\n' || character == '\t' || character == '\r');
}

int8_t *findWhitespace(int8_t *text) {
    while (true) {
        int8_t tempCharacter = *text;
        if (tempCharacter == 0) {
            break;
        }
        if (isWhitespace(tempCharacter)) {
            break;
        }
        text += 1;
    }
    return text;
}

int8_t *skipWhitespace(int8_t *text) {
    while (true) {
        int8_t tempCharacter = *text;
        if (tempCharacter == 0) {
            break;
        }
        if (!isWhitespace(tempCharacter)) {
            break;
        }
        text += 1;
    }
    return text;
}

int64_t removeBadCharacters(int8_t *text, int8_t *containsNewline) {
    *containsNewline = false;
    int64_t tempIndex1 = 0;
    int64_t tempIndex2 = 0;
    while (true) {
        int8_t tempCharacter = text[tempIndex1];
        if (tempCharacter == 0) {
            break;
        }
        if (tempCharacter == '\n') {
            *containsNewline = true;
        }
        if ((tempCharacter >= 32 && tempCharacter <= 126) || tempCharacter == '\t') {
            text[tempIndex2] = tempCharacter;
            tempIndex2 += 1;
        }
        tempIndex1 += 1;
    }
    return tempIndex2;
}

void convertTabsToSpaces(int8_t *text) {
    int64_t index = 0;
    while (true) {
        int8_t tempCharacter = text[index];
        if (tempCharacter == 0) {
            break;
        }
        if (tempCharacter == '\t') {
            text[index] = ' ';
        }
        index += 1;
    }
}

int8_t *mallocRealpath(int8_t *path) {
    wordexp_t expResult;
    wordexp((char *)path, &expResult, 0);
    int8_t tempText[50000];
    realpath(expResult.we_wordv[0], (char *)tempText);
    wordfree(&expResult);
    int8_t *output = malloc(strlen((char *)tempText) + 1);
    strcpy((char *)output, (char *)tempText);
    return output;
}

void systemCopyClipboardFile() {
    if (applicationPlatform == PLATFORM_MAC) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "cat \"%s\" | pbcopy", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
    if (applicationPlatform == PLATFORM_LINUX) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
}

void systemPasteClipboardFile() {
    if (applicationPlatform == PLATFORM_MAC) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "pbpaste > \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
    if (applicationPlatform == PLATFORM_LINUX) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard -o > \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
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
    allocation->allocationSize = size;
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
    textLine_t *tempChild1 = line->leftChild;
    textLine_t *tempChild2 = line->rightChild;
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

// Returns true iff line1 is after line2.
int8_t textLineIsAfterTextLine(textLine_t *line1, textLine_t *line2) {
    int64_t tempLineNumber1 = getTextLineNumber(line1);
    int64_t tempLineNumber2 = getTextLineNumber(line2);
    return (tempLineNumber1 > tempLineNumber2);
}

textLine_t *getTextLineByNumber(int64_t number) {
    int64_t tempLineCount = 0;
    textLine_t *tempLine = rootTextLine;
    while (true) {
        if (tempLine == NULL) {
            return NULL;
        }
        textLine_t *tempChild = tempLine->leftChild;
        int64_t tempLineCount2;
        if (tempChild == NULL) {
            tempLineCount2 = 0;
        } else {
            tempLineCount2 = tempChild->lineCount;
        }
        int64_t tempLineNumber = tempLineCount + tempLineCount2 + 1;
        if (number > tempLineNumber) {
            tempLine = tempLine->rightChild;
            tempLineCount = tempLineNumber;
        } else if (number < tempLineNumber) {
            tempLine = tempChild;
        } else {
            return tempLine;
        }
    }
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
    printf("%lld\n", line->textAllocation.length);
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
}

int8_t equalTextPos(textPos_t *pos1, textPos_t *pos2) {
    return (pos1->line == pos2->line && pos1->row == pos2->row && pos1->column == pos2->column);
}

int64_t getTextPosIndex(textPos_t *pos) {
    return pos->row * viewPortWidth + pos->column;
}

void setTextPosIndex(textPos_t *pos, int64_t index) {
    pos->row = index / viewPortWidth;
    pos->column = index % viewPortWidth;
}

int64_t getTextLineRowCount(textLine_t *line) {
    return line->textAllocation.length / viewPortWidth + 1;
}

textPos_t convertHistoryTextPosToTextPos(historyTextPos_t *pos) {
    textPos_t output;
    output.line = getTextLineByNumber(pos->lineNumber);
    setTextPosIndex(&output, pos->index);
    return output;
}

historyTextPos_t convertTextPosToHistoryTextPos(textPos_t *pos) {
    historyTextPos_t output;
    output.lineNumber = getTextLineNumber(pos->line);
    output.index = getTextPosIndex(pos);
    return output;
}

void handleTextLineDeleted(textLine_t *lineToBeDeleted) {
    if (lineToBeDeleted != topTextLine) {
        return;
    }
    topTextLine = getNextTextLine(lineToBeDeleted);
    if (topTextLine == NULL) {
        topTextLine = getPreviousTextLine(lineToBeDeleted);
    }
    topTextLineRow = 0;
}

void eraseActivityModeOrNotification();
void setActivityMode(int8_t mode);
int8_t scrollCursorOntoScreen();
void displayNotification(int8_t *message);
void displayStatusBar();
void redrawEverything();

void performHistoryAction(historyAction_t *action) {
    if (action->type == HISTORY_ACTION_INSERT) {
        textLine_t *tempLine = createEmptyTextLine();
        insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, action->text, action->length);
        if (rootTextLine == NULL) {
            rootTextLine = tempLine;
            topTextLine = tempLine;
            topTextLineRow = 0;
        } else if (action->lineNumber == 1) {
            textLine_t *tempLine2 = getTextLineByNumber(1);
            insertTextLineLeft(tempLine2, tempLine);
        } else {
            textLine_t *tempLine2 = getTextLineByNumber(action->lineNumber - 1);
            insertTextLineRight(tempLine2, tempLine);
        }
    }
    if (action->type == HISTORY_ACTION_DELETE) {
        textLine_t *tempLine = getTextLineByNumber(action->lineNumber);
        handleTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
    }
}

void undoHistoryAction(historyAction_t *action) {
    if (action->type == HISTORY_ACTION_INSERT) {
        textLine_t *tempLine = getTextLineByNumber(action->lineNumber);
        handleTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
    }
    if (action->type == HISTORY_ACTION_DELETE) {
        textLine_t *tempLine = createEmptyTextLine();
        insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, action->text, action->length);
        if (rootTextLine == NULL) {
            rootTextLine = tempLine;
            topTextLine = tempLine;
            topTextLineRow = 0;
        } else if (action->lineNumber == 1) {
            textLine_t *tempLine2 = getTextLineByNumber(1);
            insertTextLineLeft(tempLine2, tempLine);
        } else {
            textLine_t *tempLine2 = getTextLineByNumber(action->lineNumber - 1);
            insertTextLineRight(tempLine2, tempLine);
        }
    }
}

void cleanUpHistoryAction(historyAction_t *action) {
    if (action->text != NULL) {
        free(action->text);
    }
}

historyFrame_t createEmptyHistoryFrame() {
    historyFrame_t output;
    output.historyActionList = malloc(0);
    output.length = 0;
    output.allocationSize = 0;
    output.previousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
    return output;
}

void addHistoryActionToHistoryFrame(historyFrame_t *frame, historyAction_t *action) {
    int64_t tempOldSize = frame->length * sizeof(historyAction_t);
    int64_t index = frame->length;
    frame->length += 1;
    int64_t tempSize = frame->length * sizeof(historyAction_t);
    if (tempSize > frame->allocationSize) {
        frame->allocationSize = tempSize * 2;
        historyAction_t *tempList = malloc(frame->allocationSize);
        copyData((int8_t *)tempList, (int8_t *)(frame->historyActionList), tempOldSize);
        free(frame->historyActionList);
        frame->historyActionList = tempList;
    }
    *(frame->historyActionList + index) = *action;
}

void performHistoryFrame(historyFrame_t *frame) {
    int64_t index = 0;
    while (index < frame->length) {
        historyAction_t *tempAction = frame->historyActionList + index;
        performHistoryAction(tempAction);
        index += 1;
    }
    cursorTextPos = convertHistoryTextPosToTextPos(&(frame->nextCursorTextPos));
    setActivityMode(COMMAND_MODE);
    int8_t tempResult = scrollCursorOntoScreen();
    if (tempResult) {
        displayStatusBar();
    } else {
        redrawEverything();
    }
    textBufferIsDirty = true;
}

void undoHistoryFrame(historyFrame_t *frame) {
    int64_t index = frame->length - 1;
    while (index >= 0) {
        historyAction_t *tempAction = frame->historyActionList + index;
        undoHistoryAction(tempAction);
        index -= 1;
    }
    cursorTextPos = convertHistoryTextPosToTextPos(&(frame->previousCursorTextPos));
    setActivityMode(COMMAND_MODE);
    int8_t tempResult = scrollCursorOntoScreen();
    if (tempResult) {
        displayStatusBar();
    } else {
        redrawEverything();
    }
    textBufferIsDirty = true;
}

void cleanUpHistoryFrame(historyFrame_t *frame) {
    int64_t index = 0;
    while (index < frame->length) {
        historyAction_t *tempAction = frame->historyActionList + index;
        cleanUpHistoryAction(tempAction);
        index += 1;
    }
    if (frame->historyActionList != NULL) {
        free(frame->historyActionList);
    }
}

void addHistoryFrame() {
    int32_t index = 0;
    while (index < historyFrameListIndex) {
        historyFrame_t *tempFrame = historyFrameList + index;
        cleanUpHistoryFrame(tempFrame);
        index += 1;
    }
    if (historyFrameListIndex > 1) {
        int32_t index = 1;
        while (historyFrameListIndex < historyFrameListLength) {
            historyFrameList[index] = historyFrameList[historyFrameListIndex];
            index += 1;
            historyFrameListIndex += 1;
        }
        historyFrameListLength = index;
    } else if (historyFrameListIndex == 0) {
        int32_t index = historyFrameListLength - 1;
        if (index + 1 >= MAXIMUM_HISTORY_DEPTH) {
            index = MAXIMUM_HISTORY_DEPTH - 2;
            historyFrameListLength = MAXIMUM_HISTORY_DEPTH;
        } else {
            historyFrameListLength += 1;
        }
        while (index >= 0) {
            historyFrameList[index + 1] = historyFrameList[index];
            index -= 1;
        }
    }
    historyFrameListIndex = 0;
    historyFrameList[historyFrameListIndex] = createEmptyHistoryFrame();
}

historyAction_t createHistoryActionFromTextLine(textLine_t *line, int8_t actionType) {
    historyAction_t output;
    output.type = actionType;
    output.lineNumber = getTextLineNumber(line);
    int64_t tempLength = line->textAllocation.length;
    output.text = malloc(tempLength);
    copyData(output.text, line->textAllocation.text, tempLength);
    output.length = tempLength;
    return output;
}

void recordTextLine(textLine_t *line, int8_t actionType) {
    historyAction_t tempAction = createHistoryActionFromTextLine(line, actionType);
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    addHistoryActionToHistoryFrame(tempFrame, &tempAction);
}

void recordTextLineInserted(textLine_t *line) {
    recordTextLine(line, HISTORY_ACTION_INSERT);
}

void recordTextLineDeleted(textLine_t *line) {
    recordTextLine(line, HISTORY_ACTION_DELETE);
}

void updateHistoryFrameInsertAction(textLine_t *line) {
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    int64_t index = 0;
    while (index < tempFrame->length) {
        historyAction_t *tempAction = tempFrame->historyActionList + index;
        if (tempAction->type == HISTORY_ACTION_INSERT) {
            int64_t tempLength = line->textAllocation.length;
            if (tempAction->text != NULL) {
                free(tempAction->text);
            }
            tempAction->text = malloc(tempLength);
            copyData(tempAction->text, line->textAllocation.text, tempLength);
            tempAction->length = tempLength;
            break;
        }
        index += 1;
    }
}

void finishCurrentHistoryFrame() {
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    tempFrame->nextCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
}

void undoLastAction() {
    if (historyFrameListIndex >= historyFrameListLength) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"At oldest state.");
        return;
    }
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    undoHistoryFrame(tempFrame);
    historyFrameListIndex += 1;
}

void redoLastAction() {
    if (historyFrameListIndex <= 0) {
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"At newest state.");
        return;
    }
    historyFrameListIndex -= 1;
    historyFrame_t *tempFrame = historyFrameList + historyFrameListIndex;
    performHistoryFrame(tempFrame);
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
    attron(COLOR_PAIR(primaryColorPair));
    mvaddch(getCursorPosY(), cursorTextPos.column, (char)getCursorCharacter());
    attroff(COLOR_PAIR(primaryColorPair));
}

void displayCursor() {
    if (isHighlighting) {
        return;
    }
    refresh();
    attron(COLOR_PAIR(secondaryColorPair));
    mvaddch(getCursorPosY(), cursorTextPos.column, (char)getCursorCharacter());
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

int8_t textPosIsAfterTextPos(textPos_t *textPos1, textPos_t *textPos2) {
    if (textPos1->line != textPos2->line) {
        return textLineIsAfterTextLine(textPos1->line, textPos2->line);
    }
    if (textPos1->row > textPos2->row) {
        return true;
    }
    if (textPos1->row < textPos2->row) {
        return false;
    }
    return (textPos1->column > textPos2->column);
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
    int64_t tempEndColumn = tempAmount % viewPortWidth;
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
            tempEndTextPos.row = tempFirstTextPos->row;
            tempEndTextPos.column = tempFirstTextPos->column;
        } else if (textLineIsAfterTextLine(tempFirstTextPos->line, line)) {
            tempEndTextPos.row = tempEndRow - 1;
            tempEndTextPos.column = tempEndColumn;
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
            tempEndTextPos.row = tempLastTextPos->row;
            tempEndTextPos.column = tempLastTextPos->column;
            if (tempEndTextPos.row < tempEndRow - 1 || tempEndTextPos.column < tempEndColumn) {
                tempEndTextPos.column += 1;
                if (tempEndTextPos.column >= viewPortWidth) {
                    tempEndTextPos.column = 0;
                    tempEndTextPos.row += 1;
                }
            }
        } else if (textLineIsAfterTextLine(tempLastTextPos->line, line)) {
            tempEndTextPos.row = tempEndRow - 1;
            tempEndTextPos.column = tempEndColumn;
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
        tempEndTextPos.row = tempEndRow - 1;
        tempEndTextPos.column = tempEndColumn;
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
            tempTextPos.row = tempEndRow - 1;
            tempTextPos.column = tempEndColumn;
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
    if (activityMode == HIGHLIGHT_WORD_MODE) {
        int8_t tempMessage[] = "Word Highlight Mode";
        mvprintw(windowHeight - 1, 0, "%s", (char *)tempMessage);
        activityModeTextLength = (int32_t)strlen((char *)tempMessage);
    }
    if (activityMode == TEXT_COMMAND_MODE) {
        activityModeTextLength = 0;
    }
    attroff(COLOR_PAIR(secondaryColorPair));
}

int8_t isWordCharacter(int8_t tempCharacter) {
    return ((tempCharacter >= 'a' && tempCharacter <= 'z')
         || (tempCharacter >= 'A' && tempCharacter <= 'Z')
         || (tempCharacter >= '0' && tempCharacter <= '9')
         || tempCharacter == '_');
}

void displayStatusBar();

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
    int8_t tempOldMode = activityMode;
    activityMode = mode;
    if (mode == HIGHLIGHT_CHARACTER_MODE) {
        highlightTextPos.line = cursorTextPos.line;
        highlightTextPos.column = cursorTextPos.column;
        highlightTextPos.row = cursorTextPos.row;
    }
    if (mode == HIGHLIGHT_LINE_MODE) {
        highlightTextPos.line = cursorTextPos.line;
        int64_t tempLength = cursorTextPos.line->textAllocation.length;
        setTextPosIndex(&highlightTextPos, tempLength);
        cursorTextPos.row = 0;
        cursorTextPos.column = 0;
        scrollCursorOntoScreen();
    }
    if (mode == HIGHLIGHT_WORD_MODE) {
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
    int8_t tempOldIsHighlighting = isHighlighting;
    isHighlighting = (mode == HIGHLIGHT_CHARACTER_MODE || mode == HIGHLIGHT_WORD_MODE || mode == HIGHLIGHT_LINE_MODE);
    if (tempOldIsHighlighting || mode == HIGHLIGHT_LINE_MODE) {
        displayAllTextLines();
    } else if (mode == HIGHLIGHT_CHARACTER_MODE || mode == HIGHLIGHT_WORD_MODE) {
        displayTextLine(getTextLinePosY(cursorTextPos.line), cursorTextPos.line);
    }
    if (mode == TEXT_COMMAND_MODE) {
        textCommandBuffer[0] = 0;
        displayStatusBar();
    } else if (tempOldMode == TEXT_COMMAND_MODE) {
        displayStatusBar();
    } else {
        displayActivityMode();
    }
    historyFrameIsConsecutive = false;
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
    sprintf((char *)tempMessage, "Line %lld", getTextLineNumber(cursorTextPos.line));
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

void redrawEverything() {
    displayAllTextLines();
    displayStatusBar();
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

int32_t getTextLineIndentationLevel(textLine_t *line) {
    int32_t output = 0;
    int8_t tempSpaceCount = 0;
    int64_t tempLength = line->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = line->textAllocation.text[index];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                output += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            output += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        index += 1;
    }
    return output;
}

int64_t getTextLineIndentationEndIndex(textLine_t *line) {
    int64_t tempLength = line->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = line->textAllocation.text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    return index;
}

void decreaseTextLineIndentationLevelHelper(textLine_t *line) {
    int32_t tempOldLevel = getTextLineIndentationLevel(line);
    int64_t tempStartIndex = 0;
    int64_t tempEndIndex = getTextLineIndentationEndIndex(line);
    int32_t tempLevel = 0;
    int8_t tempSpaceCount = 0;
    while (tempStartIndex < tempEndIndex) {
        if (tempLevel >= tempOldLevel - 1) {
            break;
        }
        int8_t tempCharacter = line->textAllocation.text[tempStartIndex];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                tempLevel += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            tempLevel += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        tempStartIndex += 1;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    if (tempAmount > 0) {
        recordTextLineDeleted(line);
        removeTextFromTextAllocation(&(line->textAllocation), tempStartIndex, tempAmount);
        recordTextLineInserted(line);
    }
    int64_t tempOffset = -tempAmount;
    if (line == cursorTextPos.line) {
        int64_t index = getTextPosIndex(&cursorTextPos);
        index += tempOffset;
        if (index < 0) {
            index = 0;
        }
        setTextPosIndex(&cursorTextPos, index);
    }
    if (isHighlighting) {
        if (line == highlightTextPos.line) {
            int64_t index = getTextPosIndex(&highlightTextPos);
            index += tempOffset;
            if (index < 0) {
                index = 0;
            }
            setTextPosIndex(&highlightTextPos, index);
        }
    }
}

void increaseTextLineIndentationLevelHelper(textLine_t *line) {
    int32_t tempOldLevel = getTextLineIndentationLevel(line);
    int64_t tempStartIndex = 0;
    int64_t tempEndIndex = getTextLineIndentationEndIndex(line);
    int32_t tempLevel = 0;
    int8_t tempSpaceCount = 0;
    while (tempStartIndex < tempEndIndex) {
        if (tempLevel >= tempOldLevel) {
            break;
        }
        int8_t tempCharacter = line->textAllocation.text[tempStartIndex];
        if (tempCharacter == ' ') {
            tempSpaceCount += 1;
            if (tempSpaceCount >= indentationWidth) {
                tempLevel += 1;
                tempSpaceCount = 0;
            }
        } else if (tempCharacter == '\t') {
            tempLevel += 1;
            tempSpaceCount = 0;
        } else {
            break;
        }
        tempStartIndex += 1;
    }
    int64_t tempAmount = tempEndIndex - tempStartIndex;
    int8_t tempIndentation[100];
    int8_t tempIndentationLength;
    if (shouldUseHardTabs) {
        tempIndentation[0] = '\t';
        tempIndentationLength = 1;
    } else {
        int8_t index = 0;
        while (index < indentationWidth) {
            tempIndentation[index] = ' ' ;
            index += 1;
        }
        tempIndentationLength = indentationWidth;
    }
    recordTextLineDeleted(line);
    if (tempAmount > 0) {
        removeTextFromTextAllocation(&(line->textAllocation), tempStartIndex, tempAmount);
    }
    insertTextIntoTextAllocation(&(line->textAllocation), tempStartIndex, tempIndentation, tempIndentationLength);
    recordTextLineInserted(line);
    int64_t tempOffset = tempIndentationLength - tempAmount;
    if (line == cursorTextPos.line) {
        int64_t index = getTextPosIndex(&cursorTextPos);
        index += tempOffset;
        if (index < 0) {
            index = 0;
        }
        setTextPosIndex(&cursorTextPos, index);
    }
    if (isHighlighting) {
        if (line == highlightTextPos.line) {
            int64_t index = getTextPosIndex(&highlightTextPos);
            index += tempOffset;
            if (index < 0) {
                index = 0;
            }
            setTextPosIndex(&highlightTextPos, index);
        }
    }
}

void moveCursor(textPos_t *pos) {
    historyFrameIsConsecutive = false;
    textPos_t tempPreviousTextPos = cursorTextPos;
    if (!equalTextPos(pos, &tempPreviousTextPos)) {
        cursorTextPos = *pos;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (isHighlighting) {
                textLine_t *tempLine = tempPreviousTextPos.line;
                displayTextLine(getTextLinePosY(tempLine), tempLine);
                while (textLineIsAfterTextLine(tempLine, cursorTextPos.line)) {
                    tempLine = getPreviousTextLine(tempLine);
                    displayTextLine(getTextLinePosY(tempLine), tempLine);
                }
                while (textLineIsAfterTextLine(cursorTextPos.line, tempLine)) {
                    tempLine = getNextTextLine(tempLine);
                    displayTextLine(getTextLinePosY(tempLine), tempLine);
                }
                if (pos->line != tempPreviousTextPos.line) {
                    eraseLineNumber();
                    displayLineNumber();
                }
            } else {
                cursorTextPos = tempPreviousTextPos;
                eraseCursor();
                if (pos->line != cursorTextPos.line) {
                    eraseLineNumber();
                    cursorTextPos.line = pos->line;
                    displayLineNumber();
                }
                cursorTextPos.column = pos->column;
                cursorTextPos.row = pos->row;
                displayCursor();
            }
        }
    }
}

void moveCursorLeft(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        if (tempNextTextPos.row <= 0 && tempNextTextPos.column <= 0) {
            textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            setTextPosIndex(&tempNextTextPos, tempLength);
        } else if (tempNextTextPos.column <= 0) {
            tempNextTextPos.column = viewPortWidth - 1;
            tempNextTextPos.row -= 1;
        } else {
            tempNextTextPos.column -= 1;
        }
        tempCount += 1;
    }
    cursorSnapColumn = tempNextTextPos.column;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorRight(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        int64_t tempLength = tempNextTextPos.line->textAllocation.length;
        if (tempNextTextPos.row * viewPortWidth + tempNextTextPos.column >= tempLength) {
            textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            tempNextTextPos.line = tempLine;
            tempNextTextPos.column = 0;
            tempNextTextPos.row = 0;
        } else if (tempNextTextPos.column >= viewPortWidth - 1) {
            tempNextTextPos.column = 0;
            tempNextTextPos.row += 1;
        } else {
            tempNextTextPos.column += 1;
        }
        tempCount += 1;
    }
    cursorSnapColumn = tempNextTextPos.column;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorUp(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        if (tempNextTextPos.row <= 0) {
            textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            int64_t tempColumn = tempLength % viewPortWidth;
            if (tempNextTextPos.column > tempColumn) {
                tempNextTextPos.column = tempColumn;
            }
            tempNextTextPos.row = tempLength / viewPortWidth;
        } else {
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.row -= 1;
        }
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorDown(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        int64_t tempRowCount = getTextLineRowCount(tempNextTextPos.line);
        if (tempNextTextPos.row >= tempRowCount - 1) {
            textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
            if (tempLine == NULL) {
                break;
            }
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.line = tempLine;
            int64_t tempLength = tempNextTextPos.line->textAllocation.length;
            int64_t tempRowCount2 = getTextLineRowCount(tempNextTextPos.line);
            if (tempRowCount2 <= 1) {
                int64_t tempColumn = tempLength % viewPortWidth;
                if (tempNextTextPos.column > tempColumn) {
                    tempNextTextPos.column = tempColumn;
                }
            }
            tempNextTextPos.row = 0;
        } else {
            if (tempNextTextPos.column < cursorSnapColumn) {
                tempNextTextPos.column = cursorSnapColumn;
            }
            tempNextTextPos.row += 1;
            if (tempNextTextPos.row >= tempRowCount - 1) {
                int64_t tempLength = tempNextTextPos.line->textAllocation.length;
                int64_t tempColumn = tempLength % viewPortWidth;
                if (tempNextTextPos.column > tempColumn) {
                    tempNextTextPos.column = tempColumn;
                }
            }
        }
        tempCount += 1;
    }
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void adjustLineSelectionBoundaries(textPos_t *nextCursorPos) {
    if (textLineIsAfterTextLine(nextCursorPos->line, highlightTextPos.line)) {
        highlightTextPos.row = 0;
        highlightTextPos.column = 0;
        int64_t tempLength = nextCursorPos->line->textAllocation.length;
        setTextPosIndex(nextCursorPos, tempLength);
    } else {
        nextCursorPos->row = 0;
        nextCursorPos->column = 0;
        int64_t tempLength = highlightTextPos.line->textAllocation.length;
        setTextPosIndex(&highlightTextPos, tempLength);
    }
}

void moveLineSelectionUp(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        textLine_t *tempLine = getPreviousTextLine(tempNextTextPos.line);
        if (tempLine == NULL) {
            break;
        }
        tempNextTextPos.line = tempLine;
        tempCount += 1;
    }
    adjustLineSelectionBoundaries(&tempNextTextPos);
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveLineSelectionDown(int32_t amount) {
    textPos_t tempNextTextPos = cursorTextPos;
    int32_t tempCount = 0;
    while (tempCount < amount) {
        textLine_t *tempLine = getNextTextLine(tempNextTextPos.line);
        if (tempLine == NULL) {
            break;
        }
        tempNextTextPos.line = tempLine;
        tempCount += 1;
    }
    adjustLineSelectionBoundaries(&tempNextTextPos);
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void insertCharacterUnderCursor(int8_t character) {
    int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (isStartOfNonconsecutiveEscapeSequence) {
        cleanUpHistoryAction(&firstNonconsecutiveEscapeSequenceAction);
        firstNonconsecutiveEscapeSequenceAction = createHistoryActionFromTextLine(cursorTextPos.line, HISTORY_ACTION_DELETE);
        nonconsecutiveEscapeSequencePreviousCursorTextPos = convertTextPosToHistoryTextPos(&cursorTextPos);
    } else {
        if (!historyFrameIsConsecutive) {
            addHistoryFrame();
            recordTextLineDeleted(cursorTextPos.line);
        }
    }
    insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), index, &character, 1);
    if (!isStartOfNonconsecutiveEscapeSequence) {
        if (historyFrameIsConsecutive) {
            updateHistoryFrameInsertAction(cursorTextPos.line);
        } else {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = true;
        }
    }
    cursorTextPos.column += 1;
    if (cursorTextPos.column >= viewPortWidth) {
        cursorTextPos.column = 0;
        cursorTextPos.row += 1;
    }
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempNewRowCount == tempOldRowCount) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
        displayCursor();
    }
    if (!isStartOfNonconsecutiveEscapeSequence) {
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void decreaseSelectionIndentationLevel();

void deleteCharacterBeforeCursor(int8_t shouldRecordHistory) {
    int64_t index = getTextPosIndex(&cursorTextPos);
    if (index > 0 && index == getTextLineIndentationEndIndex(cursorTextPos.line)) {
        decreaseSelectionIndentationLevel();
        return;
    }
    index -= 1;
    if (index < 0) {
        if (shouldRecordHistory) {
            addHistoryFrame();
        }
        textLine_t *tempLine = getPreviousTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        addHistoryFrame();
        index = tempLine->textAllocation.length;
        if (shouldRecordHistory) {
            recordTextLineDeleted(tempLine);
        }
        insertTextIntoTextAllocation(&(tempLine->textAllocation), index, cursorTextPos.line->textAllocation.text, cursorTextPos.line->textAllocation.length);
        if (shouldRecordHistory) {
            recordTextLineInserted(tempLine);
            recordTextLineDeleted(cursorTextPos.line);
        }
        handleTextLineDeleted(cursorTextPos.line);
        deleteTextLine(cursorTextPos.line);
        setTextPosIndex(&cursorTextPos, index);
        cursorSnapColumn = cursorTextPos.column;
        if (topTextLine == cursorTextPos.line) {
            topTextLine = tempLine;
        }
        cursorTextPos.line = tempLine;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
            displayCursor();
        }
        eraseLineNumber();
        displayLineNumber();
        if (shouldRecordHistory) {
            historyFrameIsConsecutive = false;
        }
    } else {
        int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
        if (shouldRecordHistory) {
            if (!historyFrameIsConsecutive) {
                addHistoryFrame();
                recordTextLineDeleted(cursorTextPos.line);
            }
        }
        removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, 1);
        if (shouldRecordHistory) {
            if (historyFrameIsConsecutive) {
                updateHistoryFrameInsertAction(cursorTextPos.line);
            } else {
                recordTextLineInserted(cursorTextPos.line);
                historyFrameIsConsecutive = true;
            }
        }
        cursorTextPos.column -= 1;
        if (cursorTextPos.column < 0) {
            cursorTextPos.column = viewPortWidth - 1;
            cursorTextPos.row -= 1;
        }
        cursorSnapColumn = cursorTextPos.column;
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
            int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
            if (tempNewRowCount == tempOldRowCount) {
                displayTextLine(tempPosY, cursorTextPos.line);
            } else {
                displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
            }
            displayCursor();
        }
    }
    if (shouldRecordHistory) {
        textBufferIsDirty = true;
        finishCurrentHistoryFrame();
    }
}

void deleteCharacterAfterCursor() {
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (index >= tempLength) {
        textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
        if (tempLine == NULL) {
            return;
        }
        recordTextLineDeleted(cursorTextPos.line);
        insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), tempLength, tempLine->textAllocation.text, tempLine->textAllocation.length);
        recordTextLineInserted(cursorTextPos.line);
        handleTextLineDeleted(tempLine);
        recordTextLineDeleted(tempLine);
        deleteTextLine(tempLine);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        displayCursor();
        eraseLineNumber();
        displayLineNumber();
    } else {
        int64_t tempOldRowCount = getTextLineRowCount(cursorTextPos.line);
        if (!historyFrameIsConsecutive) {
            addHistoryFrame();
            recordTextLineDeleted(cursorTextPos.line);
        }
        removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, 1);
        if (historyFrameIsConsecutive) {
            updateHistoryFrameInsertAction(cursorTextPos.line);
        } else {
            recordTextLineInserted(cursorTextPos.line);
            historyFrameIsConsecutive = true;
        }
        int64_t tempNewRowCount = getTextLineRowCount(cursorTextPos.line);
        int64_t tempPosY = getTextLinePosY(cursorTextPos.line);
        if (tempNewRowCount == tempOldRowCount) {
            displayTextLine(tempPosY, cursorTextPos.line);
        } else {
            displayTextLinesUnderAndIncludingTextLine(tempPosY, cursorTextPos.line);
        }
        displayCursor();
    }
    textBufferIsDirty = true;
    finishCurrentHistoryFrame();
}

void insertNewlineBeforeCursorHelper(int32_t baseIndentationLevel) {
    textLine_t *tempLine = createEmptyTextLine();
    textLine_t *tempLine2 = cursorTextPos.line;
    int64_t index = getTextPosIndex(&cursorTextPos);
    int64_t tempAmount = cursorTextPos.line->textAllocation.length - index;
    insertTextIntoTextAllocation(&(tempLine->textAllocation), 0, cursorTextPos.line->textAllocation.text + index, tempAmount);
    int32_t tempCount = 0;
    while (tempCount < baseIndentationLevel) {
        increaseTextLineIndentationLevelHelper(tempLine);
        tempCount += 1;
    }
    recordTextLineDeleted(cursorTextPos.line);
    removeTextFromTextAllocation(&(cursorTextPos.line->textAllocation), index, tempAmount);
    recordTextLineInserted(cursorTextPos.line);
    insertTextLineRight(cursorTextPos.line, tempLine);
    recordTextLineInserted(tempLine);
    cursorTextPos.line = tempLine;
    int64_t tempIndex = getTextLineIndentationEndIndex(cursorTextPos.line);
    setTextPosIndex(&cursorTextPos, tempIndex);
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        int64_t tempPosY = getTextLinePosY(tempLine2);
        displayTextLinesUnderAndIncludingTextLine(tempPosY, tempLine2);
        displayCursor();
    }
    eraseLineNumber();
    displayLineNumber();
    textBufferIsDirty = true;
}

void insertNewlineBeforeCursor() {
    addHistoryFrame();
    int32_t tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    insertNewlineBeforeCursorHelper(tempLevel);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void saveFile() {
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Saving...");
    refresh();
    int8_t tempNewline = '\n';
    FILE *tempFile = fopen((char *)filePath, "w");
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
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Saved file.");
    textBufferIsDirty = false;
}

void moveCursorToBeginningOfLine() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfLine() {
    textPos_t tempNextTextPos = cursorTextPos;
    int64_t tempLength = tempNextTextPos.line->textAllocation.length;
    setTextPosIndex(&tempNextTextPos, tempLength);
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToBeginningOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getLeftmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void moveCursorToEndOfFile() {
    textPos_t tempNextTextPos = cursorTextPos;
    tempNextTextPos.line = getRightmostTextLine(rootTextLine);
    tempNextTextPos.column = 0;
    tempNextTextPos.row = 0;
    moveCursor(&tempNextTextPos);
    historyFrameIsConsecutive = false;
}

void copySelectionHelper() {
    FILE *tempFile = fopen((char *)clipboardFilePath, "w");
    textPos_t *tempFirstTextPos;
    textPos_t *tempLastTextPos;
    if (isHighlighting) {
        tempFirstTextPos = getFirstHighlightTextPos();
        tempLastTextPos = getLastHighlightTextPos();
    } else {
        tempFirstTextPos = &cursorTextPos;
        tempLastTextPos = &cursorTextPos;
    }
    textLine_t *tempLine = tempFirstTextPos->line;
    while (true) {
        int64_t tempStartIndex;
        int64_t tempEndIndex;
        if (tempLine == tempFirstTextPos->line) {
            tempStartIndex = getTextPosIndex(tempFirstTextPos);
        } else {
            tempStartIndex = 0;
        }
        if (tempLine == tempLastTextPos->line) {
            tempEndIndex = getTextPosIndex(tempLastTextPos) + 1;
        } else {
            tempEndIndex = tempLine->textAllocation.length + 1;
        }
        int8_t tempShouldWriteNewline;
        if (tempEndIndex > tempLine->textAllocation.length) {
            tempShouldWriteNewline = true;
            tempEndIndex = tempLine->textAllocation.length;
        } else {
            tempShouldWriteNewline = false;
        }
        int64_t tempAmount = tempEndIndex - tempStartIndex;
        fwrite(tempLine->textAllocation.text + tempStartIndex, 1, tempAmount, tempFile);
        if (tempShouldWriteNewline) {
            fwrite("\n", 1, 1, tempFile);
        }
        if (tempLine == tempLastTextPos->line) {
            break;
        }
        tempLine = getNextTextLine(tempLine);
    }
    fclose(tempFile);
    systemCopyClipboardFile();
    remove((char *)clipboardFilePath);
}

void copySelection() {
    copySelectionHelper();
    setActivityMode(COMMAND_MODE);
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Copied selection.");
}

void deleteSelectionHelper() {
    if (!isHighlighting) {
        deleteCharacterAfterCursor();
        return;
    }
    textPos_t tempFirstTextPos = *(getFirstHighlightTextPos());
    textPos_t tempLastTextPos = *(getLastHighlightTextPos());
    int64_t index = getTextPosIndex(&tempLastTextPos);
    if (index >= tempLastTextPos.line->textAllocation.length) {
        textLine_t *tempLine = getNextTextLine(tempLastTextPos.line);
        if (tempLine != NULL) {
            tempLastTextPos.line = tempLine;
            tempLastTextPos.row = 0;
            tempLastTextPos.column = 0;
        }
    } else {
        setTextPosIndex(&tempLastTextPos, index + 1);
    }
    int64_t tempStartIndex = getTextPosIndex(&tempFirstTextPos);
    int64_t tempEndIndex = getTextPosIndex(&tempLastTextPos);
    if (tempFirstTextPos.line == tempLastTextPos.line) {
        int64_t tempAmount = tempEndIndex - tempStartIndex;
        recordTextLineDeleted(tempFirstTextPos.line);
        removeTextFromTextAllocation(&(tempFirstTextPos.line->textAllocation), tempStartIndex, tempAmount);
        recordTextLineInserted(tempFirstTextPos.line);
    } else {
        textLine_t *tempLine = getNextTextLine(tempFirstTextPos.line);
        while (tempLine != tempLastTextPos.line) {
            textLine_t *tempNextLine = getNextTextLine(tempLine);
            recordTextLineDeleted(tempLine);
            handleTextLineDeleted(tempLine);
            deleteTextLine(tempLine);
            tempLine = tempNextLine;
        }
        int64_t tempAmount;
        tempAmount = tempFirstTextPos.line->textAllocation.length - tempStartIndex;
        recordTextLineDeleted(tempFirstTextPos.line);
        removeTextFromTextAllocation(&(tempFirstTextPos.line->textAllocation), tempStartIndex, tempAmount);
        tempAmount = tempLastTextPos.line->textAllocation.length - tempEndIndex;
        insertTextIntoTextAllocation(&(tempFirstTextPos.line->textAllocation), tempStartIndex, tempLastTextPos.line->textAllocation.text + tempEndIndex, tempAmount);
        recordTextLineInserted(tempFirstTextPos.line);
        handleTextLineDeleted(tempLastTextPos.line);
        recordTextLineDeleted(tempLastTextPos.line);
        deleteTextLine(tempLastTextPos.line);
    }
    cursorTextPos = tempFirstTextPos;
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempFirstTextPos.line), tempFirstTextPos.line);
        displayCursor();
    }
    eraseLineNumber();
    displayLineNumber();
    textBufferIsDirty = true;
}

void deleteSelection() {
    addHistoryFrame();
    deleteSelectionHelper();
    setActivityMode(COMMAND_MODE);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void cutSelection() {
    addHistoryFrame();
    copySelectionHelper();
    deleteSelectionHelper();
    setActivityMode(COMMAND_MODE);
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Cut selection.");
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void pasteBeforeCursorHelper(FILE *file, int32_t baseIndentationLevel) {
    fseek(file, 0, SEEK_SET);
    textLine_t *tempFirstLine = cursorTextPos.line;
    while (true) {
        int8_t *tempText = NULL;
        size_t tempSize = 0;
        int64_t tempCount = getline((char **)&tempText, &tempSize, file);
        if (tempCount < 0) {
            break;
        }
        int8_t tempContainsNewline;
        tempCount = removeBadCharacters(tempText, &tempContainsNewline);
        int64_t index = getTextPosIndex(&cursorTextPos);
        recordTextLineDeleted(cursorTextPos.line);
        insertTextIntoTextAllocation(&(cursorTextPos.line->textAllocation), index, tempText, tempCount);
        recordTextLineInserted(cursorTextPos.line);
        index += tempCount;
        setTextPosIndex(&cursorTextPos, index);
        if (tempContainsNewline) {
            insertNewlineBeforeCursorHelper(baseIndentationLevel);
            cursorTextPos.row = 0;
            cursorTextPos.column = 0;
        }
        cursorSnapColumn = cursorTextPos.column;
        free(tempText);
    }
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempFirstLine), tempFirstLine);
        displayCursor();
    }
    eraseLineNumber();
    displayLineNumber();
    textBufferIsDirty = true;
}

int8_t fileEndsInNewline(FILE *file) {
    fseek(file, 0, SEEK_END);
    int64_t tempSize = ftell(file);
    if (tempSize <= 0) {
        return false;
    }
    fseek(file, -1, SEEK_END);
    int8_t tempCharacter;
    fread(&tempCharacter, 1, 1, file);
    return (tempCharacter == '\n');
}

void pasteBeforeCursor() {
    addHistoryFrame();
    int8_t tempLastIsHighlighting = isHighlighting;
    if (isHighlighting) {
        deleteSelectionHelper();
        setActivityMode(COMMAND_MODE);
    }
    systemPasteClipboardFile();
    FILE *tempFile = fopen((char *)clipboardFilePath, "r");
    int32_t tempLevel;
    if (fileEndsInNewline(tempFile) && !tempLastIsHighlighting) {
        cursorTextPos.row = 0;
        cursorTextPos.column = 0;
        tempLevel = 0;
    } else {
        tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    }
    pasteBeforeCursorHelper(tempFile, tempLevel);
    fclose(tempFile);
    remove((char *)clipboardFilePath);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void pasteAfterCursor() {
    addHistoryFrame();
    int8_t tempLastIsHighlighting = isHighlighting;
    if (isHighlighting) {
        deleteSelectionHelper();
        setActivityMode(COMMAND_MODE);
    }
    systemPasteClipboardFile();
    FILE *tempFile = fopen((char *)clipboardFilePath, "r");
    int32_t tempLevel;
    if (!tempLastIsHighlighting) {
        if (fileEndsInNewline(tempFile)) {
            eraseCursor();
            textLine_t *tempLine = getNextTextLine(cursorTextPos.line);
            if (tempLine == NULL) {
                int64_t tempLength = cursorTextPos.line->textAllocation.length;
                setTextPosIndex(&cursorTextPos, tempLength);
                insertNewlineBeforeCursorHelper(0);
            } else {
                cursorTextPos.line = tempLine;
                cursorTextPos.row = 0;
                cursorTextPos.column = 0;
            }
            tempLevel = 0;
        } else {
            moveCursorRight(1);
            tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
        }
    } else {
        tempLevel = getTextLineIndentationLevel(cursorTextPos.line);
    }
    pasteBeforeCursorHelper(tempFile, tempLevel);
    fclose(tempFile);
    remove((char *)clipboardFilePath);
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void increaseSelectionIndentationLevel() {
    addHistoryFrame();
    textPos_t *tempStartTextPos;
    textPos_t *tempEndTextPos;
    if (isHighlighting) {
        tempStartTextPos = getFirstHighlightTextPos();
        tempEndTextPos = getLastHighlightTextPos();
    } else {
        tempStartTextPos = &cursorTextPos;
        tempEndTextPos = &cursorTextPos;
    }
    eraseCursor();
    textLine_t *tempLine = tempStartTextPos->line;
    int8_t tempIsSingleLine = tempStartTextPos->line == tempEndTextPos->line;
    if (tempIsSingleLine) {
        int64_t tempOldRowCount = getTextLineRowCount(tempLine);
        increaseTextLineIndentationLevelHelper(tempLine);
        int64_t tempNewRowCount = getTextLineRowCount(tempLine);
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (tempOldRowCount == tempNewRowCount) {
                displayTextLine(getTextLinePosY(tempLine), tempLine);
            } else {
                displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempLine), tempLine);
            }
            displayCursor();
        }
    } else {
        while (true) {
            increaseTextLineIndentationLevelHelper(tempLine);
            if (tempLine == tempEndTextPos->line) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
        }
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempStartTextPos->line), tempStartTextPos->line);
            displayCursor();
        }
    }
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void decreaseSelectionIndentationLevel() {
    addHistoryFrame();
    textPos_t *tempStartTextPos;
    textPos_t *tempEndTextPos;
    if (isHighlighting) {
        tempStartTextPos = getFirstHighlightTextPos();
        tempEndTextPos = getLastHighlightTextPos();
    } else {
        tempStartTextPos = &cursorTextPos;
        tempEndTextPos = &cursorTextPos;
    }
    eraseCursor();
    textLine_t *tempLine = tempStartTextPos->line;
    int8_t tempIsSingleLine = tempStartTextPos->line == tempEndTextPos->line;
    if (tempIsSingleLine) {
        int64_t tempOldRowCount = getTextLineRowCount(tempLine);
        decreaseTextLineIndentationLevelHelper(tempLine);
        int64_t tempNewRowCount = getTextLineRowCount(tempLine);
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            if (tempOldRowCount == tempNewRowCount) {
                displayTextLine(getTextLinePosY(tempLine), tempLine);
            } else {
                displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempLine), tempLine);
            }
            displayCursor();
        }
    } else {
        while (true) {
            decreaseTextLineIndentationLevelHelper(tempLine);
            if (tempLine == tempEndTextPos->line) {
                break;
            }
            tempLine = getNextTextLine(tempLine);
        }
        int8_t tempResult = scrollCursorOntoScreen();
        if (!tempResult) {
            displayTextLinesUnderAndIncludingTextLine(getTextLinePosY(tempStartTextPos->line), tempStartTextPos->line);
            displayCursor();
        }
    }
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

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

textPos_t findNextTermInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    while (index <= tempLength - searchTermLength) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempPos;
            tempPos.line = pos->line;
            setTextPosIndex(&tempPos, index);
            *isMissing = false;
            return tempPos;
        }
        index += 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findPreviousTermInTextLine(textPos_t *pos, int8_t *isMissing) {
    int64_t tempLength = pos->line->textAllocation.length;
    int64_t index = getTextPosIndex(pos);
    if (index > tempLength - searchTermLength) {
        index = tempLength - searchTermLength;
    }
    while (index >= 0) {
        if (equalData(searchTerm, pos->line->textAllocation.text + index, searchTermLength)) {
            textPos_t tempPos;
            tempPos.line = pos->line;
            setTextPosIndex(&tempPos, index);
            *isMissing = false;
            return tempPos;
        }
        index -= 1;
    }
    *isMissing = true;
    return *pos;
}

textPos_t findNextTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempPos2 = findNextTermInTextLine(&tempPos, &tempIsMissing);
        if (tempIsMissing) {
            tempPos.line = getNextTextLine(tempPos.line);
            if (tempPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            tempPos.row = 0;
            tempPos.column = 0;
        } else {
            *isMissing = false;
            return tempPos2;
        }
    }
}

textPos_t findPreviousTermTextPos(textPos_t *pos, int8_t *isMissing) {
    textPos_t tempPos = *pos;
    while (true) {
        int8_t tempIsMissing;
        textPos_t tempPos2 = findPreviousTermInTextLine(&tempPos, &tempIsMissing);
        if (tempIsMissing) {
            tempPos.line = getPreviousTextLine(tempPos.line);
            if (tempPos.line == NULL) {
                *isMissing = true;
                return *pos;
            }
            int64_t tempLength = tempPos.line->textAllocation.length;
            setTextPosIndex(&tempPos, tempLength);
        } else {
            *isMissing = false;
            return tempPos2;
        }
    }
}

int8_t gotoNextTerm() {
    moveCursorRight(1);
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos = findNextTermTextPos(&cursorTextPos, &tempIsMissing);
    if (tempIsMissing) {
        tempTextPos.line = getLeftmostTextLine(rootTextLine);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        tempTextPos = findNextTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            moveCursorLeft(1);
            return false;
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

int8_t gotoPreviousTerm() {
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    if (getTextLineNumber(cursorTextPos.line) <= 1 && cursorTextPos.row == 0 && cursorTextPos.column == 0) {
        tempTextPos.line = getRightmostTextLine(rootTextLine);
        int64_t tempLength = tempTextPos.line->textAllocation.length;
        setTextPosIndex(&tempTextPos, tempLength);
        tempTextPos = findPreviousTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            return false;
        }
    } else {
        moveCursorLeft(1);
        tempTextPos = findPreviousTermTextPos(&cursorTextPos, &tempIsMissing);
        if (tempIsMissing) {
            tempTextPos.line = getRightmostTextLine(rootTextLine);
            int64_t tempLength = tempTextPos.line->textAllocation.length;
            setTextPosIndex(&tempTextPos, tempLength);
            tempTextPos = findPreviousTermTextPos(&tempTextPos, &tempIsMissing);
            if (tempIsMissing) {
                moveCursorRight(1);
                return false;
            }
        }
    }
    moveCursor(&tempTextPos);
    return true;
}

void executeTextCommand() {
    int8_t *tempTermList[20];
    int8_t tempTermIndex = 0;
    int8_t tempIsInQuotes = false;
    int8_t *tempIndex1 = textCommandBuffer;
    int8_t *tempIndex2 = textCommandBuffer;
    int8_t tempIsStartOfTerm = true;
    while (true) {
        int8_t tempCharacter = *tempIndex1;
        tempIndex1 += 1;
        if (tempCharacter == 0) {
            break;
        }
        int8_t tempIsWhitespace = isWhitespace(tempCharacter);
        if (tempCharacter == '\\') {
            int8_t tempCharacter = *tempIndex1;
            tempIndex1 += 1;
            *tempIndex2 = tempCharacter;
            tempIndex2 += 1;
        } else if (tempIsStartOfTerm && !tempIsWhitespace) {
            if (tempCharacter == '"') {
                tempTermList[tempTermIndex] = tempIndex2;
                tempTermIndex += 1;
                tempIsInQuotes = true;
            } else {
                tempTermList[tempTermIndex] = tempIndex2;
                *tempIndex2 = tempCharacter;
                tempIndex2 += 1;
                tempTermIndex += 1;
            }
            tempIsStartOfTerm = false;
        } else {
            if (tempCharacter == '"') {
                *tempIndex2 = 0;
                tempIndex2 += 1;
                tempIsInQuotes = false;
                tempIsStartOfTerm = true;
            } else if (!tempIsWhitespace || tempIsInQuotes) {
                *tempIndex2 = tempCharacter;
                tempIndex2 += 1;
            } else {
                *tempIndex2 = 0;
                tempIndex2 += 1;
                tempIndex1 = skipWhitespace(tempIndex1);
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
        setActivityMode(COMMAND_MODE);
        eraseActivityModeOrNotification();
        displayNotification((int8_t *)"Error: Invalid command.");
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "gotoLine") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(COMMAND_MODE);
            eraseActivityModeOrNotification();
            displayNotification((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        int64_t tempLineNumber = atoi((char *)(tempTermList[1]));
        textPos_t tempTextPos;
        tempTextPos.line = getTextLineByNumber(tempLineNumber);
        tempTextPos.row = 0;
        tempTextPos.column = 0;
        if (tempTextPos.line == NULL) {
            setActivityMode(COMMAND_MODE);
            eraseActivityModeOrNotification();
            displayNotification((int8_t *)"Error: Bad line number.");
            return;
        }
        moveCursor(&tempTextPos);
        setActivityMode(COMMAND_MODE);
        return;
    }
    if (strcmp((char *)(tempTermList[0]), "find") == 0) {
        if (tempTermListLength != 2) {
            setActivityMode(COMMAND_MODE);
            eraseActivityModeOrNotification();
            displayNotification((int8_t *)"Error: Wrong number of arguments.");
            return;
        }
        strcpy((char *)searchTerm, (char *)(tempTermList[1]));
        searchTermLength = strlen((char *)searchTerm);
        int8_t tempResult = gotoNextTerm();
        if (!tempResult) {
            setActivityMode(COMMAND_MODE);
            eraseActivityModeOrNotification();
            displayNotification((int8_t *)"Could not find term.");
            return;
        }
        setActivityMode(COMMAND_MODE);
        return;
    }
    setActivityMode(COMMAND_MODE);
    eraseActivityModeOrNotification();
    displayNotification((int8_t *)"Error: Unrecognized command name.");
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
    
    setActivityMode(COMMAND_MODE);
    redrawEverything();
}

void playMacro();

// Returns true if the user has quit.
int8_t handleKey(int32_t key) {
    if (isRecordingMacro && key != 'M') {
        if (macroKeyListLength >= MAXIMUM_MACRO_LENGTH) {
            isRecordingMacro = false;
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
    }
    // Escape.
    if (key == 27) {
        setActivityMode(COMMAND_MODE);
    }
    if (activityMode == TEXT_COMMAND_MODE) {
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
                insertCharacterUnderCursor((int8_t)key);
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
            if (key == KEY_UP) {
                moveLineSelectionUp(1);
            }
            if (key == KEY_DOWN) {
                moveLineSelectionDown(1);
            }
            if (key == 'i') {
                moveLineSelectionUp(1);
            }
            if (key == 'k') {
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
            if (key == 'q') {
                if (textBufferIsDirty) {
                    eraseActivityModeOrNotification();
                    displayNotification((int8_t *)"Unsaved changes. (Shift + Q to quit anyway.)");
                } else {
                    return true;
                }
            }
            if (key == 'Q') {
                return true;
            }
            if (key == 't') {
                setActivityMode(TEXT_ENTRY_MODE);
            }
            if (key == 's') {
                saveFile();
            }
            if (key == '[') {
                moveCursorToBeginningOfLine();
            }
            if (key == ']') {
                moveCursorToEndOfLine();
            }
            if (key == '{') {
                moveCursorToBeginningOfFile();
            }
            if (key == '}') {
                moveCursorToEndOfFile();
            }
            if (key == 'h') {
                setActivityMode(HIGHLIGHT_CHARACTER_MODE);
            }
            if (key == 'H') {
                setActivityMode(HIGHLIGHT_LINE_MODE);
            }
            if (key == 'w') {
                setActivityMode(HIGHLIGHT_WORD_MODE);
            }
            if (key == 'c') {
                copySelection();
            }
            if (key == 'C') {
                cutSelection();
            }
            if (key == 'p') {
                pasteAfterCursor();
            }
            if (key == 'P') {
                pasteBeforeCursor();
            }
            if (key == 'd') {
                deleteSelection();
            }
            if (key == 'u') {
                undoLastAction();
            }
            if (key == 'U') {
                redoLastAction();
            }
            if (key == 'm') {
                playMacro();
            }
            if (key == 'M') {
                eraseActivityModeOrNotification();
                if (isRecordingMacro) {
                    displayNotification((int8_t *)"Finished recording.");
                } else {
                    macroKeyListLength = 0;
                    displayNotification((int8_t *)"Recording macro.");
                }
                isRecordingMacro = !isRecordingMacro;
            }
            if (key == '>') {
                increaseSelectionIndentationLevel();
            }
            if (key == '<') {
                decreaseSelectionIndentationLevel();
            }
            if (key == '/') {
                setActivityMode(TEXT_COMMAND_MODE);
            }
            if (key == 'n') {
                gotoNextTerm();
            }
            if (key == 'N') {
                gotoPreviousTerm();
            }
        }
        if (!isHighlighting) {
            // Backspace.
            if (key == 127 || key == 263) {
                deleteCharacterBeforeCursor(true);
            }
            if (key == '\n') {
                insertNewlineBeforeCursor();
            }
        } else {
            // Backspace.
            if (key == 127 || key == 263) {
                deleteSelection();
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

void setConfigurationVariable(int8_t *name, int64_t value) {
    if (strcmp((char *)name, "colorScheme") == 0) {
        if (value == 0) {
            primaryColorPair = BLACK_ON_WHITE;
            secondaryColorPair = WHITE_ON_BLACK;
        }
        if (value == 1) {
            primaryColorPair = WHITE_ON_BLACK;
            secondaryColorPair = BLACK_ON_WHITE;
        }
    }
    if (strcmp((char *)name, "indentationWidth") == 0) {
        indentationWidth = value;
    }
    if (strcmp((char *)name, "shouldUseHardTabs") == 0) {
        shouldUseHardTabs = value;
    }
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
    
    processRcFile();
    
    FILE *tempFile = fopen((char *)filePath, "r");
    if (tempFile == NULL) {
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
    cursorTextPos.line = topTextLine;
    cursorTextPos.row = 0;
    cursorTextPos.column = 0;
    cursorSnapColumn = 0;
    firstNonconsecutiveEscapeSequenceAction.text = NULL;
    
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
        int32_t tempKey = getch();
        int8_t tempResult = handleKey(tempKey);
        if (tempResult) {
            break;
        }
    }
    
    endwin();
    
    return 0;
}
