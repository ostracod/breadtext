
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <curses.h>
#include "utilities.h"
#include "vector.h"
#include "scriptValue.h"
#include "script.h"
#include "display.h"
#include "selection.h"
#include "motion.h"
#include "textCommand.h"

#define DESTINATION_TYPE_NONE 0
#define DESTINATION_TYPE_VALUE 1
#define DESTINATION_TYPE_CHARACTER 2

typedef struct expressionResult {
    scriptValue_t value;
    int8_t destinationType;
    void *destination;
} expressionResult_t;

vector_t scriptBodyList;
scriptScope_t *globalScriptScope;
scriptScope_t *localScriptScope;
vector_t scriptBranchStack;
int8_t scriptHasError = false;
int8_t scriptErrorMessage[1000];
scriptBodyLine_t scriptErrorLine;
int8_t scriptErrorHasLine = false;
vector_t keyBindingList;
vector_t keyMappingList;
vector_t commandBindingList;
int32_t garbageCollectionDelay = 0;

void initializeScriptingEnvironment() {
    firstHeapValue = NULL;
    createEmptyVector(&scriptBodyList, sizeof(scriptBody_t));
    createEmptyVector(&scriptBranchStack, sizeof(scriptBranch_t));
    createEmptyVector(&keyBindingList, sizeof(keyBinding_t));
    createEmptyVector(&keyMappingList, sizeof(keyMapping_t));
    createEmptyVector(&commandBindingList, sizeof(commandBinding_t));
}

void garbageCollectScriptHeapValues() {
    // Mark all heap values for deletion.
    scriptHeapValue_t *tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        tempHeapValue->isMarked = true;
        tempHeapValue = tempHeapValue->next;
    }
    // Unmark all reachable values.
    int64_t index = 0;
    while (index < scriptBodyList.length) {
        scriptBody_t *tempScriptBody = findVectorElement(&scriptBodyList, index);
        unmarkScriptBody(tempScriptBody);
        index += 1;
    }
    // Unmark all locked values.
    tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        if (tempHeapValue->lockDepth > 0) {
            unmarkScriptHeapValue(tempHeapValue);
        }
        tempHeapValue = tempHeapValue->next;
    }
    // Delete all values which are still marked.
    tempHeapValue = firstHeapValue;
    while (tempHeapValue != NULL) {
        scriptHeapValue_t *tempNextHeapValue = tempHeapValue->next;
        if (tempHeapValue->isMarked) {
            deleteScriptHeapValue(tempHeapValue);
        }
        tempHeapValue = tempNextHeapValue;
    }
}

void reportScriptErrorWithoutLine(int8_t *message) {
    scriptHasError = true;
    strcpy((char *)scriptErrorMessage, (char *)message);
}

void reportScriptError(int8_t *message, scriptBodyLine_t *line) {
    reportScriptErrorWithoutLine(message);
    scriptErrorLine = *line;
    scriptErrorHasLine = true;
}

int8_t characterIsEndOfScriptLine(int8_t character) {
    return (character == '\n' || character == 0 || character == '#');
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence);
scriptValue_t evaluateScriptBody(scriptBodyLine_t *scriptBodyLine);
scriptBody_t *importScript(int8_t *path);

void getScriptBodyValueList(vector_t *destination, scriptBodyPos_t *scriptBodyPos, int8_t endCharacter) {
    createEmptyVector(destination, sizeof(scriptValue_t));
    while (true) {
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (characterIsEndOfScriptLine(tempCharacter)) {
            reportScriptError((int8_t *)"Unexpected end of expression list.", scriptBodyPos->scriptBodyLine);
            return;
        }
        if (tempCharacter == endCharacter) {
            scriptBodyPos->index += 1;
            break;
        }
        expressionResult_t tempResult = evaluateExpression(scriptBodyPos, 99);
        if (scriptHasError) {
            return;
        }
        pushVectorElement(destination, &(tempResult.value));
        scriptBodyPosSkipWhitespace(scriptBodyPos);
        tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
        if (tempCharacter == ',') {
            scriptBodyPos->index += 1;
        } else if (tempCharacter == endCharacter) {
            scriptBodyPos->index += 1;
            break;
        } else {
            reportScriptError((int8_t *)"Bad expression list.", scriptBodyPos->scriptBodyLine);
            return;
        }
    }
}

keyMapping_t *findKeyMapping(int32_t oldKey, int32_t mode) {
    int64_t index = 0;
    while (index < keyMappingList.length) {
        keyMapping_t *tempKeyMapping = findVectorElement(&keyMappingList, index);
        if (mode == tempKeyMapping->mode && oldKey == tempKeyMapping->oldKey) {
            return tempKeyMapping;
        }
        index += 1;
    }
    return NULL;
}

commandBinding_t *findCommandBinding(int8_t *commandName) {
    int64_t index = 0;
    while (index < commandBindingList.length) {
        commandBinding_t *tempCommandBinding = findVectorElement(&commandBindingList, index);
        if (strcmp((char *)(tempCommandBinding->commandName), (char *)commandName) == 0) {
            return tempCommandBinding;
        }
        index += 1;
    }
    return NULL;
}

