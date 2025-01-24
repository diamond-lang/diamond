#ifndef token_h
#define token_h

#include <stdlib.h>

#include "types.h"

// Token
typedef enum {
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    LEFT_CURLY,
    RIGHT_CURLY,
    COMMA,
    PLUS,
    SLASH,
    MODULO,
    STAR,
    MINUS,
    COLON,
    AMPERSAND,
    DOT,
    NOT,
    NOT_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    COLON_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    BE,
    INTEGER,
    FLOAT,
    IDENTIFIER,
    STRING,
    STRING_LEFT,
    STRING_MIDDLE,
    STRING_RIGHT,
    IF,
    ELSE,
    WHILE,
    FUNCTION,
    INTERFACE,
    BUILTIN,
    TYPE,
    CASE,
    TRUE,
    FALSE,
    OR,
    AND,
    USE,
    BREAK,
    CONTINUE,
    RETURN,
    MUT,
    NEW,
    INCLUDE,
    EXTERN,
    LINK_WITH,
    NEW_LINE,
    END_OF_FILE
} TokenKind;

typedef ListType(TokenKind) TokenKindList;

typedef struct {
    TokenKind kind;
    String literal;
    size_t line;
    size_t column;
} Token;

typedef ListType(Token) TokenList;

void token_print(TokenList tokens);
char* token_getLiteral(Token token);

#endif