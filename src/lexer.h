#ifndef lexer_h
#define lexer_h

#include "token.h"
#include "types.h"

typedef struct {
    size_t line;
    size_t column;
    String source;
    char* sourcePointer;
    Uint32List* literals;
    char current;
    char next;
    char nextNext;
    bool previousWasImport;
    String currentLiteral;
} Lexer;

void initLexer(Lexer* lexer, char* filePath, Uint32List* literals);
Token scanToken(Lexer* lexer);

#endif