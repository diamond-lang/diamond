#include "types.h"

#include <assert.h>
#include <string.h>

void string_append(String* string, char item) {
    if (string->count + 1 >= string->capacity) {
        if (string->capacity == 0) {
            string->capacity = 256;
        } else {
            string->capacity *= 2;
        }
        string->content = realloc(
            string->content,
            string->capacity * sizeof(*string->content)
        );
    }
    string->content[string->count] = item;
    string->content[string->count + 1] = '\0';
    string->count += 1;
}

String string_substring(String string, size_t start, size_t length) {
    String result = String();
    result.content =
        realloc(result.content, (length + 1) * sizeof(*result.content));
    result.count = length;
    result.capacity = length;
    strncpy(result.content, string.content + start, length);
    result.content[length] = '\0';
    return result;
}

void string_free(String string) {}