#ifndef scopes_h
#define scopes_h

#include "types.h"

void scopes_addScope(Uint32ListStack scopes);
void scopes_removeScope();
uint32_t scopes_getBinding(uint32_t literalId);  // returns a node id
void scopes_importModule(uint32_t literalId);

#endif