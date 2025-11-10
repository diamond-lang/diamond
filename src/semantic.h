#ifndef semantic_h
#define semantic_h

#include "arena.h"
#include "program.h"

void analyzeModulesInterfaces(Program* program, Arena scracth1, Arena scratch2);
void analyzeModule(Program* program, uint32_t moduleId, Arena scratch);
void checkFunctionsUsed(Program* program, Arena scratch);

#endif