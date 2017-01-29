
#include <curses.h>
#include "textLine.h"
#include "breadtext.h"

#ifndef DISPLAY_HEADER_FILE
#define DISPLAY_HEADER_FILE

WINDOW *window;
int32_t windowWidth;
int32_t windowHeight;
int32_t viewPortWidth;
int32_t viewPortHeight;
int8_t primaryColorPair;
int8_t secondaryColorPair;
textLine_t *topTextLine;
int64_t topTextLineRow;

int64_t getTextLineRowCount(textLine_t *line);
int64_t getTextLinePosY(textLine_t *line);
int64_t getCursorPosY();
int8_t getCursorCharacter();
void eraseCursor();
void displayCursor();
void eraseTextCommandCursor();
void displayTextCommandCursor();
textPos_t *getFirstHighlightTextPos();
textPos_t *getLastHighlightTextPos();
int64_t displayTextLine(int64_t posY, textLine_t *line);
void displayTextLinesUnderAndIncludingTextLine(int64_t posY, textLine_t *line);
void displayAllTextLines();
void eraseStatusBar();
void eraseActivityMode();
void displayActivityMode();
void eraseLineNumber();
void displayLineNumber();
void eraseNotification();
void displayNotification(int8_t *message);
void eraseActivityModeOrNotification();
void displayStatusBar();
void displayHelpMessage();
void scrollHelpMessageUp();
void scrollHelpMessageDown();
void redrawEverything();
int8_t scrollCursorOntoScreen();

// DISPLAY_TEST_HEADER_FILE
#endif
