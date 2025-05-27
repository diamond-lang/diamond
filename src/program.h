#ifndef program_h
#define program_h

#include "ast.h"
#include "types.h"

typedef ListType(AstId) AstIdList;
typedef ListType(AstIdList) DependencyGraph;

typedef struct {
    AstList asts;
    DependencyGraph dependencyGraph;
} Program;

#endif