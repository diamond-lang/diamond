#ifndef scopes_h
#define scopes_h

#include <stdint.h>

void scopes_addScope();
void scopes_removeScope();
uint32_t scopes_getBinding(uint32_t identifier);
void scopes_importModule(uint32_t identifier);

#endif