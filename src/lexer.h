#ifndef lexer_h
#define lexer_h

#include "ast.h"
#include "token.h"
#include "types.h"

typedef struct {
    uint32_t line;
    uint32_t column;
    char* source;
    char current;
    char next;
    char nextNext;
    bool previousWasImport;
    String currentLiteral;
    Ast* ast;
} Lexer;

void initLexer(Lexer* lexer, char* source, Ast* ast);
Token scanToken(Lexer* lexer);

#endif