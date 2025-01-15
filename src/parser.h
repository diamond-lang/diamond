#ifndef parser_h
#define parser_h

#include "token.h"

TokenList parse(TokenList source, ErrorList* errors);

#endif