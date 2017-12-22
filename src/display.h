
#include <curses.h>
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "breadtext.h"

#ifndef DISPLAY_HEADER_FILE
#define DISPLAY_HEADER_FILE

#define DEFAULT_COLOR 0
#define COMMENT_COLOR 1
#define LITERAL_COLOR 2
#define KEYWORD_COLOR 3
#define HIGHLIGHTED_COLOR_OFFSET 4
#define HIGHLIGHTED_DEFAULT_COLOR (DEFAULT_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_COMMENT_COLOR (COMMENT_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_LITERAL_COLOR (LITERAL_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_KEYWORD_COLOR (KEYWORD_COLOR + HIGHLIGHTED_COLOR_OFFSET)

WINDOW *window;
int32_t windowWidth;
int32_t windowHeight;
int32_t viewPortWidth;
int32_t viewPortHeight;
int8_t colorSet[8];
textLine_t *topTextLine;
int64_t topTextLineRow;
int32_t colorScheme;

void setColorScheme(int32_t number);
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
void notifyUser(int8_t *message);
void displayStatusBar();
void displayHelpMessage();
void scrollHelpMessageUp(int64_t amount);
void scrollHelpMessageDown(int64_t amount);
void redrawEverything();
int8_t scrollCursorOntoScreen();
void redrawHighlightLines();

// DISPLAY_TEST_HEADER_FILE
#endif
