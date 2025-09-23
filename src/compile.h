#ifndef compile_h
#define compile_h

#include <stddef.h>

#include "arena.h"
#include "program.h"

Program getProgramGraph(Arena* arena, StringView file, Arena scratch);
void parseProgram(Program* program, Arena scratch);
void analyzeProgram(Program* program, Arena scratch1, Arena scratch2);
void codegenObjectFiles(Program program, Arena scratch1, Arena scratch2);
void linkProgram(Program program, Arena scratch);

#endif