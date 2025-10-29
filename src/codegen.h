#ifndef codegen_h
#define codegen_h

#include "arena.h"
#include "program.h"

void generateObjectCode(
    Program* program,
    uint32_t astId,
    String objectFileName,
    bool isEntry,
    Arena scratch
);
void linkObjectFiles(
    String executableName, StringList objectFiles, Arena scratch
);
void printLLVMIR(Program* program, uint32_t astId, Arena scratch);

#endif