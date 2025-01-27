#ifndef lexer_h
#define lexer_h

#include "error.h"
#include "token.h"

TokenList lex(String source, ErrorList* errors);

#endif