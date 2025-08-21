#ifndef compile_h
#define compile_h

#include <stddef.h>

#include "arena.h"
#include "program.h"

Program compile(Arena* arena, StringView file, Arena scratch);

#endif