#ifndef arena_h
#define arena_h

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#define getAlignOfExpression(expression) \
    offsetof(                            \
        struct {                         \
            char x;                      \
            __typeof__(expression) test; \
        },                               \
        test                             \
    )
#else
#define getAlignOfExpression(expression) (2 * sizeof(void*))
#endif

typedef struct {
    uint8_t* buffer;
    uint32_t offset;
} Arena;

Arena arena_new();
void* arena_allocWithAlignment(
    Arena* arena, uint32_t size, uint32_t alignment, uint32_t count
);
#define arena_alloc(arena, T, count) \
    ((T*)arena_allocWithAlignment(   \
        arena,                       \
        sizeof(T),                   \
        getAlignOfExpression(T),     \
        count                        \
    ))
void arena_free(Arena* arena);
uint32_t arena_getId(Arena arena, void* pointer);
void* _arena_getPointer(Arena arena, uint32_t alignment, uint32_t id);
#define arena_getPointer(arena, T, id) \
    ((T*)_arena_getPointer(arena, getAlignOfExpression(T), id))

#endif