
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>
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
    tempText[0] = '"';
    strcpy((char *)tempText + 1, (char *)path);
    tempText[tempLength + 1] = '"';
    tempText[tempLength + 2] = 0;
    wordexp_t expResult;
    wordexp((char *)tempText, &expResult, 0);
    realpath(expResult.we_wordv[0], (char *)tempText);
    wordfree(&expResult);
    int8_t *output = malloc(strlen((char *)tempText) + 1);
    strcpy((char *)output, (char *)tempText);
    return output;
}

void systemCopyClipboardFile() {
    if (applicationPlatform == PLATFORM_MAC) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "cat \"%s\" | pbcopy", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
    if (applicationPlatform == PLATFORM_LINUX) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
}

void systemPasteClipboardFile() {
    if (applicationPlatform == PLATFORM_MAC) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "pbpaste > \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
    if (applicationPlatform == PLATFORM_LINUX) {
        int8_t tempCommand[5000];
        sprintf((char *)tempCommand, "xclip -selection clipboard -o > \"%s\"", (char *)clipboardFilePath);
        system((char *)tempCommand);
    }
}
