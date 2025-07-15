#ifndef program_h
#define program_h

#include "ast.h"
#include "types.h"

typedef struct {
    AstList asts;
    StringList paths;
    Uint32ListList dependencyGraph;
} Program;

#endif