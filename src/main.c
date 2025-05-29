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

    Program program = (Program){List()};
    list_append(program.asts, (Ast){});
    initAst(
        list_get(program.asts, 0),
        getCanonicalPath((StringView){strlen(argv[1]), argv[1]})
    );
    parse(list_get(program.asts, 0));

    // Compile program
    // Program program = compile(argv[1]);
    // if (list_size(list_get(program.asts, 0)->errors) != 0) {
    //     reportErrors(*list_get(program.asts, 0));
    //     exit(EXIT_FAILURE);
    // }

    ast_print(*list_get(program.asts, 0));

    CharList list = List();
    list_append(list, '6');
    list_append(list, '6');
    list_append(list, '6');

    // for (size_t i = 0; i < ast.literals.count; i++) {
    //     if (ast.literals.items[i] == '\0' && i + 1 != ast.literals.count) {
    //         printf("•");
    //     } else {
    //         printf("%c", ast.literals.items[i]);
    //     }
    // }
    // printf("\n");

    arena_destroyAllLifetimes();

    return 0;
}