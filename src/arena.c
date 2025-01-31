#include "arena.h"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

typedef ListType(void *) PointerList;

PointerList arena;

#define arena_append(list, item)                                          \
    do {                                                                  \
        if (list.count >= list.capacity) {                                \
            if (list.capacity == 0) {                                     \
                list.capacity = 256;                                      \
            } else {                                                      \
                list.capacity *= 2;                                       \
            }                                                             \
            list.items =                                                  \
                realloc(list.items, list.capacity * sizeof(*list.items)); \
        }                                                                 \
        list.items[list.count] = item;                                    \
        list.count += 1;                                                  \
    } while (false);

void arena_init() { arena = (PointerList)List(); }

void *arena_alloc(size_t size) {
    void *result = malloc(size);
    arena_append(arena, result);
    return result;
}

void *arena_realloc(void *pointer, size_t newSize) {
    void *result = realloc(pointer, newSize);
    assert(result != NULL);

    if (pointer == NULL) {
        arena_append(arena, result);
    } else {
        for (size_t i = 0; i < arena.count; i++) {
            if (arena.items[i] == pointer) {
                arena.items[i] = result;
            }
        }
    }

    return result;
}

void arena_free(void *pointer) {
    for (size_t i = 0; i < arena.count; i++) {
        if (arena.items[i] == pointer) {
            free(arena.items[i]);
            arena.items[i] = NULL;
            break;
        }
    }
}

void arena_freeAll() {
    for (size_t i = 0; i < arena.count; i++) {
        if (arena.items[i] != NULL) {
            free(arena.items[i]);
        }
    }
    free(arena.items);
}