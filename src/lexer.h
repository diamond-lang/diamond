#ifndef lexer_h
#define lexer_h

#include "common.h"
#include "types.h"

TokenList lex(String source, ErrorList* errors);

#endif