#ifndef lexer_h
#define lexer_h

#include "ast.h"

typedef struct {
    size_t line;
    size_t column;
    char* source;
    Uint8List* literals;
    ErrorList* errors;
    char current;
    char next;
    char nextNext;
    String currentLiteral;
} Lexer;

void initLexer(
    Lexer* lexer, char* source, Uint8List* literals, ErrorList* errors
);
Token scanToken(Lexer* lexer);

#endif