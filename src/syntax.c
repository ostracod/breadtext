
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

int32_t getKeywordAmount(int8_t *line) {
    int32_t output = 0;
    while (true) {
        if (*line == 0) {
            break;
        }
        output += 1;
        line = findWhitespace(line);
        if (*line == 0) {
            break;
        }
        line = skipWhitespace(line);
    }
    return output;
}

void sortKeywordList() {
    // Selection sort.
    // I know it's not the fastest. It doesn't matter in this context.
    int32_t index1 = 0;
    while (index1 < keywordListLength) {
        int32_t tempCandidateIndex = index1;
        int8_t *tempCandidate = keywordList[tempCandidateIndex];
        int32_t index2 = index1 + 1;
        while (index2 < keywordListLength) {
            int8_t *tempKeyword = keywordList[index2];
            if (strcmp((char *)tempKeyword, (char *)tempCandidate) < 0) {
                tempCandidate = tempKeyword;
                tempCandidateIndex = index2;
            }
            index2 += 1;
        }
        keywordList[tempCandidateIndex] = keywordList[index1];
        keywordList[index1] = tempCandidate;
        index1 += 1;
    }
}

void processSyntaxFileDirective(int8_t *line1, int8_t *line2) {
    if (strcmp((char *)line1, "COMMENT") == 0) {
        commentPrefix = malloc(strlen((char *)line2) + 1);
        strcpy((char *)commentPrefix, (char *)line2);
    }
    if (strcmp((char *)line1, "KEYWORDS") == 0) {
        keywordListLength = getKeywordAmount(line2);
        keywordList = malloc(sizeof(int8_t *) * keywordListLength);
        int8_t *tempText = line2;
        int32_t index = 0;
        while (index < keywordListLength) {
            if (*tempText == 0) {
                break;
            }
            int8_t *tempKeyword = tempText;
            tempText = findWhitespace(tempText);
            if (*tempText != 0) {
                *tempText = 0;
                tempText += 1;
                if (*tempText != 0) {
                    tempText = skipWhitespace(tempText);
                }
            }
            int8_t *tempKeyword2 = malloc(strlen((char *)tempKeyword) + 1);
            strcpy((char *)tempKeyword2, (char *)tempKeyword);
            keywordList[index] = tempKeyword2;
            index += 1;
        }
        sortKeywordList();
    }
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
    int32_t tempLength1 = strlen((char *)syntaxDirectoryPath);
    int32_t tempLength2 = strlen((char *)tempFileName);
    closedir(tempDirectory);
    int8_t *tempFilePath = malloc(tempLength1 + 1 + tempLength2 + 1);
    copyData(tempFilePath, syntaxDirectoryPath, tempLength1);
    tempFilePath[tempLength1] = '/';
    copyData(tempFilePath + tempLength1 + 1, tempFileName, tempLength2);
    tempFilePath[tempLength1 + 1 + tempLength2] = 0;
    FILE *tempFile = fopen((char *)tempFilePath, "r");
    if (tempFile != NULL) {
        while (true) {
            int8_t tempShouldBreak = true;
            int8_t *tempText1 = NULL;
            size_t tempSize1 = 0;
            int64_t tempCount1 = getline((char **)&tempText1, &tempSize1, tempFile);
            if (tempCount1 >= 0) {
                int8_t *tempNewline1 = (int8_t *)strchr((char *)tempText1, '\n');
                if (tempNewline1 != NULL) {
                    *tempNewline1 = 0;
                }
                int8_t *tempText2 = NULL;
                size_t tempSize2 = 0;
                int64_t tempCount2 = getline((char **)&tempText2, &tempSize2, tempFile);
                if (tempCount2 >= 0) {
                    int8_t *tempNewline2 = (int8_t *)strchr((char *)tempText2, '\n');
                    if (tempNewline2 != NULL) {
                        *tempNewline2 = 0;
                    }
                    processSyntaxFileDirective(tempText1, tempText2);
                    tempShouldBreak = false;
                    free(tempText2);
                }
                free(tempText1);
            }
            if (tempShouldBreak) {
                break;
            }
        }
        fclose(tempFile);
    }
    free(tempFilePath);
}

void generateSyntaxHighlighting(textAllocation_t *allocation) {
    if (allocation->syntaxHighlighting != NULL) {
        free(allocation->syntaxHighlighting);
    }
    if (!shouldHighlightSyntax || allocation->length <= 0) {
        allocation->syntaxHighlighting = NULL;
        return;
    }
    int8_t *tempHighlighting = malloc(allocation->length);
    int64_t index = 0;
    while (index < allocation->length) {
        tempHighlighting[index] = index % 4;
        index += 1;
    }
    allocation->syntaxHighlighting = tempHighlighting;
}