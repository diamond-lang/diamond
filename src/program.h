#ifndef program_h
#define program_h

#include "ast.h"
#include "types.h"

typedef struct {
    AstList asts;
    Uint32ListList dependencyGraph;
} Program;

#endif