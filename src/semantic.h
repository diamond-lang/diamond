#ifndef semantic_h
#define semantic_h

#include "arena.h"
#include "program.h"

bool analyze(
    Program program, uint32_t astId, Arena scratch, Arena otherScratch
);

#endif