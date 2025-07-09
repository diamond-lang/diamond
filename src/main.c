#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "compile.h"
#include "core.h"
#include "parser.h"
#include "program.h"
#include "types.h"
#include "utilities.h"

typedef HashmapType(uint32_t) Uint32HashMap;

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

    // for (uint32_t i = 0; i < ast.literals.count; i++) {
    //     if (ast.literals.items[i] == '\0' && i + 1 != ast.literals.count) {
    //         printf("•");
    //     } else {
    //         printf("%c", ast.literals.items[i]);
    //     }
    // }
    // printf("\n");

    printf("%s", core);

    Uint32HashMap map = (Uint32HashMap)Hashmap();
    for (uint32_t i = 1; i <= 1000000; i++) {
        hashmap_set(map, i, i);
    }

    printf("Map:\n");
    for (uint32_t i = 0; i < map.capacity; i++) {
        if (map.keys[i] != None()) {
            printf("    %u: %u\n", map.keys[i], map.values[i]);
        }
    }

    arena_destroyCurrentLifetime();
    arena_assertNoLifetimesRemaining();

    return 0;
}