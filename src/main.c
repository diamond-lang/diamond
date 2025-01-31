#include <assert.h>
#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "types.h"
#include "utilities.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    arena_init();

    String source = readFile(argv[1]);
    ErrorList errors = List();
    TokenList tokens = lex(source, &errors);
    token_print(tokens);

    Ast ast = parse(tokens, &errors);
    ast.errors = errors;
    ast.tokens = tokens;
    ast.filePath = argv[1];
    if (errors.count == 0) {
        ast_print(ast);
    } else {
        reportErrors(ast);
    }

    arena_freeAll();

    return 0;
}