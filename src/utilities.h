#ifndef utilities_h
#define utilities_h

#include "types.h"

void normalizePath(String* path);
String getWorkingDirectory();
String getCanonicalPath(StringView string);
String readFile(char* path);
int numberOfDigits(size_t number);

#endif