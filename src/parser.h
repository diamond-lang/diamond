#ifndef parser_h
#define parser_h

#include "ast.h"

void parse(Ast* ast, char* source);
void parseImports(Ast* ast, char* source);
void parseBuiltin(Ast* ast, char* source);

#endif