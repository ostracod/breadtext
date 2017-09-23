
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "scriptValue.h"

scriptHeapValue_t *firstScriptHeapValue = NULL;

int8_t loadScriptBody(scriptBody_t *destination, int8_t *path) {
    FILE *tempFile = fopen((char *)path, "r");
    if (tempFile == NULL) {
        return false;
    }
    destination->path = malloc(strlen((char *)path) + 1);
    strcpy((char *)(destination->path), (char *)path);
    fseek(tempFile, 0, SEEK_END);
    destination->length = ftell(tempFile);
    destination->text = malloc(destination->length + 1);
    fseek(tempFile, 0, SEEK_SET);
    fread(destination->text, 1, destination->length, tempFile);
    (destination->text)[destination->length] = 0;
    fclose(tempFile);
    return true;
}
