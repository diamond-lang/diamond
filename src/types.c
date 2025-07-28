#include "types.h"

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// StringView
StringView cStringAsView(char* string) {
    return (StringView){strlen(string), string};
}

bool isLowerCase(StringView view) {
    for (uint32_t i = 0; i < view.length; i++) {
        if (!isalpha(view.pointer[i]) || !islower(view.pointer[i])) {
            return false;
        }
    }
    return true;
}

// String
uint32_t string_size(String string) {
    uint32_t bufferSize = list_size(string.buffer);
    return bufferSize == 0 ? 0 : bufferSize - 1;
}

void string_clear(String* string) { list_clear(string->buffer); }

char* string_asCString(String string) { return string.buffer.buffer; }

char string_get(String string, uint32_t index) {
    return *list_get(string.buffer, index);
}

void string_append(String* string, char item) {
    if (list_size(string->buffer) == 0) {
        list_append(string->buffer, item);
        list_append(string->buffer, '\0');
    } else {
        *list_get(string->buffer, string_size(*string)) = item;
        list_append(string->buffer, '\0');
    }
}

void string_concat(String* string, StringView toConcat) {
    list_ensureExtraCapacity(string->buffer, toConcat.length);
    list_setSize(string->buffer, string_size(*string));
    list_concat(string->buffer, toConcat.pointer, toConcat.length);
    list_append(string->buffer, '\0');
}

bool string_equal(StringView a, StringView b) {
    if (a.length == b.length) {
        return memcmp(a.pointer, b.pointer, a.length) == 0;
    }
    return false;
}

char* string_pointer(String string) { return string.buffer.buffer; }

StringView string_asView(String string) {
    return (StringView){string_size(string), list_get(string.buffer, 0)};
}

// Hashmap
static uint32_t hash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

uint32_t _hashmap_findLocation(
    uint32_t* keys, uint32_t key, uint32_t capacity
) {
    assert(keys != NULL);
    uint32_t index = hash(key) % capacity;
    while (true) {
        uint32_t keyFound = keys[index];
        if (keyFound == key || keyFound == None()) {
            return index;
        }
        index = (index + 1) % capacity;
    }
}