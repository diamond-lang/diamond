#include "types.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

// StringView
StringView cStringAsView(char* string) {
    return (StringView){strlen(string), string};
}

bool string_isLowerCase(StringView view) {
    for (uint32_t i = 0; i < view.length; i++) {
        if (!isalpha(view.pointer[i]) || !islower(view.pointer[i])) {
            return false;
        }
    }
    return true;
}

// String
uint32_t string_size(String string) { return string.count; }

void string_clear(String* string) { string->count = 0; }

char* string_asCString(String string) { return string.buffer; }

char string_get(String string, uint32_t index) {
    assert(index < string.count);
    return string.buffer[index];
}

void string_append(Arena* arena, String* string, char item) {
    if (string->count == 0) {
        string->buffer = arena_alloc(arena, char, 256);
        string->capacity = 256;
    } else {
        if (string->count + 1 >= string->capacity) {
            string->capacity *= 2;
            void* newBuffer = arena_alloc(arena, char, string->capacity);
            memcpy(newBuffer, string->buffer, string->count);
            string->buffer = newBuffer;
        }
    }
    string->buffer[string->count] = item;
    string->buffer[string->count + 1] = '\0';
    string->count += 1;
}

void string_concat(Arena* arena, String* string, StringView toConcat) {
    if (string->capacity == 0) {
        string->buffer = arena_alloc(arena, char, 256);
        string->capacity = 256;
        string->buffer[0] = '\0';
    }
    if (string->count + toConcat.length + 1 >= string->capacity) {
        while (string->count + toConcat.length + 1 >= string->capacity) {
            string->capacity *= 2;
        }
        void* newBuffer = arena_alloc(arena, char, string->capacity);
        memcpy(newBuffer, string->buffer, string->count);
        string->buffer = newBuffer;
    }
    memcpy(string->buffer + string->count, toConcat.pointer, toConcat.length);
    string->count += toConcat.length;
    string->buffer[string->count] = '\0';
}

bool string_equal(StringView a, StringView b) {
    if (a.length == b.length) {
        return memcmp(a.pointer, b.pointer, a.length) == 0;
    }
    return false;
}

char* string_pointer(String string) { return string.buffer; }

void string_ensureExtraCapacity(
    Arena* arena, String* string, uint32_t extraCapacity
) {
    if (string->count == 0) {
        string->buffer = arena_alloc(arena, char, 256);
        string->capacity = 256;
        string->buffer[0] = '\0';
    }
    if (string->count + extraCapacity >= string->capacity) {
        while (string->count + extraCapacity >= string->capacity) {
            string->capacity *= 2;
        }
        void* newBuffer = arena_alloc(arena, char, string->capacity);
        memcpy(newBuffer, string->buffer, string->count);
        string->buffer = newBuffer;
    }
}

StringView string_asView(String string) {
    return (StringView){string_size(string), string.buffer};
}

String string_substring(
    Arena* arena, String string, uint32_t start, uint32_t length
) {
    String result = {0};
    string_concat(arena, &result, (StringView){length, string.buffer + start});
    return result;
}

// Array hashmap
uint64_t _array_hashmap_findLocation(Uint32List keys, uint32_t key) {
    uint64_t i = 0;
    for (; i < list_size(keys); i++) {
        if (*list_get(keys, i) == key) break;
    }
    return i;
}
