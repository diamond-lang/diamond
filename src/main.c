#include <assert.h>
#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "types.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    arena_init();

    // Create ast
    Ast ast;
    ast.filePath = argv[1];
    ast.tokens = (TokenList)List();
    ast.errors = (ErrorList)List();
    ast.nodes = (AstNodeList)List();

    // Lex
    lex(&ast);
    if (ast.errors.count != 0) {
        reportErrors(ast);
        exit(EXIT_FAILURE);
    }

    // Parse
    parse(&ast);
    if (ast.errors.count != 0) {
        reportErrors(ast);
        exit(EXIT_FAILURE);
    }

    ast_print(ast);

    arena_freeAll();

    return 0;
}