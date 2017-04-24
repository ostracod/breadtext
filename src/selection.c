
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
#include "motion.h"
#include "insertDelete.h"
#include "selection.h"
#include "breadtext.h"

int8_t copySelectionHelper() {
    FILE *tempFile = fopen((char *)clipboardFilePath, "w");
    if (tempFile == NULL) {
        notifyUser((int8_t *)"ERROR: Could not copy text.");
        return false;
    }
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
    return true;
}

void copySelection() {
    int8_t tempResult = copySelectionHelper();
    if (!tempResult) {
        return;
    }
    setActivityMode(COMMAND_MODE);
    notifyUser((int8_t *)"Copied selection.");
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
    int8_t tempResult = copySelectionHelper();
    if (!tempResult) {
        return;
    }
    addHistoryFrame();
    deleteSelectionHelper();
    notifyUser((int8_t *)"Cut selection.");
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

int8_t isHighlightingLines() {
    textPos_t *tempFirstTextPos = getFirstHighlightTextPos();
    textPos_t *tempLastTextPos = getLastHighlightTextPos();
    if (tempFirstTextPos->row != 0 || tempFirstTextPos->column != 0) {
        return false;
    }
    int64_t index = getTextPosIndex(tempLastTextPos);
    int64_t tempLength = tempLastTextPos->line->textAllocation.length;
    return (index == tempLength);
}

void pasteBeforeCursor() {
    systemPasteClipboardFile();
    FILE *tempFile = fopen((char *)clipboardFilePath, "r");
    if (tempFile == NULL) {
        notifyUser((int8_t *)"ERROR: Could not paste text.");
        return;
    }
    addHistoryFrame();
    int8_t tempLastIsHighlighting = isHighlighting;
    int8_t tempLastIsHighlightingLines;
    if (isHighlighting) {
        tempLastIsHighlightingLines = isHighlightingLines();
        deleteSelectionHelper();
    } else {
        tempLastIsHighlightingLines = false;
    }
    int32_t tempLevel;
    if (fileEndsInNewline(tempFile) && (!tempLastIsHighlighting || tempLastIsHighlightingLines)) {
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
    systemPasteClipboardFile();
    FILE *tempFile = fopen((char *)clipboardFilePath, "r");
    if (tempFile == NULL) {
        notifyUser((int8_t *)"ERROR: Could not paste text.");
        return;
    }
    addHistoryFrame();
    int8_t tempLastIsHighlighting = isHighlighting;
    int8_t tempLastIsHighlightingLines;
    if (isHighlighting) {
        tempLastIsHighlightingLines = isHighlightingLines();
        deleteSelectionHelper();
    } else {
        tempLastIsHighlightingLines = false;
    }
    int8_t tempFileEndsInNewline = fileEndsInNewline(tempFile);
    int32_t tempLevel;
    if (tempLastIsHighlightingLines && tempFileEndsInNewline) {
        cursorTextPos.row = 0;
        cursorTextPos.column = 0;
        tempLevel = 0;
    } else if (!tempLastIsHighlighting) {
        if (tempFileEndsInNewline) {
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

void highlightWord() {
    int64_t tempStartIndex = getTextPosIndex(&cursorTextPos);
    int64_t tempEndIndex = tempStartIndex;
    int64_t tempLength = cursorTextPos.line->textAllocation.length;
    if (tempStartIndex < tempLength) {
        int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex];
        if (isWordCharacter(tempCharacter)) {
            while (tempStartIndex > 0) {
                int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempStartIndex - 1];
                if (!isWordCharacter(tempCharacter)) {
                    break;
                }
                tempStartIndex -= 1;
            }
            while (tempEndIndex < tempLength - 1) {
                int8_t tempCharacter = cursorTextPos.line->textAllocation.text[tempEndIndex + 1];
                if (!isWordCharacter(tempCharacter)) {
                    break;
                }
                tempEndIndex += 1;
            }
        }
    }
    highlightTextPos.line = cursorTextPos.line;
    setTextPosIndex(&highlightTextPos, tempStartIndex);
    setTextPosIndex(&cursorTextPos, tempEndIndex);
    scrollCursorOntoScreen();
    setActivityMode(HIGHLIGHT_STATIC_MODE);
}

int8_t highlightEnclosureHelper() {
    int8_t tempCharacter = promptSingleCharacter();
    if (tempCharacter != 0) {
        int8_t tempStartCharacter = tempCharacter;
        int8_t tempEndCharacter = tempCharacter;
        if (tempCharacter == '(') {
            tempEndCharacter = ')';
        }
        if (tempCharacter == ')') {
            tempStartCharacter = '(';
        }
        if (tempCharacter == '[') {
            tempEndCharacter = ']';
        }
        if (tempCharacter == ']') {
            tempStartCharacter = '[';
        }
        if (tempCharacter == '{') {
            tempEndCharacter = '}';
        }
        if (tempCharacter == '}') {
            tempStartCharacter = '{';
        }
        if (tempCharacter == '<') {
            tempEndCharacter = '>';
        }
        if (tempCharacter == '>') {
            tempStartCharacter = '<';
        }
        int16_t tempDepth;
        textPos_t tempStartPos = cursorTextPos;
        tempDepth = 1;
        while (true) {
            int8_t tempCharacter = getTextPosCharacter(&tempStartPos);
            if (tempCharacter == tempStartCharacter) {
                tempDepth -= 1;
                if (tempDepth <= 0) {
                    break;
                }
            }
            if (tempCharacter == tempEndCharacter && tempStartCharacter != tempEndCharacter) {
                tempDepth += 1;
            }
            int8_t tempResult = moveTextPosBackward(&tempStartPos);
            if (!tempResult) {
                notifyUser((int8_t *)"Could not find start character.");
                return false;
            }
        }
        textPos_t tempEndPos = cursorTextPos;
        tempDepth = 1;
        while (true) {
            int8_t tempCharacter = getTextPosCharacter(&tempEndPos);
            if (tempCharacter == tempEndCharacter) {
                tempDepth -= 1;
                if (tempDepth <= 0) {
                    break;
                }
            }
            if (tempCharacter == tempStartCharacter && tempStartCharacter != tempEndCharacter) {
                tempDepth += 1;
            }
            int8_t tempResult = moveTextPosForward(&tempEndPos);
            if (!tempResult) {
                notifyUser((int8_t *)"Could not find end character.");
                return false;
            }
        }
        highlightTextPos = tempStartPos;
        cursorTextPos = tempEndPos;
        return true;
    }
    return false;
}

void highlightEnclosureExclusive() {
    int8_t tempResult = highlightEnclosureHelper();
    if (tempResult) {
        moveTextPosForward(&highlightTextPos);
        moveTextPosBackward(&cursorTextPos);
        scrollCursorOntoScreen();
        setActivityMode(HIGHLIGHT_STATIC_MODE);
    }
}

void highlightEnclosureInclusive() {
    int8_t tempResult = highlightEnclosureHelper();
    if (tempResult) {
        scrollCursorOntoScreen();
        setActivityMode(HIGHLIGHT_STATIC_MODE);
    }
}

void changeLine() {
    addHistoryFrame();
    textPos_t tempFirstTextPos;
    textPos_t tempLastTextPos;
    if (isHighlighting) {
        tempFirstTextPos = *(getFirstHighlightTextPos());
        tempLastTextPos = *(getLastHighlightTextPos());
    } else {
        tempFirstTextPos = cursorTextPos;
        tempLastTextPos = cursorTextPos;
    };
    textLine_t *tempFirstLine = tempFirstTextPos.line;
    textLine_t *tempLastLine = tempLastTextPos.line;
    if (tempFirstLine != tempLastLine) {
        textLine_t *tempLine = getNextTextLine(tempFirstLine);
        while (true) {
            textLine_t *tempNextLine = getNextTextLine(tempLine);
            recordTextLineDeleted(tempLine);
            handleTextLineDeleted(tempLine);
            deleteTextLine(tempLine);
            if (tempLine == tempLastLine) {
                break;
            }
            tempLine = tempNextLine;
        }
    }
    int64_t tempLength = tempFirstLine->textAllocation.length;
    int64_t index = 0;
    while (index < tempLength) {
        int8_t tempCharacter = tempFirstLine->textAllocation.text[index];
        if (tempCharacter != ' ' && tempCharacter != '\t') {
            break;
        }
        index += 1;
    }
    recordTextLineDeleted(tempFirstLine);
    removeTextFromTextAllocation(&(tempFirstLine->textAllocation), index, tempLength - index);
    recordTextLineInserted(tempFirstLine);
    cursorTextPos.line = tempFirstLine;
    setTextPosIndex(&cursorTextPos, cursorTextPos.line->textAllocation.length);
    cursorSnapColumn = cursorTextPos.column;
    int8_t tempResult = scrollCursorOntoScreen();
    if (!tempResult) {
        displayAllTextLines();
    }
    eraseLineNumber();
    displayLineNumber();
    finishCurrentHistoryFrame();
    setActivityMode(TEXT_ENTRY_MODE);
    textBufferIsDirty = true;
}

