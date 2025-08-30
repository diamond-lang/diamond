#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "compile.h"
#include "parser.h"
#include "program.h"
#include "types.h"
#include "utilities.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    // uint32_t* p = NULL;
    // {
    //     uint32_t a = 10;
    //     p = &a;
    // }
    // printf("%d", *p);

    // Program program = (Program){List()};
    // list_append(program.asts, (Ast){});
    // initAst(
    //     list_get(program.asts, 0),
    //     getCanonicalPath((StringView){strlen(argv[1]), argv[1]})
    // );
    // parse(list_get(program.asts, 0));

    // Compile program
    Arena arena = arena_new();
    Arena scratch = arena_new();
    (void)compile(&arena, cStringAsView(argv[1]), scratch);

    return 0;
}