
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"

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

int8_t textLineOnlyContainsWhitespace(textLine_t *line) {
    int64_t tempLength = line->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = line->textAllocation.text[index];
        if (!isWhitespace(tempCharacter)) {
            return false;
        }
        index += 1;
    }
    return true;
}
