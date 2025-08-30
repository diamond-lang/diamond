#ifndef codegen_h
#define codegen_h

#include "arena.h"
#include "program.h"

void generateObjectCode(
    Program program, uint32_t astId, Arena scratch, Arena otherScratch
);
void generateExecutable(Program program, Arena scratch);

#endif