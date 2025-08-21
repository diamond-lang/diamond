#ifndef parser_h
#define parser_h

#include "arena.h"
#include "ast.h"

void parse(Ast* ast, char* source, Arena scratch);
void parseImports(Ast* ast, char* source, Arena scratch);
void parseBuiltin(Ast* ast, char* source, Arena scratch);

#endif