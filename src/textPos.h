
#include "textLine.h"

#ifndef TEXT_POS_HEADER_FILE
#define TEXT_POS_HEADER_FILE

typedef struct textPos {
    textLine_t *line;
    int64_t column;
    int64_t row;
} textPos_t;

int8_t equalTextPos(textPos_t *pos1, textPos_t *pos2);
int64_t getTextPosIndex(textPos_t *pos);
void setTextPosIndex(textPos_t *pos, int64_t index);
int8_t textPosIsAfterTextPos(textPos_t *textPos1, textPos_t *textPos2);

// TEXT_POS_HEADER_FILE
#endif

