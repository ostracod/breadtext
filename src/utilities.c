
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <wordexp.h>
#include <unistd.h>
#include "breadtext.h"
#include "utilities.h"

void copyData(int8_t *destination, int8_t *source, int64_t amount) {
    if (destination < source) {
        int64_t index = 0;
        while (index < amount) {
            destination[index] = source[index];
            index += 1;
        }
    } else {
        int64_t index = amount - 1;
        while (index >= 0) {
            destination[index] = source[index];
            index -= 1;
        }
    }
}

int8_t equalData(int8_t *source1, int8_t *source2, int64_t amount) {
    int64_t index = 0;
    while (index < amount) {
        if (source1[index] != source2[index]) {
            return false;
        }
        index += 1;
    }
    return true;
}

int8_t isWhitespace(int8_t character) {
    return (character == ' ' || character == '\n' || character == '\t' || character == '\r');
}

int8_t *findWhitespace(int8_t *text) {
    while (true) {
        int8_t tempCharacter = *text;
        if (tempCharacter == 0) {
            break;
        }
        if (isWhitespace(tempCharacter)) {
            break;
        }
        text += 1;
    }
    return text;
}

int8_t *skipWhitespace(int8_t *text) {
    while (true) {
        int8_t tempCharacter = *text;
        if (tempCharacter == 0) {
            break;
        }
        if (!isWhitespace(tempCharacter)) {
            break;
        }
        text += 1;
    }
    return text;
}

int64_t removeBadCharacters(int8_t *text, int8_t *containsNewline) {
    *containsNewline = false;
    int64_t tempIndex1 = 0;
    int64_t tempIndex2 = 0;
    while (true) {
        int8_t tempCharacter = text[tempIndex1];
        if (tempCharacter == 0) {
            text[tempIndex2] = 0;
            break;
        }
        if (tempCharacter == '\n') {
            *containsNewline = true;
        }
        if ((tempCharacter >= 32 && tempCharacter <= 126) || tempCharacter == '\t') {
            text[tempIndex2] = tempCharacter;
            tempIndex2 += 1;
        }
        tempIndex1 += 1;
    }
    return tempIndex2;
}

void convertTabsToSpaces(int8_t *text) {
    int64_t index = 0;
    while (true) {
        int8_t tempCharacter = text[index];
        if (tempCharacter == 0) {
            break;
        }
        if (tempCharacter == '\t') {
            text[index] = ' ';
        }
        index += 1;
    }
}

int8_t isWordCharacter(int8_t tempCharacter) {
    return ((tempCharacter >= 'a' && tempCharacter <= 'z')
         || (tempCharacter >= 'A' && tempCharacter <= 'Z')
         || (tempCharacter >= '0' && tempCharacter <= '9')
         || tempCharacter == '_');
}

int8_t *mallocRealpath(int8_t *path) {
    int8_t tempText[50000];
    int64_t tempLength = strlen((char *)path);
    int64_t tempIndex1 = 0;
    int64_t tempIndex2 = 0;
    while (tempIndex1 < tempLength) {
        int8_t tempCharacter = path[tempIndex1];
        if (tempCharacter == ' ') {
            tempText[tempIndex2] = '\\';
            tempIndex2 += 1;
        }
        tempText[tempIndex2] = tempCharacter;
        tempIndex2 += 1;
        tempIndex1 += 1;
    }
    tempText[tempIndex2] = 0;
    wordexp_t expResult;
    wordexp((char *)tempText, &expResult, 0);
    realpath(expResult.we_wordv[0], (char *)tempText);
    wordfree(&expResult);
    int8_t *output = malloc(strlen((char *)tempText) + 1);
    strcpy((char *)output, (char *)tempText);
    return output;
}

void systemCopyClipboardFile() {
    if (applicationPlatform == PLATFORM_LINUX || shouldUseXclip) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard \"%s\" > /dev/null 2>&1", (char *)clipboardFilePath);
        system((char *)tempCommand);
    } else {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "cat \"%s\" | pbcopy > /dev/null 2>&1", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
}

