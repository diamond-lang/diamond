#ifndef types_h
#define types_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

// List
#define ListType(T)      \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define list_append(list, item)                     \
    do {                                            \
        if (list.count >= list.capacity) {          \
            if (list.capacity == 0) {               \
                list.capacity = 256;                \
            } else {                                \
                list.capacity *= 2;                 \
            }                                       \
            list.items = arena_realloc(             \
                list.items,                         \
                list.capacity * sizeof(*list.items) \
            );                                      \
        }                                           \
        list.items[list.count] = item;              \
        list.count += 1;                            \
    } while (false);

#define List() {NULL, 0, 0}

// Stack
#define StackType(T)     \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define stack_push(stack, item)                       \
    do {                                              \
        if (stack.count >= stack.capacity) {          \
            if (stack.capacity == 0) {                \
                stack.capacity = 256;                 \
            } else {                                  \
                stack.capacity *= 2;                  \
            }                                         \
            stack.items = arena_realloc(              \
                stack.items,                          \
                stack.capacity * sizeof(*stack.items) \
            );                                        \
        }                                             \
        stack.items[stack.count] = item;              \
        stack.count += 1;                             \
    } while (false);

#define stack_pop(stack)  \
    do {                  \
        stack.count -= 1; \
    } while (false);

#define stack_top(stack) stack.items[stack.count - 1]

#define Stack() {NULL, 0, 0}

typedef StackType(bool) BoolStack;
typedef StackType(size_t) SizeStack;

// String
typedef struct {
    char* content;
    size_t count;
    size_t capacity;
} String;

void string_append(String* string, char item);
#define string_equals(buffer1, buffer2) strcmp(buffer1, buffer2) == 0
String string_substring(String string, size_t start, size_t length);

#define String() (String){NULL, 0, 0}

#endif