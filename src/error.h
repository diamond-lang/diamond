#ifndef error_h
#define error_h

#include <stddef.h>

#include "types.h"

typedef enum {
    FILE_NOT_FOUND,

    // Lexer
    UNRECOGNIZED_CHARACTER,
    UNCLOSE_BLOCK_COMMENT,

    // Syntatic
    UNEXPECTED_CHARACTER,
    UNEXPECTED_IDENTATION,
    EXPECTING_STATEMENT,
    EXPECTING_NEW_IDENTATION_LEVEL,
    UDENFINED_VARIABLE,

    // Semantic
    REASSIGNING_IMMUTABLE_VARIABLE,
    UDENFINED_FUNCTION,
    UNHANDLED_RETURN_VALUE,
} ErrorKind;

typedef struct {
    ErrorKind kind;
    size_t line;
    size_t column;
} Error;

typedef ListType(Error) ErrorList;

void reportError(Error error);

#endif