scriptValue_t invokeFunction(scriptValue_t function, vector_t *argumentList) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    int32_t tempArgumentCount = argumentList->length;
    if (function.type == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION) {
        scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(function.data);
        scriptCustomFunction_t *tempFunction = *(scriptCustomFunction_t **)&(tempHeapValue->data);
        scriptBodyLine_t tempLine = tempFunction->scriptBodyLine;
        scriptBodyPos_t tempScriptBodyPos;
        tempScriptBodyPos.scriptBodyLine = &tempLine;
        tempScriptBodyPos.index = tempLine.index;
        while (true) {
            int8_t tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            tempScriptBodyPos.index += 1;
            if (characterIsEndOfScriptLine(tempCharacter)) {
                reportScriptError((int8_t *)"Invalid function declaration.", &tempLine);
                return output;
            }
            if (tempCharacter == '(') {
                break;
            }
        }
        scriptScope_t tempNewScope = createEmptyScriptScope();
        scriptScope_t *tempScope = scriptBodyAddScope(tempLine.scriptBody, tempNewScope);
        int64_t index = 0;
        while (true) {
            scriptBodyPosSkipWhitespace(&tempScriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (characterIsEndOfScriptLine(tempCharacter)) {
                reportScriptError((int8_t *)"Unexpected end of argument list.", &tempLine);
                return output;
            }
            if (tempCharacter == ')') {
                break;
            }
            if (index >= argumentList->length) {
                reportScriptErrorWithoutLine((int8_t *)"Too many arguments.");
                return output;
            }
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                reportScriptError((int8_t *)"Bad argument name.", &tempLine);
                return output;
            }
            scriptBodyPos_t tempEndScriptBodyPos;
            tempEndScriptBodyPos = tempScriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempEndScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(&tempScriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(&tempScriptBodyPos, &tempEndScriptBodyPos);
            int8_t tempName[tempLength + 1];
            copyData(tempName, tempText, tempLength);
            tempName[tempLength] = 0;
            scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
            scriptVariable_t *tempVariable = scriptScopeAddVariable(tempScope, tempNewVariable);
            getVectorElement(&(tempVariable->value), argumentList, index);
            index += 1;
            tempScriptBodyPos = tempEndScriptBodyPos;
            scriptBodyPosSkipWhitespace(&tempScriptBodyPos);
            tempCharacter = scriptBodyPosGetCharacter(&tempScriptBodyPos);
            if (tempCharacter == ',') {
                tempScriptBodyPos.index += 1;
            } else if (tempCharacter == ')') {
                break;
            } else {
                reportScriptError((int8_t *)"Bad argument list.", &tempLine);
                return output;
            }
        }
        if (index < argumentList->length) {
            reportScriptErrorWithoutLine((int8_t *)"Not enough arguments.");
            return output;
        }
        scriptBranch_t tempBranch;
        tempBranch.type = SCRIPT_BRANCH_TYPE_FUNCTION;
        tempBranch.shouldIgnore = false;
        tempBranch.line = tempLine;
        pushVectorElement(&scriptBranchStack, &tempBranch);
        int8_t tempResult = seekNextScriptBodyLine(&tempLine);
        if (!tempResult) {
            reportScriptError((int8_t *)"Unexpected end of script", &tempLine);
            return output;
        }
        return evaluateScriptBody(&tempLine);
    } else if (function.type == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION) {
        scriptBuiltInFunction_t *tempFunction = *(scriptBuiltInFunction_t **)&(function.data);
        if (tempArgumentCount != tempFunction->argumentAmount) {
            if (tempFunction->argumentAmount == 0) {
                reportScriptErrorWithoutLine((int8_t *)"Expected no arguments.");
            } else if (tempFunction->argumentAmount == 1) {
                reportScriptErrorWithoutLine((int8_t *)"Expected 1 argument.");
            } else {
                int8_t tempBuffer[1000];
                sprintf((char *)tempBuffer, "Expected %d arguments.", tempFunction->argumentAmount);
                reportScriptErrorWithoutLine(tempBuffer);
            }
            return output;
        }
        switch (tempFunction->number) {
            case SCRIPT_FUNCTION_IS_NUM:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(tempValue.type == SCRIPT_VALUE_TYPE_NUMBER);
                break;
            }
            case SCRIPT_FUNCTION_IS_STR:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(tempValue.type == SCRIPT_VALUE_TYPE_STRING);
                break;
            }
            case SCRIPT_FUNCTION_IS_LIST:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(tempValue.type == SCRIPT_VALUE_TYPE_LIST);
                break;
            }
            case SCRIPT_FUNCTION_IS_FUNC:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(tempValue.type == SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION
                    || tempValue.type == SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION);
                break;
            }
            case SCRIPT_FUNCTION_COPY:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output = copyScriptValue(&tempValue);
                break;
            }
            case SCRIPT_FUNCTION_STR:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output = convertScriptValueToString(tempValue);
                break;
            }
            case SCRIPT_FUNCTION_NUM:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                output = convertScriptValueToNumber(tempValue);
                break;
            }
            case SCRIPT_FUNCTION_FLOOR:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = floor(*(double *)&(tempValue.data));
                break;
            }
            case SCRIPT_FUNCTION_LEN:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                if (tempValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempValue.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(output.data) = (double)(tempText->length - 1);
                } else if (tempValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempValue.data);
                    vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(output.data) = (double)(tempList->length);
                } else {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_INS:
            {
                scriptValue_t tempSequenceValue;
                scriptValue_t tempIndexValue;
                scriptValue_t tempItemValue;
                getVectorElement(&tempSequenceValue, argumentList, 0);
                getVectorElement(&tempIndexValue, argumentList, 1);
                getVectorElement(&tempItemValue, argumentList, 2);
                if (tempIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int64_t index = (int64_t)*(double *)&(tempIndexValue.data);
                if (index < 0) {
                    reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                    return output;
                }
                if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    if (tempItemValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                        reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                        return output;
                    }
                    int8_t tempCharacter = (int8_t)*(double *)&(tempItemValue.data);
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempSequenceValue.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                    if (index >= tempText->length) {
                        reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                        return output;
                    }
                    insertVectorElement(tempText, index, &tempCharacter);
                } else if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempSequenceValue.data);
                    vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
                    if (index > tempList->length) {
                        reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                        return output;
                    }
                    insertVectorElement(tempList, index, &tempItemValue);
                } else {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_REM:
            {
                scriptValue_t tempSequenceValue;
                scriptValue_t tempIndexValue;
                getVectorElement(&tempSequenceValue, argumentList, 0);
                getVectorElement(&tempIndexValue, argumentList, 1);
                if (tempIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int64_t index = (int64_t)*(double *)&(tempIndexValue.data);
                if (index < 0) {
                    reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                    return output;
                }
                if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempSequenceValue.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                    if (index >= tempText->length - 1) {
                        reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                        return output;
                    }
                    removeVectorElement(tempText, index);
                } else if (tempSequenceValue.type == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempSequenceValue.data);
                    vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
                    if (index >= tempList->length) {
                        reportScriptErrorWithoutLine((int8_t *)"Index out of range.");
                        return output;
                    }
                    removeVectorElement(tempList, index);
                } else {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_PRESS_KEY:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int32_t tempKey = (int32_t)*(double *)&(tempValue.data);
                handleKey(tempKey, false, false);
                break;
            }
            case SCRIPT_FUNCTION_GET_MODE:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(activityMode);
                break;
            }
            case SCRIPT_FUNCTION_SET_MODE:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int32_t tempMode = (int32_t)*(double *)&(tempValue.data);
                if (tempMode < 1 || tempMode > 9) {
                    reportScriptErrorWithoutLine((int8_t *)"Invalid mode.");
                    return output;
                }
                if (tempMode != COMMAND_MODE && activityMode != COMMAND_MODE) {
                    setActivityMode(COMMAND_MODE);
                }
                if (tempMode == HIGHLIGHT_CHARACTER_MODE || tempMode == HIGHLIGHT_STATIC_MODE) {
                    highlightTextPos = cursorTextPos;
                }
                setActivityMode(tempMode);
                break;
            }
            case SCRIPT_FUNCTION_GET_SELECTION_CONTENTS:
            {
                int8_t *tempText = allocateStringFromSelection();
                output = convertTextToStringValue(tempText);
                free(tempText);
                break;
            }
            case SCRIPT_FUNCTION_GET_LINE_COUNT:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                textLine_t *tempLine = getRightmostTextLine(rootTextLine);
                *(double *)&(output.data) = (double)getTextLineNumber(tempLine);
                break;
            }
            case SCRIPT_FUNCTION_GET_LINE_CONTENTS:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                if (tempValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int64_t tempLineIndex = (int64_t)*(double *)&(tempValue.data);
                textLine_t *tempLine = getTextLineByNumber(tempLineIndex + 1);
                if (tempLine == NULL) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad line index.");
                    return output;
                }
                textAllocation_t *tempAllocation = &(tempLine->textAllocation);
                int8_t tempBuffer[tempAllocation->length + 1];
                copyData(tempBuffer, tempAllocation->text, tempAllocation->length);
                tempBuffer[tempAllocation->length] = 0;
                output = convertTextToStringValue(tempBuffer);
                break;
            }
            case SCRIPT_FUNCTION_GET_CURSOR_CHAR_INDEX:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)getTextPosIndex(&cursorTextPos);
                break;
            }
            case SCRIPT_FUNCTION_GET_CURSOR_LINE_INDEX:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(getTextLineNumber(cursorTextPos.line) - 1);
                break;
            }
            case SCRIPT_FUNCTION_SET_CURSOR_POS:
            {
                scriptValue_t tempCharIndexValue;
                scriptValue_t tempLineIndexValue;
                getVectorElement(&tempCharIndexValue, argumentList, 0);
                getVectorElement(&tempLineIndexValue, argumentList, 1);
                if (tempCharIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER || tempLineIndexValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int64_t tempCharIndex = (int64_t)*(double *)&(tempCharIndexValue.data);
                int64_t tempLineIndex = (int64_t)*(double *)&(tempLineIndexValue.data);
                textLine_t *tempLine = getTextLineByNumber(tempLineIndex + 1);
                if (tempLine == NULL) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad line index.");
                    return output;
                }
                textAllocation_t *tempAllocation = &(tempLine->textAllocation);
                if (tempCharIndex < 0 || tempCharIndex > tempAllocation->length) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad char index.");
                    return output;
                }
                textPos_t tempTextPos;
                tempTextPos.line = tempLine;
                setTextPosIndex(&tempTextPos, tempCharIndex);
                moveCursor(&tempTextPos);
                break;
            }
            case SCRIPT_FUNCTION_RUN_COMMAND:
            {
                scriptValue_t tempCommandNameValue;
                scriptValue_t tempArgumentsValue;
                getVectorElement(&tempCommandNameValue, argumentList, 0);
                getVectorElement(&tempArgumentsValue, argumentList, 1);
                if (tempCommandNameValue.type != SCRIPT_VALUE_TYPE_STRING || tempArgumentsValue.type != SCRIPT_VALUE_TYPE_LIST) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(tempCommandNameValue.data);
                scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(tempArgumentsValue.data);
                vector_t *tempName = *(vector_t **)&(tempHeapValue1->data);
                vector_t *tempList = *(vector_t **)&(tempHeapValue2->data);
                int8_t *tempTermList[tempList->length + 1];
                tempTermList[0] = tempName->data;
                int64_t index = 0;
                while (index < tempList->length) {
                    scriptValue_t tempArgumentValue;
                    getVectorElement(&tempArgumentValue, tempList, index);
                    scriptValue_t tempStringValue = convertScriptValueToString(tempArgumentValue);
                    scriptHeapValue_t *tempHeapValue3 = *(scriptHeapValue_t **)&(tempStringValue.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue3->data);
                    tempTermList[index + 1] = tempText->data;
                    index += 1;
                }
                executeTextCommandByTermList(&output, tempTermList, tempList->length + 1);
                if (scriptHasError) {
                    return output;
                }
                break;
            }
            case SCRIPT_FUNCTION_NOTIFY_USER:
            {
                scriptValue_t tempValue;
                getVectorElement(&tempValue, argumentList, 0);
                scriptValue_t tempStringValue = convertScriptValueToString(tempValue);
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempStringValue.data);
                vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                notifyUser(tempText->data);
                break;
            }
            case SCRIPT_FUNCTION_PROMPT_KEY:
            {
                output.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(output.data) = (double)(getch());
                break;
            }
            case SCRIPT_FUNCTION_PROMPT_CHAR:
            {
                notifyUser((int8_t *)"Type a character.");
                int32_t tempKey = getch();
                if (tempKey < 32 || tempKey > 126) {
                    output.type = SCRIPT_VALUE_TYPE_NULL;
                } else {
                    output.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(output.data) = (double)(tempKey);
                }
                break;
            }
            case SCRIPT_FUNCTION_BIND_KEY:
            {
                scriptValue_t tempKeyValue;
                scriptValue_t tempCallbackValue;
                getVectorElement(&tempKeyValue, argumentList, 0);
                getVectorElement(&tempCallbackValue, argumentList, 1);
                if (tempKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int32_t tempKey = (int32_t)*(double *)&(tempKeyValue.data);
                keyBinding_t tempKeyBinding;
                tempKeyBinding.key = tempKey;
                tempKeyBinding.callback = tempCallbackValue;
                pushVectorElement(&keyBindingList, &tempKeyBinding);
                break;
            }
            case SCRIPT_FUNCTION_MAP_KEY:
            {
                scriptValue_t tempOldKeyValue;
                scriptValue_t tempNewKeyValue;
                scriptValue_t tempModeValue;
                getVectorElement(&tempOldKeyValue, argumentList, 0);
                getVectorElement(&tempNewKeyValue, argumentList, 1);
                getVectorElement(&tempModeValue, argumentList, 2);
                if (tempOldKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER || tempNewKeyValue.type != SCRIPT_VALUE_TYPE_NUMBER
                        || tempModeValue.type != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                int32_t tempOldKey = (int32_t)*(double *)&(tempOldKeyValue.data);
                int32_t tempNewKey = (int32_t)*(double *)&(tempNewKeyValue.data);
                int32_t tempMode = (int32_t)*(double *)&(tempModeValue.data);
                keyMapping_t *tempOldKeyMapping = findKeyMapping(tempOldKey, tempMode);
                if (tempOldKeyMapping == NULL) {
                    keyMapping_t tempNewKeyMapping;
                    tempNewKeyMapping.oldKey = tempOldKey;
                    tempNewKeyMapping.newKey = tempNewKey;
                    tempNewKeyMapping.mode = tempMode;
                    pushVectorElement(&keyMappingList, &tempNewKeyMapping);
                } else {
                    tempOldKeyMapping->newKey = tempNewKey;
                }
                break;
            }
            case SCRIPT_FUNCTION_BIND_COMMAND:
            {
                scriptValue_t tempCommandNameValue;
                scriptValue_t tempCallbackValue;
                getVectorElement(&tempCommandNameValue, argumentList, 0);
                getVectorElement(&tempCallbackValue, argumentList, 1);
                if (tempCommandNameValue.type != SCRIPT_VALUE_TYPE_STRING) {
                    reportScriptErrorWithoutLine((int8_t *)"Bad argument type.");
                    return output;
                }
                endwin();
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempCommandNameValue.data);
                vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                commandBinding_t *tempOldCommandBinding = findCommandBinding(tempText->data);
                if (tempOldCommandBinding == NULL) {
                    commandBinding_t tempNewCommandBinding;
                    tempNewCommandBinding.commandName = malloc(tempText->length);
                    strcpy((char *)(tempNewCommandBinding.commandName), (char *)(tempText->data));
                    tempNewCommandBinding.callback = tempCallbackValue;
                    pushVectorElement(&commandBindingList, &tempNewCommandBinding);
                } else {
                    tempOldCommandBinding->callback = tempCallbackValue;
                }
                break;
            }
            default:
            {
                reportScriptErrorWithoutLine((int8_t *)"Unknown function.");
                return output;
                break;
            }
        }
        return output;
    } else {
        reportScriptErrorWithoutLine((int8_t *)"Value is not a function.");
        return output;
    }
}

