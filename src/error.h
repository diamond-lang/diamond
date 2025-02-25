#ifndef error_h
#define error_h

#include "token.h"
#include "types.h"

typedef enum {
    FILE_NOT_FOUND,

    // Lexer
    UNRECOGNIZED_CHARACTER,

    // Parser
    EXPECTING_LINE_ENDING,
    UNEXPECTED_IDENTATION,
    EXPECTING_STATEMENT,
    EXPECTING_NEW_IDENTATION_LEVEL,
    UNEXPECTED_TOKEN,
    EXPECTING_EXPRESSION,

    // Semantic
    REASSIGNING_IMMUTABLE_VARIABLE,
    UDENFINED_FUNCTION,
    UNHANDLED_RETURN_VALUE,
    UDENFINED_VARIABLE,
} ErrorKind;

typedef struct {
    ErrorKind kind;
    size_t line;
    size_t column;

    union {
        struct {
            Token actualToken;
        } expectingLineEnding;
        struct {
            TokenKind expectedToken;
            TokenKind actualToken;
            char* beingParsed;
        } unexpectedToken;
        struct {
            TokenKind actualToken;
        } expectingExpression;
    };
} Error;

typedef ListType(Error) ErrorList;

#endif