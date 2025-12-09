#include "program.h"

#include "ast.h"

Ast* program_getAst(Program* program, uint32_t astId) {
    if (astId == 0) return &program->builtin;
    assert(1 <= astId && astId <= list_size(program->asts));
    return list_get(program->asts, astId - 1);
}

String program_getPath(Program* program, uint32_t astId) {
    assert(1 <= astId && astId <= list_size(program->paths));
    return *list_get(program->paths, astId - 1);
}

bool program_areTypesEqual(Program* program, TypeReference a, TypeReference b) {
    assert(a.parameterCount == 0 && b.parameterCount == 0);
    return a.moduleId == b.moduleId && a.id == b.id;
}
