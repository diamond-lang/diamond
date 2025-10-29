#ifndef types_h
#define types_h

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"  // IWYU pragma: keep

#define None() 0

// List
#define ListType(T)           \
    struct {                  \
        T** chunks;           \
        uint32_t chunksCount; \
        uint32_t count;       \
    }
typedef ListType(uint8_t) Uint8List;
typedef ListType(uint32_t) Uint32List;
typedef ListType(Uint32List) Uint32ListList;
typedef ListType(char*) CStringList;

#define list_initialChunkSize 256

#define list_capacity(list) \
    list_initialChunkSize * ((1 << (list).chunksCount) - 1)

#define list_getChunk(index) \
    (31 - __builtin_clz((index + list_initialChunkSize) >> 8))

#define _list_get(list, index)                                                 \
    (&(list).chunks[list_getChunk(index                                        \
    )][(index) - list_totalCapacityPreviousChunks(list, list_getChunk(index))] \
    )

#define list_get(list, index) \
    (assert(((uint64_t)index) < (list).count), _list_get(list, index))

#define list_size(list) ((list).count)

#define list_totalCapacityPreviousChunks(list, chunk) \
    (list_initialChunkSize * ((1 << (chunk)) - 1))

#define list_grow(arena, list)                                        \
    do {                                                              \
        if (list_capacity(list) == 0) {                               \
            (list).chunks = arena_allocWithAlignment(                 \
                arena,                                                \
                sizeof(*(list).chunks),                               \
                getAlignOfExpression(*(list).chunks),                 \
                24                                                    \
            );                                                        \
        }                                                             \
        (list).chunks[(list).chunksCount] = arena_allocWithAlignment( \
            arena,                                                    \
            sizeof(**(list).chunks),                                  \
            getAlignOfExpression(**(list).chunks),                    \
            list_initialChunkSize * (1 << (list).chunksCount)         \
        );                                                            \
        (list).chunksCount += 1;                                      \
        assert((list).chunksCount <= 24);                             \
    } while (false)

#define list_append(arena, list, item)                  \
    do {                                                \
        uint32_t totalCapacity__ = list_capacity(list); \
        if ((list).count >= totalCapacity__) {          \
            list_grow(arena, list);                     \
        }                                               \
        *_list_get(list, list_size(list)) = item;       \
        (list).count += 1;                              \
    } while (false)

#define list_ensureExtraCapacity(arena, list, extraCapacity) \
    do {                                                     \
        uint32_t capacity__ = list_capacity(list);           \
        while (capacity__ < (list).count + extraCapacity) {  \
            list_grow(arena, list);                          \
            capacity__ = list_capacity(list);                \
        }                                                    \
    } while (false)

#define list_removeIndex(list, index)                                          \
    do {                                                                       \
        if ((list).count > 0) {                                                \
            uint32_t sizeMinusOne__ = (list).count > 0 ? (list).count - 1 : 0; \
            for (uint32_t j__ = index; j__ < sizeMinusOne__; j__++) {          \
                *list_get(list, j__) = *list_get(list, j__ + 1);               \
            }                                                                  \
            if (index < (list).count) {                                        \
                (list).count--;                                                \
            }                                                                  \
        }                                                                      \
    } while (false)

#define list_removeFirstMatch(list, item)                                      \
    do {                                                                       \
        if ((list).count > 0) {                                                \
            uint32_t i__;                                                      \
            for (i__ = 0; i__ < (list).count; i__++) {                         \
                if (*list_get(list, i__) == item) {                            \
                    break;                                                     \
                }                                                              \
            }                                                                  \
            uint32_t sizeMinusOne__ = (list).count > 0 ? (list).count - 1 : 0; \
            for (uint32_t j__ = i__; j__ < sizeMinusOne__; j__++) {            \
                *list_get(list, j__) = *list_get(list, j__ + 1);               \
            }                                                                  \
            if (i__ < (list).count) {                                          \
                (list).count--;                                                \
            }                                                                  \
        }                                                                      \
    } while (false)

#define list_setSize(list, newSize)             \
    do {                                        \
        assert(newSize <= list_capacity(list)); \
        (list).count = newSize;                 \
    } while (false)

// Stack
#define StackType(T) ListType(T)
typedef StackType(uint32_t) Uint32Stack;
typedef StackType(Uint32List) Uint32ListStack;

#define stack_size(stack) list_size(stack)
#define stack_get(stack, index) list_get(stack, index)
#define stack_push(arena, stack, item) list_append(arena, stack, item)
#define stack_pop(stack) list_setSize(stack, list_size(stack) - 1)
#define stack_top(stack) list_get(stack, list_size(stack) - 1)

// String
typedef struct {
    char* buffer;
    uint32_t count;
    uint32_t capacity;
} String;

typedef struct {
    uint32_t length;
    char* pointer;
} StringView;

StringView cStringAsView(char* string);
bool string_isLowerCase(StringView view);

uint32_t string_size(String string);
void string_clear(String* string);
char* string_asCString(String string);
char string_get(String string, uint32_t index);
void string_append(Arena* arena, String* string, char item);
void string_concat(Arena* arena, String* string, StringView toConcat);
bool string_equal(StringView a, StringView b);
char* string_pointer(String string);
void string_ensureExtraCapacity(
    Arena* arena, String* string, uint32_t extraCapacity
);
StringView string_asView(String string);
String string_substring(
    Arena* arena, String string, uint32_t start, uint32_t length
);

typedef ListType(String) StringList;

// ArrayHashmap
#define ArrayHashmapType(T) \
    struct {                \
        Uint32List keys;    \
        ListType(T) values; \
    }

typedef ArrayHashmapType(uint32_t) Uint32Hashmap;

#define array_hashmap_size(hashmap) list_size((hashmap).keys)

#define array_hashmap_capacity(hashmap) list_capacity((hashmap).keys)

uint64_t _array_hashmap_findLocation(Uint32List keys, uint32_t key);

#define array_hashmap_get(hashmap, key)                         \
    (_array_hashmap_findLocation((hashmap).keys, key) <         \
             list_size((hashmap).keys)                          \
         ? list_get(                                            \
               (hashmap).values,                                \
               _array_hashmap_findLocation((hashmap).keys, key) \
           )                                                    \
         : NULL)

#define array_hashmap_set(arena, hashmap, key, value)      \
    do {                                                   \
        uint32_t i__;                                      \
        for (i__ = 0; i__ < (hashmap).keys.count; i__++) { \
            if (*list_get((hashmap).keys, i__) == key) {   \
                break;                                     \
            }                                              \
        }                                                  \
        if (i__ < list_size((hashmap).keys)) {             \
            *list_get((hashmap).keys, i__) = value;        \
        } else {                                           \
            list_append(arena, (hashmap).keys, key);       \
            list_append(arena, (hashmap).values, value);   \
        }                                                  \
    } while (false)

#endif