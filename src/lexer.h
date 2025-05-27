#ifndef lexer_h
#define lexer_h

#include "ast.h"

typedef struct {
    size_t line;
    size_t column;
    String source;
    char* sourcePointer;
    DataList* literals;
    ErrorList* errors;
    char current;
    char next;
    char nextNext;
    bool previousWasUseOrInclude;
    String currentLiteral;
} Lexer;

void initLexer(
    Lexer* lexer, char* filePath, DataList* literals, ErrorList* errors
);
Token scanToken(Lexer* lexer);
void lexer_free(Lexer* lexer);

#endif