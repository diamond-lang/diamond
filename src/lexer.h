#ifndef lexer_h
#define lexer_h

#include "token.h"
#include "types.h"

typedef struct {
    uint32_t line;
    uint32_t column;
    char* source;
    Uint32List* literals;
    char current;
    char next;
    char nextNext;
    bool previousWasImport;
    String currentLiteral;
} Lexer;

void initLexer(Lexer* lexer, char* source, Uint32List* literals);
Token scanToken(Lexer* lexer);

#endif