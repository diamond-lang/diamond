#ifndef types_h
#define types_h

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// List
#define ListType(T)      \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define list_append(list, item)                                             \
    do {                                                                    \
        if (list.count >= list.capacity) {                                  \
            if (list.capacity == 0) {                                       \
                list.capacity = 256;                                        \
            } else {                                                        \
                list.capacity *= 2;                                         \
            }                                                               \
            list.items                                                      \
                = realloc(list.items, list.capacity * sizeof(*list.items)); \
        }                                                                   \
        list.items[list.count] = item;                                      \
        list.count += 1;                                                    \
    } while (false);

#define List() {NULL, 0, 0}

// Stack
#define StackType(T)     \
    struct {             \
        T* items;        \
        size_t count;    \
        size_t capacity; \
    }

#define stack_push(stack, item)                                                \
    do {                                                                       \
        if (stack.count >= stack.capacity) {                                   \
            if (stack.capacity == 0) {                                         \
                stack.capacity = 256;                                          \
            } else {                                                           \
                stack.capacity *= 2;                                           \
            }                                                                  \
            stack.items                                                        \
                = realloc(stack.items, stack.capacity * sizeof(*stack.items)); \
        }                                                                      \
        stack.items[stack.count] = item;                                       \
        stack.count += 1;                                                      \
    } while (false);

#define stack_pop(stack)  \
    do {                  \
        stack.count -= 1; \
    } while (false);

#define stack_top(stack) stack.items[stack.count - 1]

#define Stack() {NULL, 0, 0}

// String
typedef struct {
    char* content;
    size_t count;
    size_t capacity;
} String;

void string_append(String* string, char item);
String string_substring(String string, size_t start, size_t length);
#define string_equals(buffer1, buffer2) strcmp(buffer1, buffer2) == 0

#define String() (String){NULL, 0, 0}

// Token
typedef enum {
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    LEFT_CURLY,
    RIGHT_CURLY,
    COMMA,
    PLUS,
    SLASH,
    MODULO,
    STAR,
    MINUS,
    COLON,
    AMPERSAND,
    DOT,
    NOT,
    NOT_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    COLON_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    BE,
    INTEGER,
    FLOAT,
    IDENTIFIER,
    STRING,
    STRING_LEFT,
    STRING_MIDDLE,
    STRING_RIGHT,
    IF,
    ELSE,
    WHILE,
    FUNCTION,
    INTERFACE,
    BUILTIN,
    TYPE,
    CASE,
    TRUE,
    FALSE,
    OR,
    AND,
    USE,
    BREAK,
    CONTINUE,
    RETURN,
    MUT,
    NEW,
    INCLUDE,
    EXTERN,
    LINK_WITH,
    NEW_LINE,
    END_OF_FILE
} TokenKind;

typedef struct {
    TokenKind kind;
    String literal;
    size_t line;
    size_t column;
} Token;

// Error
typedef struct {
    const char* message;
} Error;

// List types
typedef ListType(Token) TokenList;
typedef ListType(Error) ErrorList;

// Stack types
typedef StackType(bool) BoolStack;

#endif