int8_t escapeScriptCharacter(int8_t character) {
    if (character == 'n') {
        return '\n';
    } else if (character == 't') {
        return '\t';
    } else {
        return character;
    }
}

void storeExpressionResultValueInDestination(expressionResult_t *expressionResult, scriptBodyLine_t *scriptBodyLine) {
    if (expressionResult->destinationType == DESTINATION_TYPE_VALUE) {
        *(scriptValue_t *)(expressionResult->destination) = expressionResult->value;
    } else if (expressionResult->destinationType == DESTINATION_TYPE_CHARACTER) {
        if (expressionResult->value.type == SCRIPT_VALUE_TYPE_NUMBER) {
            *(int8_t *)(expressionResult->destination) = (int8_t)*(double *)&(expressionResult->value.data);
        } else {
            reportScriptError((int8_t *)"Bad operand types.", scriptBodyLine);
        }
    } else {
        reportScriptError((int8_t *)"Invalid destination.", scriptBodyLine);
    }
}

expressionResult_t evaluateExpression(scriptBodyPos_t *scriptBodyPos, int8_t precedence) {
    expressionResult_t expressionResult;
    expressionResult.value.type = SCRIPT_VALUE_TYPE_MISSING;
    expressionResult.destinationType = DESTINATION_TYPE_NONE;
    expressionResult.destination = NULL;
    scriptBodyPosSkipWhitespace(scriptBodyPos);
    scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_UNARY_PREFIX);
    if (tempOperator != NULL) {
        scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
        expressionResult_t tempResult = evaluateExpression(scriptBodyPos, tempOperator->precedence);
        if (scriptHasError) {
            return expressionResult;
        }
        int8_t tempType = tempResult.value.type;
        switch (tempOperator->number) {
            case SCRIPT_OPERATOR_NEGATE:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = -*(double *)&(tempResult.value.data);
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_BOOLEAN_NOT:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (*(double *)&(tempResult.value.data) == 0.0);
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_BITWISE_NOT:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (double)~(uint32_t)*(double *)&(tempResult.value.data);
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_INCREMENT_PREFIX:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    if (tempResult.destinationType == DESTINATION_TYPE_VALUE) {
                        scriptValue_t *tempValue = (scriptValue_t *)(tempResult.destination);
                        *(double *)&(tempValue->data) += 1;
                        expressionResult.value = *tempValue;
                    } else if (tempResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                        int8_t *tempValue = (int8_t *)(tempResult.destination);
                        *tempValue += 1;
                        *(double *)&(expressionResult.value.data) = (double)*tempValue;
                    }
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            case SCRIPT_OPERATOR_DECREMENT_PREFIX:
            {
                if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                    if (tempResult.destinationType == DESTINATION_TYPE_VALUE) {
                        scriptValue_t *tempValue = (scriptValue_t *)(tempResult.destination);
                        *(double *)&(tempValue->data) -= 1;
                        expressionResult.value = *tempValue;
                    } else if (tempResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                        int8_t *tempValue = (int8_t *)(tempResult.destination);
                        *tempValue -= 1;
                        *(double *)&(expressionResult.value.data) = (double)*tempValue;
                    }
                } else {
                    reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                break;
            }
            default:
            {
                break;
            }
        }
        return expressionResult;
    }
    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
    while (true) {
        if (characterIsEndOfScriptLine(tempFirstCharacter)) {
            return expressionResult;
        }
        if (isScriptNumberCharacter(tempFirstCharacter)) {
            scriptBodyPos_t tempScriptBodyPos = *scriptBodyPos;
            scriptBodyPosSeekEndOfNumber(&tempScriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
            int8_t tempText[tempLength + 1];
            copyData(tempText, getScriptBodyPosPointer(scriptBodyPos), tempLength);
            tempText[tempLength] = 0;
            double tempNumber;
            sscanf((char *)tempText, "%lf", &tempNumber);
            // TODO: Handle malformed numbers.
            // TODO: Handle hexadecimal numbers.
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = tempNumber;
            *scriptBodyPos = tempScriptBodyPos;
            break;
        }
        if (isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
            scriptBodyPos_t tempScriptBodyPos = *scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(scriptBodyPos, &tempScriptBodyPos);
            scriptVariable_t *tempVariable = scriptScopeFindVariableWithNameLength(localScriptScope, tempText, tempLength);
            if (tempVariable != NULL) {
                expressionResult.value = tempVariable->value;
                expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                expressionResult.destination = &(tempVariable->value);
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
            if (localScriptScope != globalScriptScope) {
                tempVariable = scriptScopeFindVariableWithNameLength(globalScriptScope, tempText, tempLength);
                if (tempVariable != NULL) {
                    expressionResult.value = tempVariable->value;
                    expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                    expressionResult.destination = &(tempVariable->value);
                    *scriptBodyPos = tempScriptBodyPos;
                    break;
                }
            }
            scriptBuiltInFunction_t *tempBuiltInFunction = findScriptBuiltInFunctionByName(tempText, tempLength);
            if (tempBuiltInFunction != NULL) {
                expressionResult.value.type = SCRIPT_VALUE_TYPE_BUILT_IN_FUNCTION;
                *(scriptBuiltInFunction_t **)&(expressionResult.value.data) = tempBuiltInFunction;
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
            scriptConstant_t *tempConstant = getScriptConstantByName(tempText, tempLength);
            if (tempConstant != NULL) {
                expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                *(double *)&(expressionResult.value.data) = (double)(tempConstant->value);
                *scriptBodyPos = tempScriptBodyPos;
                break;
            }
        }
        if (tempFirstCharacter == '"') {
            vector_t *tempText = malloc(sizeof(vector_t));
            createEmptyVector(tempText, 1);
            scriptBodyPos->index += 1;
            int8_t tempIsEscaped = false;
            while (true) {
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (characterIsEndOfScriptLine(tempCharacter)) {
                    reportScriptError((int8_t *)"Unexpected end of string.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                if (tempIsEscaped) {
                    tempCharacter = escapeScriptCharacter(tempCharacter);
                    pushVectorElement(tempText, &tempCharacter);
                    tempIsEscaped = false;
                } else {
                    if (tempCharacter == '"') {
                        break;
                    } else if (tempCharacter == '\\') {
                        tempIsEscaped = true;
                    } else {
                        pushVectorElement(tempText, &tempCharacter);
                    }
                }
                scriptBodyPos->index += 1;
            }
            scriptBodyPos->index += 1;
            int8_t tempCharacter = 0;
            pushVectorElement(tempText, &tempCharacter);
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_STRING;
            *(vector_t **)&(tempHeapValue->data) = tempText;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_STRING;
            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue;
            break;
        }
        if (tempFirstCharacter == '\'') {
            scriptBodyPos->index += 1;
            int8_t tempValue = scriptBodyPosGetCharacter(scriptBodyPos);
            if (characterIsEndOfScriptLine(tempValue)) {
                reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            if (tempValue == '\\') {
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (characterIsEndOfScriptLine(tempCharacter)) {
                    reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPos->index += 1;
                tempValue = escapeScriptCharacter(tempCharacter);
            }
            int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempCharacter != '\'') {
                reportScriptError((int8_t *)"Malformed character.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = tempValue;
            break;
        }
        if (tempFirstCharacter == '(') {
            scriptBodyPos->index += 1;
            expressionResult = evaluateExpression(scriptBodyPos, 99);
            if (scriptHasError) {
                return expressionResult;
            }
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempCharacter != ')') {
                reportScriptError((int8_t *)"Missing close parenthesis.", scriptBodyPos->scriptBodyLine);
                return expressionResult;
            }
            scriptBodyPos->index += 1;
            break;
        }
        if (tempFirstCharacter == '[') {
            scriptBodyPos->index += 1;
            vector_t *tempList = malloc(sizeof(vector_t));
            getScriptBodyValueList(tempList, scriptBodyPos, ']');
            if (scriptHasError) {
                return expressionResult;
            }
            scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
            tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
            *(vector_t **)&(tempHeapValue->data) = tempList;
            expressionResult.value.type = SCRIPT_VALUE_TYPE_LIST;
            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"true")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = 1.0;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"false")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
            *(double *)&(expressionResult.value.data) = 0.0;
            break;
        }
        if (scriptBodyPosTextMatchesIdentifier(scriptBodyPos, (int8_t *)"null")) {
            expressionResult.value.type = SCRIPT_VALUE_TYPE_NULL;
            break;
        }
        reportScriptError((int8_t *)"Unknown expression type.", scriptBodyPos->scriptBodyLine);
        return expressionResult;
    }
    while (true) {
        int8_t hasProcessedOperator = false;
        while (true) {
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
            if (tempFirstCharacter == '(') {
                scriptBodyPos->index += 1;
                vector_t tempArgumentList;
                getScriptBodyValueList(&tempArgumentList, scriptBodyPos, ')');
                if (scriptHasError) {
                    return expressionResult;
                }
                scriptValue_t tempValue = invokeFunction(expressionResult.value, &tempArgumentList);
                cleanUpVector(&tempArgumentList);
                if (scriptHasError) {
                    if (!scriptErrorHasLine) {
                        scriptErrorLine = *(scriptBodyPos->scriptBodyLine);
                        scriptErrorHasLine = true;
                    }
                    return expressionResult;
                }
                expressionResult.value = tempValue;
                expressionResult.destinationType = DESTINATION_TYPE_NONE;
                expressionResult.destination = NULL;
                hasProcessedOperator = true;
                break;
            }
            if (tempFirstCharacter == '[') {
                scriptBodyPos->index += 1;
                expressionResult_t tempResult = evaluateExpression(scriptBodyPos, 99);
                if (scriptHasError) {
                    return expressionResult;
                }
                int8_t tempType1 = expressionResult.value.type;
                int8_t tempType2 = tempResult.value.type;
                if (tempType2 != SCRIPT_VALUE_TYPE_NUMBER) {
                    reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                int64_t index = (int64_t)*(double *)&(tempResult.value.data);
                if (tempType1 == SCRIPT_VALUE_TYPE_STRING) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(expressionResult.value.data);
                    vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                    if (index < 0 || index >= tempText->length - 1) {
                        reportScriptError((int8_t *)"Index out of range.", scriptBodyPos->scriptBodyLine);
                        return expressionResult;
                    }
                    int8_t *tempLocation = (int8_t *)findVectorElement(tempText, index);
                    int8_t tempCharacter = *tempLocation;
                    expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                    *(double *)&(expressionResult.value.data) = (double)tempCharacter;
                    expressionResult.destinationType = DESTINATION_TYPE_CHARACTER;
                    expressionResult.destination = tempLocation;
                } else if (tempType1 == SCRIPT_VALUE_TYPE_LIST) {
                    scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(expressionResult.value.data);
                    vector_t *tempList = *(vector_t **)&(tempHeapValue->data);
                    if (index < 0 || index > tempList->length - 1) {
                        reportScriptError((int8_t *)"Index out of range.", scriptBodyPos->scriptBodyLine);
                        return expressionResult;
                    }
                    scriptValue_t *tempLocation = (scriptValue_t *)findVectorElement(tempList, index);
                    scriptValue_t tempValue = *tempLocation;
                    expressionResult.value = tempValue;
                    expressionResult.destinationType = DESTINATION_TYPE_VALUE;
                    expressionResult.destination = tempLocation;
                } else {
                    reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPosSkipWhitespace(scriptBodyPos);
                int8_t tempCharacter = scriptBodyPosGetCharacter(scriptBodyPos);
                if (tempCharacter != ']') {
                    reportScriptError((int8_t *)"Missing close bracket.", scriptBodyPos->scriptBodyLine);
                    return expressionResult;
                }
                scriptBodyPos->index += 1;
                hasProcessedOperator = true;
                break;
            }
            scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_UNARY_POSTFIX);
            if (tempOperator != NULL) {
                scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
                int8_t tempType = expressionResult.value.type;
                switch (tempOperator->number) {
                    case SCRIPT_OPERATOR_INCREMENT_POSTFIX:
                    {
                        if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                            if (expressionResult.destinationType == DESTINATION_TYPE_VALUE) {
                                scriptValue_t *tempValue = (scriptValue_t *)(expressionResult.destination);
                                *(double *)&(tempValue->data) += 1;
                            } else if (expressionResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                                int8_t *tempValue = (int8_t *)(expressionResult.destination);
                                *tempValue += 1;
                            }
                        } else {
                            reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DECREMENT_POSTFIX:
                    {
                        if (tempType == SCRIPT_VALUE_TYPE_NUMBER) {
                            if (expressionResult.destinationType == DESTINATION_TYPE_VALUE) {
                                scriptValue_t *tempValue = (scriptValue_t *)(expressionResult.destination);
                                *(double *)&(tempValue->data) -= 1;
                            } else if (expressionResult.destinationType == DESTINATION_TYPE_CHARACTER) {
                                int8_t *tempValue = (int8_t *)(expressionResult.destination);
                                *tempValue -= 1;
                            }
                        } else {
                            reportScriptError((int8_t *)"Bad operand type.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                hasProcessedOperator = true;
                break;
            }
            break;
        }
        if (!hasProcessedOperator) {
            break;
        }
    }
    while (true) {
        int8_t hasProcessedOperator = false;
        while (true) {
            scriptBodyPosSkipWhitespace(scriptBodyPos);
            scriptOperator_t *tempOperator = scriptBodyPosGetOperator(scriptBodyPos, SCRIPT_OPERATOR_TYPE_BINARY);
            if (tempOperator != NULL) {
                if (tempOperator->precedence >= precedence) {
                    break;
                }
                scriptBodyPosSkipOperator(scriptBodyPos, tempOperator);
                expressionResult_t tempResult = evaluateExpression(scriptBodyPos, tempOperator->precedence);
                if (scriptHasError) {
                    return expressionResult;
                }
                int8_t tempType1 = expressionResult.value.type;
                int8_t tempType2 = tempResult.value.type;
                switch (tempOperator->number) {
                    case SCRIPT_OPERATOR_ADD:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) += *(double *)&(tempResult.value.data);
                        } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                            scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(expressionResult.value.data);
                            scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(tempResult.value.data);
                            vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
                            vector_t *tempText2 = *(vector_t **)&(tempHeapValue2->data);
                            vector_t *tempText3 = malloc(sizeof(vector_t));
                            copyVector(tempText3, tempText1);
                            removeVectorElement(tempText3, tempText3->length - 1);
                            pushVectorOntoVector(tempText3, tempText2);
                            scriptHeapValue_t *tempHeapValue3 = createScriptHeapValue();
                            tempHeapValue3->type = SCRIPT_VALUE_TYPE_STRING;
                            *(vector_t **)&(tempHeapValue3->data) = tempText3;
                            expressionResult.value.type = SCRIPT_VALUE_TYPE_STRING;
                            *(scriptHeapValue_t **)&(expressionResult.value.data) = tempHeapValue3;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_SUBTRACT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) -= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MULTIPLY:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) *= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DIVIDE:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue = *(double *)&(tempResult.value.data);
                            if (tempValue == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) /= tempValue;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MODULUS:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            if (tempValue2 == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) = fmod(tempValue1, tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_AND:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 && tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_OR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 || tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_XOR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(!!tempValue1 ^ !!tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_AND:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 & tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_OR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 | tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_XOR:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 ^ tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_LEFT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 << tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_RIGHT:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >> tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_GREATER:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 > tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_LESS:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 < tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_EQUAL:
                    {
                        int8_t tempConditionResult = scriptValuesAreEqualShallow(&(expressionResult.value), &(tempResult.value));
                        *(double *)&(expressionResult.value.data) = (double)tempConditionResult;
                        expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                        break;
                    }
                    case SCRIPT_OPERATOR_NOT_EQUAL:
                    {
                        int8_t tempConditionResult = scriptValuesAreEqualShallow(&(expressionResult.value), &(tempResult.value));
                        *(double *)&(expressionResult.value.data) = (double)!tempConditionResult;
                        expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                        break;
                    }
                    case SCRIPT_OPERATOR_GREATER_OR_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >= tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_LESS_OR_EQUAL:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 <= tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_IDENTICAL:
                    {
                        int8_t tempConditionResult = scriptValuesAreIdentical(&(expressionResult.value), &(tempResult.value));
                        *(double *)&(expressionResult.value.data) = (double)tempConditionResult;
                        expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                        break;
                    }
                    case SCRIPT_OPERATOR_NOT_IDENTICAL:
                    {
                        int8_t tempConditionResult = scriptValuesAreIdentical(&(expressionResult.value), &(tempResult.value));
                        *(double *)&(expressionResult.value.data) = (double)!tempConditionResult;
                        expressionResult.value.type = SCRIPT_VALUE_TYPE_NUMBER;
                        break;
                    }
                    case SCRIPT_OPERATOR_ASSIGN:
                    {
                        expressionResult.value = tempResult.value;
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_ADD_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) += *(double *)&(tempResult.value.data);
                        } else if (tempType1 == SCRIPT_VALUE_TYPE_STRING && tempType2 == SCRIPT_VALUE_TYPE_STRING) {
                            scriptHeapValue_t *tempHeapValue1 = *(scriptHeapValue_t **)&(expressionResult.value.data);
                            scriptHeapValue_t *tempHeapValue2 = *(scriptHeapValue_t **)&(tempResult.value.data);
                            vector_t *tempText1 = *(vector_t **)&(tempHeapValue1->data);
                            vector_t *tempText2 = *(vector_t **)&(tempHeapValue2->data);
                            removeVectorElement(tempText1, tempText1->length - 1);
                            pushVectorOntoVector(tempText1, tempText2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_SUBTRACT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) -= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MULTIPLY_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            *(double *)&(expressionResult.value.data) *= *(double *)&(tempResult.value.data);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_DIVIDE_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue = *(double *)&(tempResult.value.data);
                            if (tempValue == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) /= tempValue;
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_MODULUS_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            if (tempValue2 == 0.0) {
                                reportScriptError((int8_t *)"Divide by zero.", scriptBodyPos->scriptBodyLine);
                                return expressionResult;
                            }
                            *(double *)&(expressionResult.value.data) = fmod(tempValue1, tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_AND_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 && tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_OR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 || tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BOOLEAN_XOR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempValue1 = *(double *)&(expressionResult.value.data);
                            double tempValue2 = *(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(!!tempValue1 ^ !!tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_AND_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 & tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_OR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 | tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITWISE_XOR_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 ^ tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_LEFT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 << tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    case SCRIPT_OPERATOR_BITSHIFT_RIGHT_ASSIGN:
                    {
                        if (tempType1 == SCRIPT_VALUE_TYPE_NUMBER && tempType2 == SCRIPT_VALUE_TYPE_NUMBER) {
                            uint32_t tempValue1 = (uint32_t)*(double *)&(expressionResult.value.data);
                            uint32_t tempValue2 = (uint32_t)*(double *)&(tempResult.value.data);
                            *(double *)&(expressionResult.value.data) = (double)(tempValue1 >> tempValue2);
                        } else {
                            reportScriptError((int8_t *)"Bad operand types.", scriptBodyPos->scriptBodyLine);
                            return expressionResult;
                        }
                        storeExpressionResultValueInDestination(&expressionResult, scriptBodyPos->scriptBodyLine);
                        if (scriptHasError) {
                            return expressionResult;
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                expressionResult.destinationType = DESTINATION_TYPE_NONE;
                expressionResult.destination = NULL;
                hasProcessedOperator = true;
                break;
            }
            break;
        }
        if (!hasProcessedOperator) {
            break;
        }
    }
    return expressionResult;
}

int8_t evaluateStatement(scriptValue_t *returnValue, scriptBodyLine_t *scriptBodyLine) {
    if (scriptBodyLine->index >= scriptBodyLine->scriptBody->length) {
        return false;
    }
    garbageCollectionDelay -= 1;
    if (garbageCollectionDelay <= 0) {
        //garbageCollectScriptHeapValues();
        // TODO: Decrease the frequency.
        garbageCollectionDelay = 0;
    }
    scriptBodyPos_t scriptBodyPos;
    scriptBodyPos.scriptBodyLine = scriptBodyLine;
    scriptBodyPos.index = scriptBodyLine->index;
    scriptBodyPosSkipWhitespace(&scriptBodyPos);
    scriptBranch_t *currentBranch = findVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
    vector_t *currentScopeStack = &(currentBranch->line.scriptBody->scopeStack);
    globalScriptScope = findVectorElement(currentScopeStack, 0);
    localScriptScope = findVectorElement(currentScopeStack, currentScopeStack->length - 1);
    if (currentBranch->type == SCRIPT_BRANCH_TYPE_IMPORT) {
        if (!currentBranch->shouldIgnore) {
            scriptScope_t *tempScope = findVectorElement(&(currentBranch->importScriptBody->scopeStack), 0);
            if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"share")) {
                while (true) {
                    scriptBodyPosSkipWhitespace(&scriptBodyPos);
                    int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
                    if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                        reportScriptError((int8_t *)"Bad variable name.", scriptBodyLine);
                        return false;
                    }
                    scriptBodyPos_t tempScriptBodyPos;
                    tempScriptBodyPos = scriptBodyPos;
                    scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
                    int8_t *tempText = getScriptBodyPosPointer(&scriptBodyPos);
                    int64_t tempLength = getDistanceToScriptBodyPos(&scriptBodyPos, &tempScriptBodyPos);
                    int8_t tempName[tempLength + 1];
                    copyData(tempName, tempText, tempLength);
                    tempName[tempLength] = 0;
                    scriptVariable_t *tempSourceVariable = scriptScopeFindVariable(tempScope, tempName);
                    if (tempSourceVariable == NULL) {
                        reportScriptError((int8_t *)"Missing variable.", scriptBodyLine);
                        return false;
                    }
                    scriptVariable_t *tempDestinationVariable = scriptScopeFindVariable(localScriptScope, tempName);
                    if (tempDestinationVariable == NULL) {
                        scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
                        tempDestinationVariable = scriptScopeAddVariable(localScriptScope, tempNewVariable);
                    }
                    tempDestinationVariable->value = tempSourceVariable->value;
                    scriptBodyPos = tempScriptBodyPos;
                    scriptBodyPosSkipWhitespace(&scriptBodyPos);
                    int8_t tempCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
                    if (characterIsEndOfScriptLine(tempCharacter)) {
                        break;
                    }
                    if (tempCharacter == ',') {
                        scriptBodyPos.index += 1;
                    } else {
                        reportScriptError((int8_t *)"Unexpected token.", scriptBodyLine);
                        return false;
                    }
                }
                return seekNextScriptBodyLine(scriptBodyLine);
            }
            if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"greedy")) {
                scriptBodyPosSkipWhitespace(&scriptBodyPos);
                if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"dirtbag")) {
                    int64_t index = 0;
                    while (index < tempScope->variableList.length) {
                        scriptVariable_t *tempSourceVariable = findVectorElement(&(tempScope->variableList), index);
                        int8_t *tempName = tempSourceVariable->name;
                        scriptVariable_t *tempDestinationVariable = scriptScopeFindVariable(localScriptScope, tempName);
                        if (tempDestinationVariable == NULL) {
                            scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
                            tempDestinationVariable = scriptScopeAddVariable(localScriptScope, tempNewVariable);
                        } else {
                            reportScriptError((int8_t *)"Greedy dirtbag collision.", scriptBodyLine);
                            return false;
                        }
                        tempDestinationVariable->value = tempSourceVariable->value;
                        index += 1;
                    }
                    return seekNextScriptBodyLine(scriptBodyLine);
                }
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"end")) {
            removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        reportScriptError((int8_t *)"Invalid statement in import body.", scriptBodyLine);
        return false;
    }
    if (currentBranch->shouldIgnore) {
        scriptBranch_t *lastBranch = findVectorElement(&scriptBranchStack, scriptBranchStack.length - 2);
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_IF;
            tempBranch.shouldIgnore = true;
            tempBranch.hasExecuted = false;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"else")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                if (!lastBranch->shouldIgnore && !currentBranch->hasExecuted) {
                    scriptBodyPosSkipWhitespace(&scriptBodyPos);
                    if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
                        expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                        if (scriptHasError) {
                            return false;
                        }
                        if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                            double tempCondition = *(double *)&(tempResult.value.data);
                            if (tempCondition) {
                                currentBranch->shouldIgnore = false;
                                currentBranch->hasExecuted = true;
                            }
                        } else {
                            reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                            return false;
                        }
                    } else {
                        currentBranch->shouldIgnore = false;
                        currentBranch->hasExecuted = true;
                    }
                }
                return seekNextScriptBodyLine(scriptBodyLine);
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"while")) {
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_WHILE;
            tempBranch.shouldIgnore = true;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"import")) {
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_IMPORT;
            tempBranch.shouldIgnore = true;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"end")) {
            removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        return seekNextScriptBodyLine(scriptBodyLine);
    } else {
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"dec")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                reportScriptError((int8_t *)"Missing declaration name.", scriptBodyLine);
                return false;
            }
            scriptBodyPos_t tempScriptBodyPos;
            tempScriptBodyPos = scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(&scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(&scriptBodyPos, &tempScriptBodyPos);
            int8_t tempName[tempLength + 1];
            copyData(tempName, tempText, tempLength);
            tempName[tempLength] = 0;
            scriptVariable_t *tempVariable = scriptScopeFindVariable(localScriptScope, tempName);
            if (tempVariable == NULL) {
                scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
                tempVariable = scriptScopeAddVariable(localScriptScope, tempNewVariable);
            }
            scriptBodyPos = tempScriptBodyPos;
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (tempCharacter == '=') {
                scriptBodyPos.index += 1;
                scriptBodyPosSkipWhitespace(&scriptBodyPos);
                expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                if (scriptHasError) {
                    return false;
                }
                tempVariable->value = tempResult.value;
            }
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"if")) {
            expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
            if (scriptHasError) {
                return false;
            }
            if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempCondition = *(double *)&(tempResult.value.data);
                int8_t tempShouldExecute = (tempCondition != 0);
                scriptBranch_t tempBranch;
                tempBranch.type = SCRIPT_BRANCH_TYPE_IF;
                tempBranch.shouldIgnore = !tempShouldExecute;
                tempBranch.hasExecuted = tempShouldExecute;
                tempBranch.line = *scriptBodyLine;
                pushVectorElement(&scriptBranchStack, &tempBranch);
                return seekNextScriptBodyLine(scriptBodyLine);
            } else {
                reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"else")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                currentBranch->shouldIgnore = true;
                return seekNextScriptBodyLine(scriptBodyLine);
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"while")) {
            expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
            if (scriptHasError) {
                return false;
            }
            if (tempResult.value.type == SCRIPT_VALUE_TYPE_NUMBER) {
                double tempCondition = *(double *)&(tempResult.value.data);
                int8_t tempShouldExecute = (tempCondition != 0);
                scriptBranch_t tempBranch;
                tempBranch.type = SCRIPT_BRANCH_TYPE_WHILE;
                tempBranch.shouldIgnore = !tempShouldExecute;
                tempBranch.line = *scriptBodyLine;
                pushVectorElement(&scriptBranchStack, &tempBranch);
                return seekNextScriptBodyLine(scriptBodyLine);
            } else {
                reportScriptError((int8_t *)"Invalid condition type.", scriptBodyLine);
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"func")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempFirstCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (!isFirstScriptIdentifierCharacter(tempFirstCharacter)) {
                reportScriptError((int8_t *)"Missing declaration name.", scriptBodyLine);
                return false;
            }
            scriptBodyPos_t tempScriptBodyPos;
            tempScriptBodyPos = scriptBodyPos;
            scriptBodyPosSeekEndOfIdentifier(&tempScriptBodyPos);
            int8_t *tempText = getScriptBodyPosPointer(&scriptBodyPos);
            int64_t tempLength = getDistanceToScriptBodyPos(&scriptBodyPos, &tempScriptBodyPos);
            int8_t tempName[tempLength + 1];
            copyData(tempName, tempText, tempLength);
            tempName[tempLength] = 0;
            scriptVariable_t *tempVariable = scriptScopeFindVariable(localScriptScope, tempName);
            if (tempVariable == NULL) {
                scriptVariable_t tempNewVariable = createEmptyScriptVariable(tempName);
                tempVariable = scriptScopeAddVariable(localScriptScope, tempNewVariable);
            }
            scriptCustomFunction_t *tempScriptFunction = malloc(sizeof(scriptCustomFunction_t));
            tempScriptFunction->scriptBodyLine = *scriptBodyLine;
            scriptHeapValue_t *tempHeapValue = malloc(sizeof(scriptHeapValue_t));
            tempHeapValue->type = SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION;
            *(scriptCustomFunction_t **)&(tempHeapValue->data) = tempScriptFunction;
            scriptValue_t tempValue;
            tempValue.type = SCRIPT_VALUE_TYPE_CUSTOM_FUNCTION;
            *(scriptHeapValue_t **)&(tempValue.data) = tempHeapValue;
            tempVariable->value = tempValue;
            scriptBranch_t tempBranch;
            tempBranch.type = SCRIPT_BRANCH_TYPE_FUNCTION;
            tempBranch.shouldIgnore = true;
            tempBranch.line = *scriptBodyLine;
            pushVectorElement(&scriptBranchStack, &tempBranch);
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"end")) {
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_IF) {
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                return seekNextScriptBodyLine(scriptBodyLine);
            }
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_WHILE) {
                *scriptBodyLine = currentBranch->line;
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                return true;
            }
            if (currentBranch->type == SCRIPT_BRANCH_TYPE_FUNCTION) {
                removeVectorElement(&scriptBranchStack, scriptBranchStack.length - 1);
                vector_t *tempScopeStack = &(scriptBodyLine->scriptBody->scopeStack);
                int64_t index = tempScopeStack->length - 1;
                scriptScope_t *tempScope = findVectorElement(tempScopeStack, index);
                cleanUpScriptScope(tempScope);
                removeVectorElement(tempScopeStack, index);
                returnValue->type = SCRIPT_VALUE_TYPE_MISSING;
                return false;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"break")) {
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid break statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t *tempBranch = findVectorElement(&scriptBranchStack, index);
                tempBranch->shouldIgnore = true;
                if (tempBranch->type == SCRIPT_BRANCH_TYPE_WHILE) {
                    break;
                }
                index -= 1;
            }
            return seekNextScriptBodyLine(scriptBodyLine);
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"continue")) {
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid continue statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t tempBranch;
                getVectorElement(&tempBranch, &scriptBranchStack, index);
                removeVectorElement(&scriptBranchStack, index);
                if (tempBranch.type == SCRIPT_BRANCH_TYPE_WHILE) {
                    *scriptBodyLine = tempBranch.line;
                    return true;
                }
                index -= 1;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"ret")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            int8_t tempCharacter = scriptBodyPosGetCharacter(&scriptBodyPos);
            if (!characterIsEndOfScriptLine(tempCharacter)) {
                expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
                if (scriptHasError) {
                    return false;
                }
                *returnValue = tempResult.value;
            } else {
                returnValue->type = SCRIPT_VALUE_TYPE_MISSING;
            }
            int64_t index = scriptBranchStack.length - 1;
            while (true) {
                if (index < 0) {
                    reportScriptError((int8_t *)"Invalid return statement.", scriptBodyLine);
                    return false;
                }
                scriptBranch_t tempBranch;
                getVectorElement(&tempBranch, &scriptBranchStack, index);
                removeVectorElement(&scriptBranchStack, index);
                if (tempBranch.type == SCRIPT_BRANCH_TYPE_FUNCTION) {
                    return false;
                }
                index -= 1;
            }
        }
        if (scriptBodyPosTextMatchesIdentifier(&scriptBodyPos, (int8_t *)"import")) {
            scriptBodyPosSkipWhitespace(&scriptBodyPos);
            expressionResult_t tempResult = evaluateExpression(&scriptBodyPos, 99);
            if (scriptHasError) {
                return false;
            }
            if (tempResult.value.type == SCRIPT_VALUE_TYPE_STRING) {
                scriptHeapValue_t *tempHeapValue = *(scriptHeapValue_t **)&(tempResult.value.data);
                vector_t *tempText = *(vector_t **)&(tempHeapValue->data);
                scriptBranch_t tempBranch;
                tempBranch.type = SCRIPT_BRANCH_TYPE_IMPORT;
                tempBranch.shouldIgnore = false;
                tempBranch.line = *scriptBodyLine;
                tempBranch.importScriptBody = importScript(tempText->data);
                pushVectorElement(&scriptBranchStack, &tempBranch);
                return seekNextScriptBodyLine(scriptBodyLine);
            } else {
                reportScriptError((int8_t *)"Invalid path type.", scriptBodyLine);
                return false;
            }
        }
        scriptBodyPos_t tempScriptBodyPos;
        tempScriptBodyPos.scriptBodyLine = scriptBodyLine;
        tempScriptBodyPos.index = scriptBodyLine->index;
        evaluateExpression(&tempScriptBodyPos, 99);
        if (scriptHasError) {
            return false;
        }
        return seekNextScriptBodyLine(scriptBodyLine);
    }
}

