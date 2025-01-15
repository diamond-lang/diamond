#include <memory.h>
#include <stdlib.h>

void arena_init();
void *arena_alloc(size_t size);
void *arena_realloc(void *pointer, size_t newSize);
void arena_free();