#include "types.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
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

void string_concat(String* string, StringView toConcat) {
    for (size_t i = 0; i < toConcat.length; i++) {
        string_append(string, toConcat.pointer[i]);
    }
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

void string_free(String string) { free(string.content); }

StringView string_asView(String string) {
    return (StringView){string.count, string.content};
}

// HashTable
static int32_t hash(int32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static Bucket* findBucket(Bucket* content, int32_t key, size_t capacity) {
    uint32_t index = hash(key) % capacity;
    while (true) {
        Bucket* entry = &content[index];
        if (entry->key == key || entry->key == -1) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

int32_t* hashtable_get(HashTable hashtable, int32_t key) {
    Bucket* bucket = findBucket(hashtable.content, key, hashtable.capacity);
    assert(bucket != NULL);
    return &bucket->value;
}

void hashtable_set(HashTable* hashtable, int32_t key, int32_t value) {
    if (hashtable->count + 1 > hashtable->capacity * 0.7) {
        size_t newCapacity = 0;
        if (hashtable->capacity == 0) {
            newCapacity = 256;
        } else {
            newCapacity = hashtable->capacity * 2;
        }
        Bucket* newBuffer = malloc(sizeof(Bucket) * newCapacity);
        assert(newBuffer);

        for (size_t i = 0; i < newCapacity; i++) {
            newBuffer[i].key = -1;
            newBuffer[i].value = -1;
        }

        if (newCapacity != 256) {
            for (size_t i = 0; i < hashtable->capacity; i++) {
                if (hashtable->content[i].key != -1) {
                    Bucket* bucket = findBucket(
                        newBuffer,
                        hashtable->content[i].key,
                        newCapacity
                    );
                    bucket->key = hashtable->content[i].key;
                    bucket->value = hashtable->content[i].value;
                }
            }
        }

        free(hashtable->content);
        hashtable->content = newBuffer;
        hashtable->capacity = newCapacity;
    }

    Bucket* bucket = findBucket(hashtable->content, key, hashtable->capacity);
    bucket->key = key;
    bucket->value = value;
    hashtable->count += 1;
}