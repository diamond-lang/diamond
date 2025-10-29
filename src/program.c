#include "program.h"

Ast* program_getAst(Program* program, uint32_t astId) {
    if (astId == 0) return &program->builtin;
    assert(1 <= astId && astId <= list_size(program->asts));
    return list_get(program->asts, astId - 1);
}

String program_getPath(Program* program, uint32_t astId) {
    assert(1 <= astId && astId <= list_size(program->paths));
    return *list_get(program->paths, astId - 1);
}