void systemPasteClipboardFile() {
    int8_t tempCommand[5000];
    sprintf((char *)tempCommand, "touch \"%s\" > /dev/null 2>&1", (char *)clipboardFilePath);
    system((char *)tempCommand);
    if (access((char *)clipboardFilePath, F_OK) == -1) {
        return;
    }
    if (applicationPlatform == PLATFORM_LINUX || shouldUseXclip) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard -o > \"%s\" 2> /dev/null", (char *)clipboardFilePath);
        system((char *)tempCommand);
    } else {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "pbpaste > \"%s\" 2> /dev/null", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
}

void addToHexadecimalText(int8_t *text, int64_t offset) {
    int64_t tempLength = strlen((char *)text);
    uint64_t tempNumber = 0;
    int64_t tempOffset = 0;
    int64_t index = tempLength - 1;
    while (index >= 0) {
        int8_t tempCharacter = text[index];
        if (tempCharacter >= '0' && tempCharacter <= '9') {
            tempNumber |= (uint64_t)(tempCharacter - '0') << tempOffset;
        }
        if (tempCharacter >= 'A' && tempCharacter <= 'F') {
            tempNumber |= (uint64_t)(tempCharacter - 'A' + 10) << tempOffset;
        }
        if (tempCharacter >= 'a' && tempCharacter <= 'f') {
            tempNumber |= (uint64_t)(tempCharacter - 'a' + 10) << tempOffset;
        }
        tempOffset += 4;
        index -= 1;
    }
    tempNumber += offset;
    tempOffset = 0;
    index = tempLength - 1;
    while (index >= 0) {
        int8_t tempValue = (int8_t)((((uint64_t)tempNumber) & ((uint64_t)0x0F << tempOffset)) >> tempOffset);
        if (tempValue >= 0 && tempValue <= 9) {
            text[index] = tempValue + '0';
        }
        if (tempValue >= 10 && tempValue <= 15) {
            text[index] = tempValue + 'A' - 10;
        }
        tempOffset += 4;
        index -= 1;
    }
}

void sleepMilliseconds(int32_t milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int8_t *getFileExtension(int8_t *path) {
    int32_t index = strlen((char *)path) - 1;
    while (index >= 0) {
        int8_t tempCharacter = path[index];
        if (tempCharacter == '/') {
            return NULL;
        }
        if (tempCharacter == '.') {
            return path + index + 1;
        }
        index -= 1;
    }
    return NULL;
}

void parseSpaceSeperatedTerms(int8_t **termList, int32_t *termListLength, int8_t *text) {
    int8_t tempTermIndex = 0;
    int8_t tempIsInQuotes = false;
    int8_t *tempText1 = text;
    int8_t *tempText2 = text;
    int8_t tempIsStartOfTerm = true;
    while (true) {
        int8_t tempCharacter = *tempText1;
        tempText1 += 1;
        if (tempCharacter == 0) {
            *tempText2 = 0;
            tempText2 += 1;
            break;
        }
        int8_t tempIsWhitespace = isWhitespace(tempCharacter);
        if (tempCharacter == '\\') {
            int8_t tempCharacter = *tempText1;
            if (tempCharacter == 't') {
                tempCharacter = '\t';
            }
            tempText1 += 1;
            *tempText2 = tempCharacter;
            tempText2 += 1;
        } else if (tempIsStartOfTerm && !tempIsWhitespace) {
            if (tempCharacter == '"') {
                termList[tempTermIndex] = tempText2;
                tempTermIndex += 1;
                tempIsInQuotes = true;
            } else {
                termList[tempTermIndex] = tempText2;
                *tempText2 = tempCharacter;
                tempText2 += 1;
                tempTermIndex += 1;
            }
            tempIsStartOfTerm = false;
        } else {
            if (tempCharacter == '"') {
                *tempText2 = 0;
                tempText2 += 1;
                tempIsInQuotes = false;
                tempIsStartOfTerm = true;
            } else if (!tempIsWhitespace || tempIsInQuotes) {
                *tempText2 = tempCharacter;
                tempText2 += 1;
            } else {
                *tempText2 = 0;
                tempText2 += 1;
                tempText1 = skipWhitespace(tempText1);
                tempIsStartOfTerm = true;
            }
        }
    }
    *termListLength = tempTermIndex;
}

