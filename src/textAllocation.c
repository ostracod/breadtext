
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utilities.h"
#include "textAllocation.h"
#include "motion.h"

void eraseSyntaxHighlighting(textAllocation_t *allocation) {
    if (allocation->syntaxHighlighting != NULL) {
        free(allocation->syntaxHighlighting);
        allocation->syntaxHighlighting = NULL;
    }
}

void setTextAllocationSize(textAllocation_t *allocation, int64_t size) {
    eraseSyntaxHighlighting(allocation);
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
    eraseSyntaxHighlighting(allocation);
    int64_t tempLength = allocation->length + amount;
    if (tempLength > allocation->allocationSize || allocation->text == NULL) {
        setTextAllocationSize(allocation, tempLength * 2);
    }
    int64_t tempAmount = allocation->length - index;
    copyData(allocation->text + index + amount, allocation->text + index, tempAmount);
    copyData(allocation->text + index, text, amount);
    allocation->length = tempLength;
    int8_t tempMarkIndex = 0;
    while (tempMarkIndex < MARK_AMOUNT) {
        mark_t *tempMark = markList + tempMarkIndex;
        if (tempMark->isSet) {
            textAllocation_t *tempAllocation = &(tempMark->textLine->textAllocation);
            if (allocation == tempAllocation && tempMark->characterIndex >= index) {
                tempMark->characterIndex += amount;
            }
        }
        tempMarkIndex += 1;
    }
}

void removeTextFromTextAllocation(textAllocation_t *allocation, int64_t index, int64_t amount) {
    eraseSyntaxHighlighting(allocation);
    int64_t tempLength = allocation->length - amount;
    int64_t tempAmount = allocation->length - index;
    copyData(allocation->text + index, allocation->text + index + amount, tempAmount);
    if (tempLength < allocation->allocationSize / 4) {
        setTextAllocationSize(allocation, tempLength * 2);
    }
    allocation->length = tempLength;
    int8_t tempMarkIndex = 0;
    while (tempMarkIndex < MARK_AMOUNT) {
        mark_t *tempMark = markList + tempMarkIndex;
        if (tempMark->isSet) {
            textAllocation_t *tempAllocation = &(tempMark->textLine->textAllocation);
            if (allocation == tempAllocation && tempMark->characterIndex >= index) {
                if (tempMark->characterIndex >= index + amount) {
                    tempMark->characterIndex -= amount;
                } else {
                    tempMark->characterIndex = index;
                }
            }
        }
        tempMarkIndex += 1;
    }
}

void cleanUpTextAllocation(textAllocation_t *allocation) {
    if (allocation->text != NULL) {
        free(allocation->text);
    }
    if (allocation->syntaxHighlighting != NULL) {
        free(allocation->syntaxHighlighting);
    }
}




