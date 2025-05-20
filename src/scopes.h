#ifndef scopes_h
#define scopes_h

#include "ast.h"
#include "types.h"

typedef ListType(NodeId) Scope;
typedef StackType(Scope) Scopes;

void scopes_addScope();
void scopes_removeScope();
NodeId scopes_getBinding(LiteralId identifier);
void scopes_importModule(LiteralId identifier);

#endif