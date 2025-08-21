#ifndef arena_h
#define arena_h

#include <stddef.h>
#include <stdlib.h>

typedef struct {
    uint8_t* buffer;
    uint32_t offset;
} Arena;

Arena arena_new();
void* arena_alloc(
    Arena* arena, uint32_t size, uint32_t alignment, uint32_t count
);
#define alloc(arena, T, count) \
    (T*)arena_alloc(arena, sizeof(T), alignof(T), count)
void arena_free(Arena* arena);
uint32_t arena_getOffset(Arena arena, void* pointer);

#endif