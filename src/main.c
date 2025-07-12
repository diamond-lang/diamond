#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "compile.h"
#include "parser.h"
#include "program.h"
#include "types.h"
#include "utilities.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    arena_newLifetime();

    // Program program = (Program){List()};
    // list_append(program.asts, (Ast){});
    // initAst(
    //     list_get(program.asts, 0),
    //     getCanonicalPath((StringView){strlen(argv[1]), argv[1]})
    // );
    // parse(list_get(program.asts, 0));

    // Compile program
    (void)compile(cStringAsView(argv[1]));

    arena_destroyCurrentLifetime();
    arena_assertNoLifetimesRemaining();

    return 0;
}