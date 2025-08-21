#ifndef lexer_h
#define lexer_h

#include <stdint.h>

#include "ast.h"
#include "token.h"

typedef struct {
    uint32_t line;
    uint32_t column;
    char* source;
    char current;
    char next;
    char nextNext;
    bool previousWasImport;
    bool parsingBuiltins;
    char* initialSource;
    uint32_t start;
    uint32_t offset;
    Ast* ast;
} Lexer;

void initLexer(Lexer* lexer, char* source, Ast* ast, bool parsingBuiltins);
Token scanToken(Lexer* lexer);

#endif