#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "compile.h"
#include "parser.h"
#include "program.h"
#include "utilities.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    // Compile program
    Program program = compile(argv[1]);
    if (program.asts.items[0].errors.count != 0) {
        reportErrors(program.asts.items[0]);
        exit(EXIT_FAILURE);
    }

    ast_print(program.asts.items[0]);

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