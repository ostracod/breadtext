
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include <inttypes.h>
#include "utilities.h"
#include "breadtext.h"
#include "textLine.h"
#include "textAllocation.h"
#include "textCommand.h"
#include "display.h"
#include "systematicTest.h"

namedKey_t namedKeySet[] = {
    {27, (int8_t *)"ESC"},
    {KEY_LEFT, (int8_t *)"LEFT"},
    {KEY_RIGHT, (int8_t *)"RIGHT"},
    {KEY_UP, (int8_t *)"UP"},
    {KEY_DOWN, (int8_t *)"DOWN"},
    {' ', (int8_t *)"SPACE"},
    {'\n', (int8_t *)"NEWLINE"},
    {127, (int8_t *)"BACKSPACE"},
    {'\t', (int8_t *)"TAB"},
    {KEY_BTAB, (int8_t *)"BTAB"}
};

FILE *systematicTestResultFile;
int32_t failedAssertionCount;
int32_t assertionCount;
int32_t testCount;
int32_t initialDocumentIsEmpty;

int32_t convertNameToKey(int8_t *name) {
    int index = 0;
    while (index < sizeof(namedKeySet) / sizeof(*namedKeySet)) {
        namedKey_t *tempNamedKey = namedKeySet + index;
        if (strcmp((char *)name, (char *)(tempNamedKey->name)) == 0) {
            return tempNamedKey->key;
        }
        index += 1;
    }
    return -1;
}

