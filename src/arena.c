#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common.h"

#define arena_size() UINT32_MAX

Arena arena_new() {
    Arena arena = {0};
    arena.buffer = malloc(arena_size());
    assert(arena.buffer);
    return arena;
}
void arena_init(Arena* arena) {
    arena->buffer = NULL;
    arena->offset = 0;
}

static bool isPowerOfTwo(uintptr_t x) { return (x & (x - 1)) == 0; }

static uint32_t paddingNeeded(Arena* arena, uint32_t alignment) {
    assert(isPowerOfTwo(alignment));
    uintptr_t pointer = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    uintptr_t modulo = pointer & (alignment - 1);
    return modulo == 0 ? 0 : alignment - modulo;
}

void* arena_allocWithAlignment(
    Arena* arena, uint32_t size, uint32_t alignment, uint32_t count
) {
    assert(arena && arena->buffer);
    uint32_t padding = paddingNeeded(arena, alignment);
    if ((uint64_t)arena->offset + padding + size * count < UINT32_MAX) {
        void* result = &arena->buffer[arena->offset + padding];
        assert(((uintptr_t)result % alignment) == 0);
        arena->offset += padding + size * count;
        memset(result, 0, padding + size * count);
        return result;
    } else {
        unreachable();
    }
    return NULL;
}

void arena_free(Arena* arena) {
    free(arena->buffer);
    *arena = (Arena){0};
}

uint32_t arena_getId(Arena arena, void* pointer) {
    uintptr_t buffer = (uintptr_t)arena.buffer;
    uintptr_t ptr = (uintptr_t)pointer;
    assert(buffer <= ptr && ptr < (buffer + UINT32_MAX));
    return ptr - buffer + 1;
}

void* arena_getPointer(Arena arena, uint32_t id) {
    uint64_t id64 = id;
    assert(0 < id64 && id64 <= UINT32_MAX);
    uintptr_t pointer = (uintptr_t)arena.buffer + (id - 1);
    return (void*)pointer;
}
