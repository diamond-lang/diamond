#include "utilities.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
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

String appendToPath(Arena* arena, String path, StringView toAppend) {
    if (toAppend.length == 0) return path;
    if (string_get(path, string_size(path) - 1) != '/' &&
        toAppend.pointer[0] != '/') {
        string_append(arena, &path, '/');
    }
    string_concat(arena, &path, toAppend);
    return path;
}

typedef ListType(StringView) PartList;
typedef StackType(StringView) PartStack;

static String normalizePath(Arena* arena, String path, Arena scratch) {
    // Get parts in path
    PartList parts = {0};
    for (size_t i = 0; i < string_size(path);) {
        if (string_get(path, i) == '/') i += 1;
        size_t j = i;
        while (j < string_size(path) && string_get(path, j) != '/') j++;
        StringView part = (StringView){j - i, string_asCString(path) + i};
        if (!string_equal(part, cStringAsView(".")))
            list_append(&scratch, parts, part);
        i = j;
    }

    // Get normalized parts
    PartStack normalizedParts = {0};
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
            } else if (!isAbsolutePath(string_asView(path))) {
                stack_push(&scratch, normalizedParts, *list_get(parts, i));
            }
        } else {
            stack_push(&scratch, normalizedParts, *list_get(parts, i));
        }
    }

    // Construct normalized path and return
    String result = {0};
    if (isAbsolutePath(string_asView(path))) string_append(arena, &result, '/');
    for (size_t i = 0; i < stack_size(normalizedParts); i++) {
        StringView part = *stack_get(normalizedParts, i);
        string_concat(arena, &result, part);
        if (i + 1 != stack_size(normalizedParts))
            string_append(arena, &result, '/');
    }
    return result;
}

String getWorkingDirectory(Arena* arena) {
    String string = {0};
    string_ensureExtraCapacity(arena, &string, 256);
    do {
        char* workingDirectory = getcwd(string.buffer, string.capacity);
        if (workingDirectory == NULL && errno == ERANGE) {
            string_ensureExtraCapacity(arena, &string, string.capacity);
        } else {
            break;
        }
    } while (true);
    string.count = strlen(string.buffer);
    return string;
}

String getCanonicalPath(Arena* arena, StringView path, Arena scratch) {
    if (isAbsolutePath(path)) {
        String canonicalPath = {0};
        canonicalPath = appendToPath(arena, canonicalPath, path);
        canonicalPath = normalizePath(arena, canonicalPath, scratch);
        return canonicalPath;
    } else {
        String canonicalPath = getWorkingDirectory(arena);
        canonicalPath = appendToPath(arena, canonicalPath, path);
        canonicalPath = normalizePath(arena, canonicalPath, scratch);
        return canonicalPath;
    }
}

String readFile(Arena* arena, char* path) {
    FILE* file = fopen(path, "r");
    assert(file != NULL);

    int result = fseek(file, 0, SEEK_END);
    assert(result == 0);

    long fileSize = ftell(file);
    assert(fileSize != -1);

    char* content = alloc(arena, char, fileSize + 1);
    assert(fseek(file, 0, SEEK_SET) == 0);

    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';

    fclose(file);

    String result2 = {content, fileSize, fileSize};
    return result2;
}

int numberOfDigits(uint32_t number) {
    if (number == 0) return 1;
    int numberOfDigits = 0;
    while (number > 0) {
        number /= 10;
        numberOfDigits += 1;
    }
    return numberOfDigits;
}