void printArgCountError(char *commandName) {
    fprintf(systematicTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", commandName);
    fflush(systematicTestResultFile);
}

int8_t processSystematicTestCommand(int8_t *command) {
    int8_t *tempTermList[50];
    int32_t tempTermListLength;
    parseSpaceSeperatedTerms(tempTermList, &tempTermListLength, command);
    if (tempTermListLength <= 0) {
        fprintf(systematicTestResultFile, "ERROR: Invalid command.\n");
        fflush(systematicTestResultFile);
        return false;
    }
    char *commandName = (char *)(tempTermList[0]);
    if (strcmp(commandName, "TEST") == 0) {
        if (tempTermListLength != 2) {
            printArgCountError(commandName);
            return false;
        }
        resetApplication();
        initialDocumentIsEmpty = true;
        fprintf(systematicTestResultFile, "RUNNING TEST %s\n", tempTermList[1]);
        fflush(systematicTestResultFile);
        testCount += 1;
        return true;
    }
    if (strcmp(commandName, "PRESS_KEY") == 0) {
        if (tempTermListLength < 2) {
            printArgCountError(commandName);
            return false;
        }
        int32_t index = 1;
        while (index < tempTermListLength) {
            int8_t *tempName = tempTermList[index];
            int32_t tempKey;
            if (strlen((char *)tempName) == 1) {
                tempKey = tempName[0];
            } else {
                tempKey = convertNameToKey(tempName);
                if (tempKey < 0) {
                    fprintf(systematicTestResultFile, "ERROR: Bad key name.\n%s\n", tempName);
                    fflush(systematicTestResultFile);
                    return false;
                }
            }
            handleKey(tempKey, false, false, true);
            refresh();
            sleepMilliseconds(1);
            index += 1;
        }
        return true;
    }
    if (strcmp(commandName, "ASSERT_LINE_COUNT") == 0) {
        if (tempTermListLength != 2) {
            printArgCountError(commandName);
            return false;
        }
        int64_t tempLineCount1;
        sscanf((char *)(tempTermList[1]), "%" PRId64, &tempLineCount1);
        textLine_t *tempLine = getRightmostTextLine(rootTextLine);
        int64_t tempLineCount2 = getTextLineNumber(tempLine);
        assertionCount += 1;
        if (tempLineCount1 != tempLineCount2) {
            failedAssertionCount += 1;
            fprintf(systematicTestResultFile, "LINE COUNT ASSERTION FAILURE\nExpected: %" PRId64 "\nFound: %" PRId64 "\n", tempLineCount1, tempLineCount2);
            fflush(systematicTestResultFile);
            return false;
        }
        return true;
    }
    if (strcmp(commandName, "ASSERT_LINE") == 0) {
        if (tempTermListLength != 3) {
            printArgCountError(commandName);
            return false;
        }
        int64_t tempLineNumber;
        sscanf((char *)(tempTermList[1]), "%" PRId64, &tempLineNumber);
        textLine_t *tempLine = getTextLineByNumber(tempLineNumber);
        assertionCount += 1;
        if (tempLine == NULL) {
            fprintf(systematicTestResultFile, "LINE ASSERTION FAILURE\nMissing line number %" PRId64 "\n", tempLineNumber);
            fflush(systematicTestResultFile);
            return false;
        }
        int8_t tempHasFailed = false;
        if (strlen((char *)(tempTermList[2])) != tempLine->textAllocation.length) {
            tempHasFailed = true;
        } else {
            if (!equalData(tempTermList[2], tempLine->textAllocation.text, tempLine->textAllocation.length)) {
                tempHasFailed = true;
            }
        }
        if (tempHasFailed) {
            failedAssertionCount += 1;
            int8_t tempText[tempLine->textAllocation.length + 1];
            copyData(tempText, tempLine->textAllocation.text, tempLine->textAllocation.length + 1);
            tempText[tempLine->textAllocation.length] = 0;
            fprintf(systematicTestResultFile, "LINE ASSERTION FAILURE (line %" PRId64 ")\nExpected: %s\nFound: %s\n", tempLineNumber, tempTermList[2], tempText);
            fflush(systematicTestResultFile);
            return false;
        }
        return true;
    }
    if (strcmp(commandName, "ASSERT_POS") == 0) {
        if (tempTermListLength != 3) {
            printArgCountError(commandName);
            return false;
        }
        int64_t tempLineNumber1;
        int64_t index1;
        sscanf((char *)(tempTermList[1]), "%" PRId64, &tempLineNumber1);
        sscanf((char *)(tempTermList[2]), "%" PRId64, &index1);
        int64_t tempLineNumber2 = getTextLineNumber(cursorTextPos.line);
        int64_t index2 = getTextPosIndex(&cursorTextPos);
        assertionCount += 1;
        if (tempLineNumber1 != tempLineNumber2 || index1 != index2) {
            failedAssertionCount += 1;
            fprintf(systematicTestResultFile, "POS ASSERTION FAILURE\nExpected: %" PRId64 " %" PRId64 "\nFound: %" PRId64 " %" PRId64 "\n", tempLineNumber1, index1, tempLineNumber2, index2);
            fflush(systematicTestResultFile);
            return false;
        }
        return true;
    }
    if (strcmp(commandName, "ASSERT_HIGHLIGHT_POS") == 0) {
        if (tempTermListLength != 3) {
            printArgCountError(commandName);
            return false;
        }
        int64_t tempLineNumber1;
        int64_t index1;
        sscanf((char *)(tempTermList[1]), "%" PRId64, &tempLineNumber1);
        sscanf((char *)(tempTermList[2]), "%" PRId64, &index1);
        int64_t tempLineNumber2 = getTextLineNumber(highlightTextPos.line);
        int64_t index2 = getTextPosIndex(&highlightTextPos);
        assertionCount += 1;
        if (tempLineNumber1 != tempLineNumber2 || index1 != index2) {
            failedAssertionCount += 1;
            fprintf(systematicTestResultFile, "HIGHLIGHT POS ASSERTION FAILURE\nExpected: %" PRId64 " %" PRId64 "\nFound: %" PRId64 " %" PRId64 "\n", tempLineNumber1, index1, tempLineNumber2, index2);
            fflush(systematicTestResultFile);
            return false;
        }
        return true;
    }
    if (strcmp(commandName, "ADD_LINE") == 0) {
        if (tempTermListLength != 2) {
            printArgCountError(commandName);
            return false;
        }
        textLine_t *tempLine1 = createEmptyTextLine();
        insertTextIntoTextAllocation(&(tempLine1->textAllocation), 0, tempTermList[1], strlen((char *)(tempTermList[1])));
        if (initialDocumentIsEmpty) {
            textLine_t *tempLine2 = rootTextLine;
            insertTextLineRight(tempLine2, tempLine1);
            cursorTextPos.line = tempLine1;
            setTextPosIndex(&cursorTextPos, 0);
            handleTextLineDeleted(tempLine2);
            deleteTextLine(tempLine2);
            initialDocumentIsEmpty = false;
        } else {
            textLine_t *tempLine2 = getRightmostTextLine(rootTextLine);
            insertTextLineRight(tempLine2, tempLine1);
        }
        redrawEverything();
        return true;
    }
    if (strcmp(commandName, "SET_POS") == 0) {
        if (tempTermListLength != 3) {
            printArgCountError(commandName);
            return false;
        }
        int64_t tempLineNumber;
        int64_t index;
        sscanf((char *)(tempTermList[1]), "%" PRId64, &tempLineNumber);
        sscanf((char *)(tempTermList[2]), "%" PRId64, &index);
        textLine_t *tempLine = getTextLineByNumber(tempLineNumber);
        if (tempLine == NULL) {
            fprintf(systematicTestResultFile, "ERROR: Missing line number %" PRId64 ".\n", tempLineNumber);
            fflush(systematicTestResultFile);
            return false;
        }
        if (index > tempLine->textAllocation.length) {
            fprintf(systematicTestResultFile, "ERROR: Line number %" PRId64 " is too short.\n", tempLineNumber);
            fflush(systematicTestResultFile);
            return false;
        }
        cursorTextPos.line = tempLine;
        setTextPosIndex(&cursorTextPos, index);
        redrawEverything();
        return true;
    }
    if (strcmp(commandName, "TEXT_COMMAND") == 0) {
        if (tempTermListLength != 2) {
            printArgCountError(commandName);
            return false;
        }
        setActivityMode(TEXT_COMMAND_MODE);
        strcpy((char *)textCommandBuffer, (char *)(tempTermList[1]));
        executeTextCommand();
        return true;
    }
    if (strcmp(commandName, "USE_SYSTEM_CLIPBOARD") == 0) {
        if (tempTermListLength != 2) {
            printArgCountError(commandName);
            return false;
        }
        shouldUseSystemClipboard = (tempTermList[1][0] == '1');
        return true;
    }
    fprintf(systematicTestResultFile, "ERROR: Invalid command.\n%s\n", commandName);
    fflush(systematicTestResultFile);
    return false;
}

int8_t runSystematicTest() {
    int8_t output = true;
    failedAssertionCount = 0;
    assertionCount = 0;
    testCount = 0;
    FILE *tempFile = fopen((char *)systematicTestDefinitionPath, "r");
    systematicTestResultFile = fopen((char *)systematicTestResultPath, "w");
    if (tempFile == NULL) {
        fprintf(systematicTestResultFile, "ERROR: Could not open test definition file.\n");
        fflush(systematicTestResultFile);
        return false;
    }
    while (true) {
        int8_t *tempText = NULL;
        size_t tempSize = 0;
        int64_t tempCount = getline((char **)&tempText, &tempSize, tempFile);
        if (tempCount < 0) {
            break;
        }
        int64_t tempLength = strlen((char *)tempText);
        while (tempLength > 0) {
            int64_t index = tempLength - 1;
            if (tempText[index] == '\n') {
                tempText[index] = 0;
                tempLength = index;
            } else {
                break;
            }
        }
        if (tempLength > 0) {
            int8_t tempResult = processSystematicTestCommand(tempText);
            if (!tempResult) {
                output = false;
            }
        }
        free(tempText);
    }
    fprintf(systematicTestResultFile, "====================================\n");
    fprintf(systematicTestResultFile, "TEST COUNT: %d\n", testCount);
    fprintf(systematicTestResultFile, "FAILED ASSERTIONS: %d / %d\n", failedAssertionCount, assertionCount);
    if (output) {
        fprintf(systematicTestResultFile, "ALL TESTS PASSED\n");
    } else {
        if (failedAssertionCount <= 0) {
            fprintf(systematicTestResultFile, "TESTING ERROR, BUT NO FAILED ASSERTIONS\nCHECK YOUR SETUP\n");
        } else {
            fprintf(systematicTestResultFile, "TESTING FAILED\n");
        }
    }
    fflush(systematicTestResultFile);
    fclose(tempFile);
    fclose(systematicTestResultFile);
    return output;
}

