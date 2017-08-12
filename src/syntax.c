
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include <dirent.h>
#include "utilities.h"
#include "display.h"
#include "breadtext.h"
#include "syntax.h"

void removeAllSyntax() {
    if (commentPrefix != NULL) {
        free(commentPrefix);
        commentPrefix = NULL;
    }
    if (keywordList != NULL) {
        int32_t index = 0;
        while (index < keywordListLength) {
            free(keywordList[index]);
            index += 1;
        }
        free(keywordList);
    }
}

int8_t syntaxFileMatchesExtension(int8_t *fileName, int8_t *extension) {
    int32_t tempFileNameLength = strlen((char *)fileName);
    int32_t tempExtensionLength = strlen((char *)extension);
    int32_t index = 7;
    while (true) {
        if (index + tempExtensionLength > tempFileNameLength) {
            break;
        }
        if (equalData(fileName + index, extension, tempExtensionLength)) {
            return true;
        }
        while (true) {
            int8_t tempCharacter = fileName[index];
            index += 1;
            if (tempCharacter == '_' || tempCharacter == 0) {
                break;
            }
        }
    }
    return false;
}

void updateSyntaxDefinition() {
    removeAllSyntax();
    if (!shouldHighlightSyntax) {
        redrawEverything();
        return;
    }
    DIR *tempDirectory = opendir((char *)syntaxDirectoryPath);
    if (tempDirectory == NULL) {
        redrawEverything();
        return;
    }
    int8_t *tempExtension = getFileExtension(filePath);
    struct dirent *ent;
    int8_t *tempFileName;
    while (true) {
        ent = readdir(tempDirectory);
        if (ent == NULL) {
            redrawEverything();
            return;
        }
        tempFileName = (int8_t *)ent->d_name;
        int32_t tempLength = strlen((char *)tempFileName);
        if (tempLength >= 8) {
            if (equalData(tempFileName, (int8_t *)"syntax_", 7)) {
                if (syntaxFileMatchesExtension(tempFileName, tempExtension)) {
                    break;
                }
            }
        }
    }
    closedir(tempDirectory);
    // TODO: Process tempFileName.
    
}
