#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    // Parse
    Ast ast = parse(argv[1]);
    if (ast.errors.count != 0) {
        reportErrors(ast);
        exit(EXIT_FAILURE);
    }

    ast_print(ast);

    // for (size_t i = 0; i < ast.literals.count; i++) {
    //     if (ast.literals.items[i] == '\0' && i + 1 != ast.literals.count) {
    //         printf("•");
    //     } else {
    //         printf("%c", ast.literals.items[i]);
    //     }
    // }
    // printf("\n");

    return 0;
}