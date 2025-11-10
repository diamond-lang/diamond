#ifndef codegen_h
#define codegen_h

#include "arena.h"
#include "program.h"

void generateObjectCode(
    Program* program,
    uint32_t astId,
    String objectFileName,
    String path,
    bool isEntry,
    Arena scratch
);
void linkObjectFiles(
    String executableName, StringList objectFiles, Arena scratch
);
void printLLVMIR(Program* program, uint32_t astId, String path, Arena scratch);

#endif