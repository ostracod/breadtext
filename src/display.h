
#include <curses.h>
#include "textAllocation.h"
#include "textLine.h"
#include "textPos.h"
#include "breadtext.h"

#ifndef DISPLAY_HEADER_FILE
#define DISPLAY_HEADER_FILE

#define BRIGHT_COLOR_OFFSET 8
#define DEFAULT_COLOR 1
#define COMMENT_COLOR 2
#define LITERAL_COLOR 3
#define KEYWORD_COLOR 4
#define STATUS_BAR_COLOR 5
#define HIGHLIGHTED_COLOR_OFFSET 5
#define HIGHLIGHTED_DEFAULT_COLOR (DEFAULT_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_COMMENT_COLOR (COMMENT_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_LITERAL_COLOR (LITERAL_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_KEYWORD_COLOR (KEYWORD_COLOR + HIGHLIGHTED_COLOR_OFFSET)
#define HIGHLIGHTED_STATUS_BAR_COLOR (STATUS_BAR_COLOR + HIGHLIGHTED_COLOR_OFFSET)

WINDOW *window;
int32_t windowWidth;
int32_t windowHeight;
int32_t viewPortWidth;
int32_t viewPortHeight;
textLine_t *topTextLine;
int64_t topTextLineRow;
int32_t colorScheme;

int32_t bodyForegroundColor;
int32_t bodyBackgroundColor;
int32_t highlightForegroundColor;
int32_t highlightBackgroundColor;
int32_t statusBarForegroundColor;
int32_t statusBarBackgroundColor;
int32_t keywordColor;
int32_t valueLiteralColor;
int32_t commentColor;

void updateColorPairs();
void setColorFromConfigValue(int32_t *destination, int32_t value);
int32_t convertColorToConfigValue(int32_t color);
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
