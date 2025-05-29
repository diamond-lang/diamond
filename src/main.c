#include <assert.h>
#include <stddef.h>
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

    // for (size_t i = 0; i < ast.literals.count; i++) {
    //     if (ast.literals.items[i] == '\0' && i + 1 != ast.literals.count) {
    //         printf("•");
    //     } else {
    //         printf("%c", ast.literals.items[i]);
    //     }
    // }
    // printf("\n");

    arena_destroyCurrentLifetime();
    arena_assertNoLifetimesRemaining();

    return 0;
}