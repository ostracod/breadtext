
#include <stdio.h>
#include <stdlib.h>
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
#include "breadtext.h"

int64_t findAndReplaceAllTerms(int8_t *replacementText) {
    int64_t tempLength = strlen((char *)replacementText);
    int64_t output = 0;
    int8_t tempIsMissing;
    textPos_t tempTextPos;
    tempTextPos.line = getLeftmostTextLine(rootTextLine);
    tempTextPos.row = 0;
    tempTextPos.column = 0;
    while (true) {
        tempTextPos = findNextTermTextPos(&tempTextPos, &tempIsMissing);
        if (tempIsMissing) {
            break;
        }
        if (output == 0) {
            addHistoryFrame();
        }
        int64_t index = getTextPosIndex(&tempTextPos);
        recordTextLineDeleted(tempTextPos.line);
        removeTextFromTextAllocation(&(tempTextPos.line->textAllocation), index, searchTermLength);
        insertTextIntoTextAllocation(&(tempTextPos.line->textAllocation), index, replacementText, tempLength);
        recordTextLineInserted(tempTextPos.line);
        index += tempLength;
        setTextPosIndex(&tempTextPos, index);
        output += 1;
    }
    if (output > 0) {
        finishCurrentHistoryFrame();
        historyFrameIsConsecutive = false;
        displayAllTextLines();
        textBufferIsDirty = true;
    }
    return output;
}
