
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"
#include "scriptValue.h"

scriptBody_t *loadScriptBody(int8_t *path) {
    FILE *tempFile = fopen((char *)path, "r");
    if (tempFile == NULL) {
        return NULL;
    }
    scriptBody_t *output = malloc(sizeof(scriptBody_t));
    output->path = malloc(strlen((char *)path) + 1);
    strcpy((char *)(output->path), (char *)path);
    fseek(tempFile, 0, SEEK_END);
    output->length = ftell(tempFile);
    output->text = malloc(output->length + 1);
    fseek(tempFile, 0, SEEK_SET);
    fread(output->text, 1, output->length, tempFile);
    (output->text)[output->length] = 0;
    fclose(tempFile);
    return output;
}
