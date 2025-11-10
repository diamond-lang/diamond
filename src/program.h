#ifndef program_h
#define program_h

#include <stdint.h>

#include "ast.h"
#include "types.h"

typedef struct {
    Ast builtin;
    AstList asts;
    StringList paths;
    Uint32ListList dependencyGraph;
} Program;

Ast* program_getAst(Program* program, uint32_t astId);
String program_getPath(Program* program, uint32_t astId);
bool program_areTypesEqual(Program* program, TypeReference a, TypeReference b);

#endif