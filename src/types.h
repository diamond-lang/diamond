#ifndef types_h
#define types_h

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"  // IWYU pragma: keep

// List
#define ListType(T)        \
    struct {               \
        T* buffer;         \
        uint32_t size;     \
        uint32_t capacity; \
        uint32_t lifetime; \
    }

#define List() {NULL, 0, 0, arena_currentLifetime()}
#define ListWithLifetime(lifetime) {NULL, 0, 0, lifetime}

#define list_get(list, index) \
    (assert(index < (list).size), &(list).buffer[index])

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

#define list_removeItemAtIndex(list, index)                              \
    do {                                                                 \
        assert(index < (list).size);                                     \
        uint32_t sizeMinusOne__ = (list).size > 0 ? (list).size - 1 : 0; \
        for (uint32_t i__ = index; i__ < sizeMinusOne__; i__++) {        \
            (list).buffer[i__] = (list).buffer[i__ + 1];                 \
        }                                                                \
        if (index < (list).size) {                                       \
            (list).size--;                                               \
        }                                                                \
    } while (false);

#define list_removeFirstMatch(list, item)                                    \
    do {                                                                     \
        if ((list).size > 0) {                                               \
            uint32_t i__;                                                    \
            for (i__ = 0; i__ < (list).size; i__) {                          \
                if ((list).buffer[i__] == item) {                            \
                    break;                                                   \
                }                                                            \
            }                                                                \
            uint32_t sizeMinusOne__ = (list).size > 0 ? (list).size - 1 : 0; \
            for (uint32_t j__ = i__; j__ < sizeMinusOne__; j__++) {          \
                (list).buffer[j__] = (list).buffer[j__ + 1];                 \
            }                                                                \
            if (i__ < (list).size) {                                         \
                (list).size--;                                               \
            }                                                                \
        }                                                                    \
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

typedef ListType(uint8_t) Uint8List;
typedef ListType(uint32_t) Uint32List;
typedef ListType(Uint32List) Uint32ListList;

// Stack
#define StackType(T) ListType(T)

#define stack_size(stack) list_size(stack)

#define stack_get(stack, index) list_get(stack, index)

#define stack_push(stack, item) list_append(stack, item)

#define stack_pop(stack) list_setSize(stack, list_size(stack) - 1)

#define stack_top(stack) list_get(stack, list_size(stack) - 1)

#define Stack() List()

typedef StackType(uint32_t) Uint32Stack;
typedef StackType(Uint32List) Uint32ListStack;

// String
typedef ListType(char) CharList;

typedef struct {
    CharList buffer;
} String;

typedef struct {
    uint32_t length;
    char* pointer;
} StringView;

StringView cStringAsView(char* string);

uint32_t string_size(String string);
void string_clear(String* string);
char* string_asCString(String string);
char string_get(String string, uint32_t index);
void string_append(String* string, char item);
void string_concat(String* string, StringView toConcat);
bool string_equal(StringView a, StringView b);
StringView string_asView(String string);

#define String() \
    (String) { (CharList) List() }

typedef ListType(String) StringList;

// HashMap
#define HashmapType(T)     \
    struct {               \
        uint32_t* keys;    \
        T* values;         \
        uint32_t size;     \
        uint32_t capacity; \
        uint32_t lifetime; \
    }

#define Hashmap() {NULL, NULL, 0, 0, arena_currentLifetime()}
#define hashmap_size(hashmap) (hashmap).size
uint32_t _hashmap_findLocation(uint32_t* keys, uint32_t key, uint32_t capacity);
#define hashmap_get(hashmap, key)                                  \
    (((hashmap).size != 0 && (hashmap).keys[_hashmap_findLocation( \
                                 (hashmap).keys,                   \
                                 key,                              \
                                 (hashmap).capacity                \
                             )] != None())                         \
         ? &(hashmap).values[_hashmap_findLocation(                \
               (hashmap).keys,                                     \
               key,                                                \
               (hashmap).capacity                                  \
           )]                                                      \
         : NULL)
#define hashmap_set(hashmap, key, value)                                    \
    do {                                                                    \
        if ((hashmap).size + 1 > (hashmap).capacity * 0.7) {                \
            uint32_t newCapacity__ = 0;                                     \
            if ((hashmap).capacity == 0) {                                  \
                newCapacity__ = 256;                                        \
            } else {                                                        \
                newCapacity__ = (hashmap).capacity * 2;                     \
            }                                                               \
            uint32_t sizeOfValue__ = sizeof(*(hashmap).values);             \
            uint32_t* newKeys__ = malloc(sizeof(uint32_t) * newCapacity__); \
            void* newValues__ = malloc(sizeOfValue__ * newCapacity__);      \
            assert(newKeys__);                                              \
            assert(newValues__);                                            \
            for (uint32_t i__ = 0; i__ < newCapacity__; i__++) {            \
                newKeys__[i__] = None();                                    \
            }                                                               \
            if (newCapacity__ != 256) {                                     \
                for (uint32_t i__ = 0; i__ < (hashmap).capacity; i__++) {   \
                    if ((hashmap).keys[i__] != None()) {                    \
                        uint32_t location__ = _hashmap_findLocation(        \
                            newKeys__,                                      \
                            (hashmap).keys[i__],                            \
                            newCapacity__                                   \
                        );                                                  \
                        newKeys__[location__] = (hashmap).keys[i__];        \
                        memcpy(                                             \
                            newValues__ + location__ * sizeOfValue__,       \
                            (hashmap).values + i__,                         \
                            sizeOfValue__                                   \
                        );                                                  \
                    }                                                       \
                }                                                           \
            }                                                               \
            arena_swapAllocations(                                          \
                (hashmap).lifetime,                                         \
                (void**)&(hashmap).keys,                                    \
                newKeys__                                                   \
            );                                                              \
            arena_swapAllocations(                                          \
                (hashmap).lifetime,                                         \
                (void**)&(hashmap).values,                                  \
                newValues__                                                 \
            );                                                              \
            (hashmap).capacity = newCapacity__;                             \
        }                                                                   \
        uint32_t location__ =                                               \
            _hashmap_findLocation((hashmap).keys, key, (hashmap).capacity); \
        if ((hashmap).keys[location__] == None()) {                         \
            (hashmap).size++;                                               \
        }                                                                   \
        (hashmap).keys[location__] = key;                                   \
        (hashmap).values[location__] = value;                               \
    } while (false)

typedef HashmapType(uint32_t) Uint32Hashmap;

#endif