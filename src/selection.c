
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include "breadtext.h"

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
        deleteCharacterAfterCursor(false);
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
    setActivityMode(COMMAND_MODE);
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
    if (!isHighlighting) {
        deleteCharacterAfterCursor(true);
        return;
    }
    addHistoryFrame();
    deleteSelectionHelper();
    finishCurrentHistoryFrame();
    historyFrameIsConsecutive = false;
}

void cutSelection() {
    addHistoryFrame();
    copySelectionHelper();
    deleteSelectionHelper();
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
            int64_t index = getIndentationWidth(baseIndentationLevel);
            setTextPosIndex(&cursorTextPos, index);
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
