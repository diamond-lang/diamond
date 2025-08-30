#ifndef utilities_h
#define utilities_h

#include <stdint.h>

#include "arena.h"
#include "types.h"

String getWorkingDirectory(Arena* arena);
String getCanonicalPath(Arena* arena, StringView importPath, Arena scratch);
String getBasePath(Arena* arena, String path);
String getPathWithoutExtension(Arena* arena, String path);
String readFile(Arena* arena, char* path);
String numberAsString(Arena* arena, uint32_t number);
int numberOfDigits(uint32_t number);

#endif