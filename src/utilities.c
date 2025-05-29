#include "utilities.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "types.h"

#ifdef WIN32
todo();
#else
#include <unistd.h>
#endif

bool isAbsolutePath(StringView path) {
    assert(path.length != 0);
    if (path.pointer[0] == '/') {
        return true;
    } else {
        return false;
    }
}

void appendToPath(String* path, StringView toAppend) {
    if (toAppend.length == 0) return;
    if (string_get(*path, string_size(*path) - 1) != '/' &&
        toAppend.pointer[0] != '/') {
        string_append(path, '/');
    }
    string_concat(path, toAppend);
}

typedef ListType(StringView) PartList;
typedef StackType(StringView) PartStack;

void normalizePath(String* path) {
    arena_newLifetime();

    // Create a copy of the input and clear path
    String copy = String();
    string_concat(&copy, string_asView(*path));
    string_clear(path);

    // Get parts in path
    PartList parts = (PartList)List();
    for (size_t i = 0; i < string_size(copy);) {
        if (string_get(copy, i) == '/') i += 1;
        size_t j = i;
        while (string_get(copy, j) != '/' && string_get(copy, j) != '\0') j++;
        StringView part = (StringView){j - i, string_pointer(copy) + i};
        if (!string_equal(part, cStringAsView("."))) list_append(parts, part);
        i = j;
    }

    // Get normalized parts
    PartStack normalizedParts = (PartStack)Stack();
    for (size_t i = 0; i < list_size(parts); i++) {
        bool partIsTwoDots =
            string_equal(*list_get(parts, i), cStringAsView(".."));
        if (partIsTwoDots && stack_size(normalizedParts) != 0) {
            bool previousNormalizedPartIsTwoDots = string_equal(
                *stack_get(normalizedParts, i - 1),
                cStringAsView("..")
            );
            if (!previousNormalizedPartIsTwoDots) {
                stack_pop(normalizedParts);
            } else if (!isAbsolutePath(string_asView(*path))) {
                stack_push(normalizedParts, *list_get(parts, i));
            }
        } else {
            stack_push(normalizedParts, *list_get(parts, i));
        }
    }

    // Construct normalized path
    if (isAbsolutePath(string_asView(copy))) string_append(path, '/');
    for (size_t i = 0; i < stack_size(normalizedParts); i++) {
        StringView part = *stack_get(normalizedParts, i);
        string_concat(path, part);
        if (i + 1 != stack_size(normalizedParts)) string_append(path, '/');
    }

    arena_destroyCurrentLifetime();
}

String getWorkingDirectory() {
    CharList buffer = List();
    list_ensureExtraCapacity(buffer, 256);
    do {
        char* workingDirectory = getcwd(buffer.buffer, buffer.capacity);
        if (workingDirectory == NULL && errno == ERANGE) {
            list_ensureExtraCapacity(buffer, list_size(buffer));
        } else {
            break;
        }
    } while (true);
    list_setSize(buffer, strlen(buffer.buffer) + 1);
    return (String){buffer};
}

String getCanonicalPath(StringView path) {
    if (isAbsolutePath(path)) {
        String canonicalPath = String();
        appendToPath(&canonicalPath, path);
        normalizePath(&canonicalPath);
        return canonicalPath;
    } else {
        String canonicalPath = getWorkingDirectory();
        appendToPath(&canonicalPath, path);
        normalizePath(&canonicalPath);
        return canonicalPath;
    }
}

String readFile(char* path) {
    // Open file
    FILE* file = fopen(path, "r");
    assert(file != NULL);

    // Get file length
    int result = fseek(file, 0, SEEK_END);
    assert(result == 0);

    long fileSize = ftell(file);
    assert(fileSize != -1);

    // Create buffer
    CharList buffer = List();
    list_ensureExtraCapacity(buffer, fileSize + 1);

    // Go to beginning
    assert(fseek(file, 0, SEEK_SET) == 0);
    fread(buffer.buffer, sizeof(char), fileSize, file);
    buffer.buffer[fileSize] = '\0';
    list_setSize(buffer, fileSize + 1);

    // Close
    fclose(file);

    // Return file
    return (String){buffer};
}

int numberOfDigits(size_t number) {
    assert(number > 0);
    int numberOfDigits = 0;
    while (number > 0) {
        number /= 10;
        numberOfDigits += 1;
    }
    return numberOfDigits;
}