#ifndef utilities_h
#define utilities_h

#include "arena.h"
#include "types.h"

String getWorkingDirectory(Arena* arena);
String getCanonicalPath(Arena* arena, StringView string, Arena scratch);
String readFile(Arena* arena, char* path);
int numberOfDigits(uint32_t number);

#endif