scriptValue_t evaluateScriptBody(scriptBodyLine_t *scriptBodyLine) {
    scriptValue_t output;
    output.type = SCRIPT_VALUE_TYPE_MISSING;
    while (true) {
        int8_t tempResult = evaluateStatement(&output, scriptBodyLine);
        if (!tempResult || scriptHasError) {
            break;
        }
    }
    return output;
}

scriptBody_t *importScriptHelper(int8_t *path) {
    int32_t index = 0;
    while (index < scriptBodyList.length) {
        scriptBody_t *tempScriptBody;
        tempScriptBody = findVectorElement(&scriptBodyList, index);
        if (strcmp((char *)(tempScriptBody->path), (char *)path) == 0) {
            return tempScriptBody;
        }
        index += 1;
    }
    scriptBody_t tempNewScriptBody;
    int8_t tempResult = loadScriptBody(&tempNewScriptBody, path);
    if (!tempResult) {
        reportScriptErrorWithoutLine((int8_t *)"Import file missing.");
        return NULL;
    }
    createEmptyVector(&(tempNewScriptBody.scopeStack), sizeof(scriptScope_t));
    scriptBodyAddScope(&tempNewScriptBody, createEmptyScriptScope());
    pushVectorElement(&scriptBodyList, &tempNewScriptBody);
    scriptBody_t *tempScriptBody = findVectorElement(&scriptBodyList, scriptBodyList.length - 1);
    scriptBodyLine_t tempScriptBodyLine;
    tempScriptBodyLine.scriptBody = tempScriptBody;
    tempScriptBodyLine.index = 0;
    tempScriptBodyLine.number = 1;
    scriptBranch_t tempBranch;
    tempBranch.type = SCRIPT_BRANCH_TYPE_ROOT;
    tempBranch.shouldIgnore = false;
    tempBranch.line = tempScriptBodyLine;
    pushVectorElement(&scriptBranchStack, &tempBranch);
    evaluateScriptBody(&tempScriptBodyLine);
    return tempScriptBody;
}

