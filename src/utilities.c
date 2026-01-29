#include "utilities.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "common.h"
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
    for (uint32_t i = 0; i < string_size(path);) {
        if (string_get(path, i) == '/') i += 1;
        uint32_t j = i;
        while (j < string_size(path) && string_get(path, j) != '/') j++;
        StringView part = (StringView){j - i, string_asCString(path) + i};
        if (!string_equal(part, cStringAsView(".")))
            list_append(&scratch, parts, part);
        i = j;
    }

    // Get normalized parts
    PartStack normalizedParts = {0};
    for (uint32_t i = 0; i < list_size(parts); i++) {
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
    for (uint32_t i = 0; i < stack_size(normalizedParts); i++) {
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

String getCanonicalPath(Arena* arena, StringView importPath, Arena scratch) {
    if (isAbsolutePath(importPath)) {
        String canonicalPath = {0};
        canonicalPath = appendToPath(arena, canonicalPath, importPath);
        canonicalPath = normalizePath(arena, canonicalPath, scratch);
        return canonicalPath;
    } else {
        String canonicalPath = getWorkingDirectory(arena);
        canonicalPath = appendToPath(arena, canonicalPath, importPath);
        canonicalPath = normalizePath(arena, canonicalPath, scratch);
        return canonicalPath;
    }
}

String getRelativePath(Arena* arena, String from, String to, Arena scratch) {
    String relativePath = {0};

    // Get parts in from
    PartList partsFrom = {0};
    for (uint32_t i = 0; i < string_size(from);) {
        if (string_get(from, i) == '/') i += 1;
        uint32_t j = i;
        while (j < string_size(from) && string_get(from, j) != '/') j++;
        StringView part = (StringView){j - i, string_asCString(from) + i};
        if (!string_equal(part, cStringAsView(".")))
            list_append(&scratch, partsFrom, part);
        i = j;
    }

    // Get parts in to
    PartList partsTo = {0};
    for (uint32_t i = 0; i < string_size(to);) {
        if (string_get(to, i) == '/') i += 1;
        uint32_t j = i;
        while (j < string_size(to) && string_get(to, j) != '/') j++;
        StringView part = (StringView){j - i, string_asCString(to) + i};
        if (!string_equal(part, cStringAsView(".")))
            list_append(&scratch, partsTo, part);
        i = j;
    }

    // Find part where absolute paths diverge
    uint32_t different = 0;
    for (; different < list_size(partsFrom) && different < list_size(partsTo);
         different++) {
        StringView partFrom = *list_get(partsFrom, different);
        StringView partTo = *list_get(partsTo, different);
        if (!string_equal(partFrom, partTo)) {
            break;
        }
    }

    // Add ".." to go "from" path to "to" path
    uint32_t partsTotal =
        (list_size(partsFrom) - different) + (list_size(partsTo) - different);
    for (uint32_t i = different; i < list_size(partsFrom); i++) {
        string_concat(arena, &relativePath, cStringAsView(".."));
        if ((i - different) + 1 != partsTotal) {
            string_concat(arena, &relativePath, cStringAsView("/"));
        }
    }

    // Add parts from "to" path
    for (uint32_t i = different; i < list_size(partsTo); i++) {
        StringView part = *list_get(partsTo, i);
        string_concat(arena, &relativePath, part);
        if ((i - different) + 1 != partsTotal) {
            string_concat(arena, &relativePath, cStringAsView("/"));
        }
    }

    // Return
    return relativePath;
}

String getBasePath(Arena* arena, String path) {
    uint32_t i = string_size(path) - 1;
    while (0 <= i && i <= string_size(path)) {
        if (path.buffer[i] == '/') {
            break;
        }
        i--;
    }
    return string_substring(arena, path, i + 1, string_size(path) - (i + 1));
}

String getPathWithoutExtension(Arena* arena, String path) {
    uint32_t i = string_size(path);
    while (0 <= i && i <= string_size(path)) {
        if (path.buffer[i] == '.') {
            break;
        }
        i--;
    }
    assert(i <= string_size(path));
    return string_substring(arena, path, 0, i);
}

String readFile(Arena* arena, char* path) {
    FILE* file = fopen(path, "r");
    assert(file != NULL);

    int result = fseek(file, 0, SEEK_END);
    assert(result == 0);

    long fileSize = ftell(file);
    assert(fileSize != -1);

    char* content = arena_alloc(arena, char, fileSize + 1);
    assert(fseek(file, 0, SEEK_SET) == 0);

    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';

    fclose(file);

    String result2 = {content, fileSize, fileSize};
    return result2;
}

String numberAsString(Arena* arena, uint32_t number) {
    uint32_t digitsCount = numberOfDigits(number);
    String result = {0};
    string_ensureExtraCapacity(arena, &result, digitsCount + 1);
    snprintf(result.buffer, digitsCount + 1, "%d", number);
    result.count = digitsCount;
    return result;
}

#ifdef _WIN32
#include <io.h>
#elif __APPLE__ || __LINUX__
#include <unistd.h>
#endif

bool fileExists(char* path) { return (access(path, F_OK) == 0); }

String getCachePath(Arena* arena) {
    String result = {0};
    switch (currentPlatform()) {
    case Windows: todo();
    case Linux: todo();
    case MacOS: {
        char* homeDir = getenv("HOME");
        string_concat(arena, &result, cStringAsView(homeDir));
        string_concat(arena, &result, cStringAsView("/.cache/diamond/"));
        string_concat(
            arena,
            &result,
            string_asView(numberAsString(arena, getProccessId()))
        );
    }
    }
    return result;
}

String getObjectFileName(
    Arena* arena, uint32_t astId, String canonicalPath, Arena scratch
) {
    String result = getCachePath(arena);
    String baseName = getPathWithoutExtension(&scratch, canonicalPath);
    baseName = getBasePath(&scratch, baseName);
    String number = numberAsString(&scratch, astId);
    string_concat(arena, &result, cStringAsView("/"));
    string_concat(arena, &result, string_asView(number));
    string_concat(arena, &result, cStringAsView("_"));
    string_concat(arena, &result, string_asView(baseName));
    switch (currentPlatform()) {
    case Windows: string_concat(arena, &result, cStringAsView(".obj")); break;
    case Linux: string_concat(arena, &result, cStringAsView(".o")); break;
    case MacOS: string_concat(arena, &result, cStringAsView(".o")); break;
    }
    return result;
}

String getBuiltinObjectFileName(Arena* arena) {
    String result = getCachePath(arena);
    string_concat(arena, &result, cStringAsView("/builtin.o"));
    return result;
}

String getExecutableName(Arena* arena, String path) {
    String executableName = getPathWithoutExtension(arena, path);
    switch (currentPlatform()) {
    case Windows: {
        string_concat(arena, &executableName, cStringAsView(".exe"));
        break;
    }
    case Linux: break;
    case MacOS: break;
    }
    return executableName;
}

String getCommandToExecute(Arena* arena, String executableName) {
    return executableName;
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

int getProccessId() { return getpid(); }