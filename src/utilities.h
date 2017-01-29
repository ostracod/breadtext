
#ifndef UTILITIES_HEADER_FILE
#define UTILITIES_HEADER_FILE

#define true 1
#define false 0

#define PLATFORM_MAC 1
#define PLATFORM_LINUX 2

int8_t *clipboardFilePath;
int8_t applicationPlatform;

void copyData(int8_t *destination, int8_t *source, int64_t amount);
int8_t equalData(int8_t *source1, int8_t *source2, int64_t amount);
int8_t isWhitespace(int8_t character);
int8_t *findWhitespace(int8_t *text);
int8_t *skipWhitespace(int8_t *text);
int64_t removeBadCharacters(int8_t *text, int8_t *containsNewline);
void convertTabsToSpaces(int8_t *text);
int8_t isWordCharacter(int8_t tempCharacter);
int8_t *mallocRealpath(int8_t *path);
void systemCopyClipboardFile();
void systemPasteClipboardFile();

// UTILITIES_HEADER_FILE
#endif

