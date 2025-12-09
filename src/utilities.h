#ifndef utilities_h
#define utilities_h

#include <stdint.h>

#include "arena.h"
#include "types.h"

String getWorkingDirectory(Arena* arena);
String getCanonicalPath(Arena* arena, StringView importPath, Arena scratch);
String getRelativePath(Arena* arena, String from, String to, Arena scratch);
String getBasePath(Arena* arena, String path);
String getPathWithoutExtension(Arena* arena, String path);
String readFile(Arena* arena, char* path);
bool fileExists(char* path);
String getObjectFileName(
    Arena* arena, uint32_t astId, String canonicalPath, Arena scratch
);
String getBuiltinObjectFileName(Arena* arena);
String getExecutableName(Arena* arena, String path);
String getCommandToExecute(Arena* arena, String executableName);
String numberAsString(Arena* arena, uint32_t number);
int numberOfDigits(uint32_t number);

#endif