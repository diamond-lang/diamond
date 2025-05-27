#ifndef types_h
#define types_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// List
#define ListType(T)      \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define list_append(list, item)                                           \
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

#define list_append2(list, item)                                             \
    do {                                                                     \
        if (list->count >= list->capacity) {                                 \
            if (list->capacity == 0) {                                       \
                list->capacity = 256;                                        \
            } else {                                                         \
                list->capacity *= 2;                                         \
            }                                                                \
            list->items =                                                    \
                realloc(list->items, list->capacity * sizeof(*list->items)); \
        }                                                                    \
        list->items[list->count] = item;                                     \
        list->count += 1;                                                    \
    } while (false);

#define list_setCapacity(list, newCapacity)                                    \
    do {                                                                       \
        list.capacity = newCapacity;                                           \
        list.items = realloc(list.items, list.capacity * sizeof(*list.items)); \
        if (list.capacity < list.count) list.count = list.capacity;            \
    } while (false);

#define list_ensureCapacity(list, extraCapacity)                          \
    do {                                                                  \
        while (list.count + extraCapacity > list.capacity) {              \
            if (list.capacity == 0) {                                     \
                list.capacity = 256;                                      \
            } else {                                                      \
                list.capacity *= 2;                                       \
            }                                                             \
            list.items =                                                  \
                realloc(list.items, list.capacity * sizeof(*list.items)); \
            assert(list.items != NULL);                                   \
        }                                                                 \
    } while (false);

#define list_removeFirstMatch(list, item)                            \
    do {                                                             \
        if (list.count > 0) {                                        \
            size_t i##item;                                          \
            for (i##item = 0; i##item < list.count; i##item++) {     \
                if (list.items[i##item] == item) {                   \
                    break;                                           \
                }                                                    \
            }                                                        \
            for (size_t j##item = i##item; j##item < list.count - 1; \
                 j##item++) {                                        \
                list.items[j##item] = list.items[j##item + 1];       \
            }                                                        \
            if (i##item < list.count) {                              \
                list.count--;                                        \
            }                                                        \
        }                                                            \
    } while (false);

#define List() {NULL, 0, 0}

// Stack
#define StackType(T)     \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define stack_push(stack, item)                                              \
    do {                                                                     \
        if (stack.count >= stack.capacity) {                                 \
            if (stack.capacity == 0) {                                       \
                stack.capacity = 256;                                        \
            } else {                                                         \
                stack.capacity *= 2;                                         \
            }                                                                \
            stack.items =                                                    \
                realloc(stack.items, stack.capacity * sizeof(*stack.items)); \
        }                                                                    \
        stack.items[stack.count] = item;                                     \
        stack.count += 1;                                                    \
    } while (false);

#define stack_pop(stack)  \
    do {                  \
        stack.count -= 1; \
    } while (false);

#define stack_top(stack) stack.items[stack.count - 1]

#define Stack() {NULL, 0, 0}

typedef StackType(size_t) SizeTStack;

// String
typedef struct {
    char* content;
    size_t count;
    size_t capacity;
} String;

typedef struct {
    size_t length;
    char* pointer;
} StringView;

void string_append(String* string, char item);
void string_concat(String* string, StringView toConcat);
#define string_equals(buffer1, buffer2) strcmp(buffer1, buffer2) == 0
String string_substring(String string, size_t start, size_t length);
void string_free(String string);
StringView string_asView(String string);

#define String() (String){NULL, 0, 0}

typedef ListType(String) StringList;

// HashTable
typedef struct {
    int32_t key;
    int32_t value;
} Bucket;

typedef struct {
    Bucket* content;
    size_t count;
    size_t capacity;
} HashTable;

#define HashTable() (HashTable){NULL, 0, 0};
int32_t* hashtable_get(HashTable hastable, int32_t key);
void hashtable_set(HashTable* hastable, int32_t key, int32_t value);

#endif