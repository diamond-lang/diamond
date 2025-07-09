#ifndef utilities_h
#define utilities_h

#include "types.h"

void normalizePath(String* path);
String getWorkingDirectory();
String getCanonicalPath(StringView string);
String readFile(char* path);
int numberOfDigits(uint32_t number);

#endif