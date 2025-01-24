#ifndef lexer_h
#define lexer_h

#include "token.h"

TokenList lex(String source, ErrorList* errors);

#endif