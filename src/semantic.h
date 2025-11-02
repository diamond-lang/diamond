#ifndef semantic_h
#define semantic_h

#include "arena.h"
#include "program.h"

void analyzeModulesInterfaces(Program* program, Arena scracth);
void analyzeModule(Program* program, uint32_t astId, Arena scratch);

#endif