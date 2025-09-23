#ifndef codegen_h
#define codegen_h

#include "arena.h"
#include "program.h"

void generateObjectCode(
    Program program, uint32_t astId, Arena scratch1, Arena scratch2
);
void generateExecutable(Program program, Arena scratch);
void printLLVMIR(
    Program program, uint32_t astId, Arena scratch1, Arena scratch2
);

#endif