
#include "textAllocation.h"

#ifndef TEXT_LINE_HEADER_FILE
#define TEXT_LINE_HEADER_FILE

typedef struct textLine textLine_t;
struct textLine {
    textLine_t *parent;
    textLine_t *leftChild;
    textLine_t *rightChild;
    textAllocation_t textAllocation;
    int8_t depth;
    int64_t lineCount;
};

textLine_t *rootTextLine;

textLine_t *createEmptyTextLine();
textLine_t *getLeftmostTextLine(textLine_t *line);
textLine_t *getRightmostTextLine(textLine_t *line);
void updateTextLineInfoAboutChildren(textLine_t *line);
int8_t getTextLineBalance(textLine_t *line);
void insertTextLineLeft(textLine_t *parent, textLine_t *child);
void insertTextLineRight(textLine_t *parent, textLine_t *child);
void deleteTextLine(textLine_t *line);
textLine_t *getPreviousTextLine(textLine_t *line);
textLine_t *getNextTextLine(textLine_t *line);
int64_t getTextLineNumber(textLine_t *line);
int8_t textLineIsAfterTextLine(textLine_t *line1, textLine_t *line2);
textLine_t *getTextLineByNumber(int64_t number);

// TEXT_LINE_HEADER_FILE
#endif