scriptBody_t *importScript(int8_t *path) {
    path = mallocRealpath(path);
    scriptBody_t *output = importScriptHelper(path);
    free(path);
    return output;
}

void resetScriptError() {
    scriptHasError = false;
    scriptErrorLine.number = -1;
}

void displayScriptError() {
    int8_t tempText[1000];
    if (scriptErrorLine.number < 0) {
        sprintf((char *)tempText, "ERROR: %s", (char *)scriptErrorMessage);
    } else {
        int8_t *tempPath = scriptErrorLine.scriptBody->path;
        int64_t tempFileNameIndex = strlen((char *)tempPath);
        while (tempFileNameIndex > 0) {
            int8_t tempCharacter = tempPath[tempFileNameIndex - 1];
            if (tempCharacter == '/') {
                break;
            }
            tempFileNameIndex -= 1;
        }
        sprintf(
            (char *)tempText,
            "ERROR: %s (Line %lld, %s)",
            (char *)scriptErrorMessage,
            scriptErrorLine.number,
            (char *)(tempPath + tempFileNameIndex)
        );
    }
    notifyUser(tempText);
}

int8_t runScript(int8_t *path) {
    resetScriptError();
    importScript(path);
    if (scriptHasError) {
        displayScriptError();
    }
    return !scriptHasError;
}

