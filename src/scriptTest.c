
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>
#include "utilities.h"
#include "breadtext.h"
#include "vector.h"
#include "script.h"
#include "scriptTest.h"

#define SCRIPT_TEST_PHASE_NONE 0
#define SCRIPT_TEST_PHASE_LOAD 1
#define SCRIPT_TEST_PHASE_RUN 2
#define SCRIPT_TEST_PHASE_ASSERT_OUTPUT 3

FILE *scriptTestResultFile;
int32_t failedTestCount;
int32_t testCount;
int32_t scriptTestPhase;
vector_t scriptContent;
int64_t scriptTestLogIndex;

int8_t processScriptTestCommand(int8_t *command) {
    if (scriptTestPhase == SCRIPT_TEST_PHASE_NONE) {
        int8_t *tempTermList[50];
        int32_t tempTermListLength;
        parseSpaceSeperatedTerms(tempTermList, &tempTermListLength, command);
        if (tempTermListLength <= 0) {
            fprintf(scriptTestResultFile, "ERROR: Invalid command.\n");
            fflush(scriptTestResultFile);
            return false;
        }
        if (strcmp((char *)(tempTermList[0]), "START_TEST") == 0) {
            if (tempTermListLength != 2) {
                fprintf(scriptTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", tempTermList[0]);
                fflush(scriptTestResultFile);
                return false;
            }
            resetApplication();
            scriptTestPhase = SCRIPT_TEST_PHASE_LOAD;
            createEmptyVector(&scriptContent, 1);
            fprintf(scriptTestResultFile, "RUNNING TEST %s\n", tempTermList[1]);
            fflush(scriptTestResultFile);
            testCount += 1;
            return true;
        }
    } else if (scriptTestPhase == SCRIPT_TEST_PHASE_LOAD) {
        if (strcmp((char *)command, "RUN_SCRIPT") == 0) {
            int8_t tempCharacter = 0;
            pushVectorElement(&scriptContent, &tempCharacter);
            runScriptAsText(scriptContent.data);
            scriptTestPhase = SCRIPT_TEST_PHASE_RUN;
            scriptTestLogIndex = 0;
            return true;
        }
        pushVectorElementArray(&scriptContent, command, strlen((char *)command));
        int8_t tempCharacter = '\n';
        pushVectorElement(&scriptContent, &tempCharacter);
        return true;
    } else if (scriptTestPhase == SCRIPT_TEST_PHASE_RUN) {
        int8_t *tempTermList[50];
        int32_t tempTermListLength;
        parseSpaceSeperatedTerms(tempTermList, &tempTermListLength, command);
        if (tempTermListLength <= 0) {
            fprintf(scriptTestResultFile, "ERROR: Invalid command.\n");
            fflush(scriptTestResultFile);
            return false;
        }
        if (strcmp((char *)(tempTermList[0]), "ASSERT_OUTPUT") == 0) {
            if (tempTermListLength != 1) {
                fprintf(scriptTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", tempTermList[0]);
                fflush(scriptTestResultFile);
                return false;
            }
            scriptTestPhase = SCRIPT_TEST_PHASE_ASSERT_OUTPUT;
            return true;
        }
        if (strcmp((char *)(tempTermList[0]), "ASSERT_ERROR") == 0) {
            if (tempTermListLength != 1) {
                fprintf(scriptTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", tempTermList[0]);
                fflush(scriptTestResultFile);
                return false;
            }
            if (!scriptHasError) {
                failedTestCount += 1;
                fprintf(scriptTestResultFile, "ASSERTION FAILURE\nMissing script error.\n");
                fflush(scriptTestResultFile);
                return false;
            }
            return true;
        }
        if (strcmp((char *)(tempTermList[0]), "ASSERT_NO_ERROR") == 0) {
            if (tempTermListLength != 1) {
                fprintf(scriptTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", tempTermList[0]);
                fflush(scriptTestResultFile);
                return false;
            }
            if (scriptHasError) {
                failedTestCount += 1;
                fprintf(scriptTestResultFile, "ASSERTION FAILURE\nScript has error.\n");
                fflush(scriptTestResultFile);
                return false;
            }
            return true;
        }
        if (strcmp((char *)(tempTermList[0]), "PRESS_KEY") == 0) {
            if (tempTermListLength < 2) {
                fprintf(scriptTestResultFile, "ERROR: Wrong number of arguments.\n%s\n", tempTermList[0]);
                fflush(scriptTestResultFile);
                return false;
            }
            int32_t index = 1;
            while (index < tempTermListLength) {
                int8_t *tempName = tempTermList[index];
                int32_t tempKey;
                if (strcmp((char *)tempName, "NEWLINE") == 0) {
                    tempKey = '\n';
                } else if (strcmp((char *)tempName, "ESC") == 0) {
                    tempKey = 27;
                } else {
                    tempKey = tempName[0];
                }
                handleKey(tempKey, true, true, true);
                refresh();
                sleepMilliseconds(1);
                index += 1;
            }
            return true;
        }
    } else if (scriptTestPhase == SCRIPT_TEST_PHASE_ASSERT_OUTPUT) {
        if (strcmp((char *)command, "END_TEST") == 0) {
            scriptTestPhase = SCRIPT_TEST_PHASE_NONE;
            if (scriptTestLogIndex < scriptTestLogMessageList.length) {
                failedTestCount += 1;
                fprintf(scriptTestResultFile, "ASSERTION FAILURE\nToo much output.\n");
                fflush(scriptTestResultFile);
                return false;
            }
            return true;
        }
        if (scriptTestLogIndex >= scriptTestLogMessageList.length) {
            failedTestCount += 1;
            fprintf(scriptTestResultFile, "ASSERTION FAILURE\nNot enough output.\n");
            fflush(scriptTestResultFile);
            return false;
        }
        int8_t *tempMessage;
        getVectorElement(&tempMessage, &scriptTestLogMessageList, scriptTestLogIndex);
        scriptTestLogIndex += 1;
        if (strcmp((char *)command, (char *)tempMessage) != 0) {
            failedTestCount += 1;
            fprintf(scriptTestResultFile, "ASSERTION FAILURE\nExpected: %s\nFound: %s\n", (char *)command, (char *)tempMessage);
            fflush(scriptTestResultFile);
            return false;
        } else {
            fprintf(scriptTestResultFile, "%s\n", (char *)command);
            fflush(scriptTestResultFile);
        }
        return true;
    }
    fprintf(scriptTestResultFile, "ERROR: Invalid command.\n%s\n", (char *)command);
    fflush(scriptTestResultFile);
    return false;
}

int8_t runScriptTest() {
    int8_t output = true;
    failedTestCount = 0;
    testCount = 0;
    scriptTestPhase = SCRIPT_TEST_PHASE_NONE;
    FILE *tempFile = fopen((char *)scriptTestDefinitionPath, "r");
    scriptTestResultFile = fopen((char *)scriptTestResultPath, "w");
    if (tempFile == NULL) {
        fprintf(scriptTestResultFile, "ERROR: Could not open test definition file.\n");
        fflush(scriptTestResultFile);
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
            int8_t tempResult = processScriptTestCommand(tempText);
            if (!tempResult) {
                output = false;
            }
        }
        free(tempText);
    }
    fprintf(scriptTestResultFile, "====================================\n");
    fprintf(scriptTestResultFile, "TEST COUNT: %d\n", testCount);
    fprintf(scriptTestResultFile, "FAILED TESTS: %d\n", failedTestCount);
    if (output) {
        fprintf(scriptTestResultFile, "ALL TESTS PASSED\n");
    } else {
        if (failedTestCount <= 0) {
            fprintf(scriptTestResultFile, "TESTING ERROR, BUT NO FAILED TESTS\nCHECK YOUR SETUP\n");
        } else {
            fprintf(scriptTestResultFile, "TESTING FAILED\n");
        }
    }
    fflush(scriptTestResultFile);
    fclose(tempFile);
    fclose(scriptTestResultFile);
    return output;
}



