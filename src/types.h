#ifndef types_h
#define types_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"  // IWYU pragma: keep

// List
#define ListType(T)      \
    struct {             \
        T* buffer;       \
        size_t size;     \
        size_t capacity; \
        size_t lifetime; \
    }

#define List() {NULL, 0, 0, arena_currentLifetime()}
#define ListWithLifetime(lifetime) {NULL, 0, 0, lifetime}

void _list_get(void* buffer, size_t size, size_t index, int sizeOfItem);

#define list_get(list, index)                    \
    (index < (list).size ? &(list).buffer[index] \
                         : (assert(index < (list).size), &(list).buffer[0]))

#define list_size(list) ((list).size)

#define list_append(list, item)                          \
    do {                                                 \
        if ((list).size >= (list).capacity) {            \
            if ((list).capacity == 0) {                  \
                (list).capacity = 256;                   \
            } else {                                     \
                (list).capacity *= 2;                    \
            }                                            \
            (list).buffer = arena_realloc(               \
                (list).lifetime,                         \
                (list).buffer,                           \
                (list).capacity * sizeof(*(list).buffer) \
            );                                           \
        }                                                \
        (list).buffer[(list).size] = item;               \
        (list).size += 1;                                \
    } while (false);

#define list_setSize(list, newSize)         \
    do {                                    \
        assert(newSize <= (list).capacity); \
        (list).size = newSize;              \
    } while (false);

#define list_ensureExtraCapacity(list, extraCapacity)                       \
    do {                                                                    \
        if ((list).capacity == 0) (list).capacity = 256;                    \
        while ((list).size + extraCapacity > (list).capacity)               \
            (list).capacity *= 2;                                           \
        (list).buffer =                                                     \
            arena_realloc((list).lifetime, (list).buffer, (list).capacity); \
        assert((list).buffer != NULL);                                      \
    } while (false);

#define list_removeItemAtIndex(list, index)                            \
    do {                                                               \
        assert(index < (list).size);                                   \
        size_t sizeMinusOne__ = (list).size > 0 ? (list).size - 1 : 0; \
        for (size_t i__ = index; i__ < sizeMinusOne__; i__++) {        \
            (list).buffer[i__] = (list).buffer[i__ + 1];               \
        }                                                              \
        if (index < (list).size) {                                     \
            (list).size--;                                             \
        }                                                              \
    } while (false);

#define list_removeFirstMatch(list, item)                                  \
    do {                                                                   \
        if ((list).size > 0) {                                             \
            size_t i__;                                                    \
            for (i__ = 0; i__ < (list).size; i__) {                        \
                if ((list).buffer[i__] == item) {                          \
                    break;                                                 \
                }                                                          \
            }                                                              \
            size_t sizeMinusOne__ = (list).size > 0 ? (list).size - 1 : 0; \
            for (size_t j__ = i__; j__ < sizeMinusOne__; j__++) {          \
                (list).buffer[j__] = (list).buffer[j__ + 1];               \
            }                                                              \
            if (i__ < (list).size) {                                       \
                (list).size--;                                             \
            }                                                              \
        }                                                                  \
    } while (false);

#define list_clear(list) \
    do {                 \
        (list).size = 0; \
    } while (false);

#define list_concat(list, otherListPointer, otherListSize) \
    do {                                                   \
        list_ensureExtraCapacity(list, otherListSize);     \
        memcpy(                                            \
            (list).buffer + list_size(list),               \
            otherListPointer,                              \
            otherListSize                                  \
        );                                                 \
        (list).size += otherListSize;                      \
    } while (false);

// Stack
#define StackType(T) ListType(T)

#define stack_size(stack) list_size(stack)

#define stack_get(stack, index) list_get(stack, index)

#define stack_push(stack, item) list_append(stack, item)

#define stack_pop(stack) list_setSize(stack, list_size(stack) - 1)

#define stack_top(stack) list_get(stack, list_size(stack) - 1)

#define Stack() List()

typedef StackType(size_t) SizeTStack;

// String
typedef ListType(char) CharList;

typedef struct {
    CharList buffer;
} String;

typedef struct {
    size_t length;
    char* pointer;
} StringView;

StringView cStringAsView(char* string);

size_t string_size(String string);
void string_clear(String* string);
char* string_pointer(String string);
char string_get(String string, size_t index);
void string_append(String* string, char item);
void string_concat(String* string, StringView toConcat);
bool string_equal(StringView a, StringView b);
StringView string_asView(String string);

#define String() \
    (String) { (CharList) List() }

typedef ListType(String) StringList;

#endif