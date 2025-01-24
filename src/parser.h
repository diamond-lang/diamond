#ifndef parser_h
#define parser_h

#include "ast.h"
#include "token.h"

Ast parse(TokenList source, ErrorList* errors);

#endif