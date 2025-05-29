#ifndef arena_h
#define arena_h

#include <stddef.h>
#include <stdlib.h>

void arena_newLifetime();
size_t arena_currentLifetime();
void* arena_realloc(size_t lifetime, void* pointer, size_t numberOfBytes);
void arena_destroyCurrentLifetime();
void arena_destroyAllLifetimes();

#endif