int8_t invokeKeyBinding(int32_t key) {
    vector_t tempArgumentList;
    createEmptyVector(&tempArgumentList, sizeof(scriptValue_t));
    resetScriptError();
    int8_t output = false;
    int64_t index = 0;
    while (index < keyBindingList.length) {
        keyBinding_t *tempKeyBinding = findVectorElement(&keyBindingList, index);
        if (tempKeyBinding->key == key) {
            scriptValue_t tempResult = invokeFunction(tempKeyBinding->callback, &tempArgumentList);
            if (scriptHasError) {
                displayScriptError();
                output = true;
                break;
            }
            if (tempResult.type != SCRIPT_VALUE_TYPE_NUMBER) {
                notifyUser((int8_t *)"ERROR: Key binding must return boolean.");
                output = true;
                break;
            }
            int8_t tempShouldOverride = *(double *)&(tempResult.data);
            if (tempShouldOverride) {
                output = true;
                break;
            }
        }
        index += 1;
    }
    cleanUpVector(&tempArgumentList);
    return output;
}

int32_t invokeKeyMapping(int32_t key) {
    keyMapping_t *tempKeyMapping = findKeyMapping(key, activityMode);
    if (tempKeyMapping != NULL) {
        return tempKeyMapping->newKey;
    }
    return key;
}

