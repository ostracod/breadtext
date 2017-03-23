
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utilities.h"
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "display.h"
#include "breadtext.h"

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

int8_t moveTextPosForward(textPos_t *pos) {
    int64_t index = getTextPosIndex(pos);
    int64_t tempLength = pos->line->textAllocation.length;
    if (index >= tempLength) {
        textLine_t *tempLine = getNextTextLine(pos->line);
        if (tempLine == NULL) {
            return false;
        }
        pos->line = tempLine;
        setTextPosIndex(pos, 0);
    } else {
        index += 1;
        setTextPosIndex(pos, index);
    }
    return true;
}

int8_t moveTextPosBackward(textPos_t *pos) {
    int64_t index = getTextPosIndex(pos);
    if (index <= 0) {
        textLine_t *tempLine = getPreviousTextLine(pos->line);
        if (tempLine == NULL) {
            return false;
        }
        int64_t tempLength = tempLine->textAllocation.length;
        pos->line = tempLine;
        setTextPosIndex(pos, tempLength);
    } else {
        index -= 1;
        setTextPosIndex(pos, index);
    }
    return true;
}

int8_t getTextPosCharacter(textPos_t *pos) {
    int64_t index = getTextPosIndex(pos);
    int64_t tempLength = pos->line->textAllocation.length;
    if (index >= tempLength) {
        return '\n';
    } else {
        return pos->line->textAllocation.text[index];
    }
}

