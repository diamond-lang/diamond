#include <assert.h>
#include <stdio.h>

#include "lexer.h"
#include "token.h"
#include "types.h"
#include "utilities.h"

int main(int argc, char* argv[]) {
    assert(argc == 2);

    String source = readFile(argv[1]);
    ErrorList errors = List();
    TokenList tokens = lex(source, &errors);
    token_print(tokens);

    return 0;
}