int8_t invokeCommandBinding(scriptValue_t *destination, int8_t **termList, int32_t termListLength) {
    commandBinding_t *tempCommandBinding = findCommandBinding(termList[0]);
    if (tempCommandBinding == NULL) {
        return false;
    }
    setActivityMode(COMMAND_MODE);
    vector_t *tempArgumentList = malloc(sizeof(vector_t));
    createEmptyVector(tempArgumentList, sizeof(scriptValue_t));
    int64_t index = 1;
    while (index < termListLength) {
        scriptValue_t tempValue = convertTextToStringValue(termList[index]);
        pushVectorElement(tempArgumentList, &tempValue);
        index += 1;
    }
    scriptHeapValue_t *tempHeapValue = createScriptHeapValue();
    tempHeapValue->type = SCRIPT_VALUE_TYPE_LIST;
    *(vector_t **)&(tempHeapValue->data) = tempArgumentList;
    scriptValue_t tempListValue;
    tempListValue.type = SCRIPT_VALUE_TYPE_LIST;
    *(scriptHeapValue_t **)&(tempListValue.data) = tempHeapValue;
    vector_t tempSingleArgumentList;
    createEmptyVector(&tempSingleArgumentList, sizeof(scriptValue_t));
    pushVectorElement(&tempSingleArgumentList, &tempListValue);
    resetScriptError();
    scriptValue_t tempResult = invokeFunction(tempCommandBinding->callback, &tempSingleArgumentList);
    cleanUpVector(&tempSingleArgumentList);
    if (scriptHasError) {
        displayScriptError();
    }
    if (destination != NULL) {
        *destination = tempResult;
    }
    return true;
}
