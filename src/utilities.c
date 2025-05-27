#include "utilities.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void appendToPath(String* path1, StringView toAppend) {
    if (toAppend.length == 0) return;
    if (path1->content[path1->count - 1] != '/' && toAppend.pointer[0] != '/') {
        string_append(path1, '/');
    }
    for (size_t i = 0; i < toAppend.length; i++) {
        string_append(path1, toAppend.pointer[i]);
    }
}

typedef struct {
    size_t index;
    size_t length;
} PathComponent;

typedef ListType(PathComponent) ComponentList;
typedef ListType(PathComponent) ComponentStack;

void normalizePath(String* path) {
    char* newBuffer = malloc(path->capacity);
    assert(newBuffer != NULL);

    ComponentList parts = (ComponentList)List();
    ComponentStack normalizedParts = (ComponentStack)Stack();
    for (size_t i = 0; i < path->count;) {
        if (path->content[i] == '/') {
            i += 1;
        }
        size_t j = i;
        while (path->content[j] != '/' && path->content[j] != '\0') j++;
        PathComponent component = (PathComponent){i, j - i};
        if (component.length == 1) {
            if (path->content[component.index] != '.') {
                list_append(parts, component);
            }
        } else {
            list_append(parts, component);
        }
        i = j;
    }

    for (size_t i = 0; i < parts.count; i++) {
        bool partIsTwoDots = parts.items[i].length == 2 &&
                             (memcmp(
                                  path->content + parts.items[i].index,
                                  "..",
                                  parts.items[i].length
                              ) == 0);
        if (partIsTwoDots) {
            bool previousNormalizedPartIsTwoDots =
                normalizedParts.count != 0 &&
                normalizedParts.items[normalizedParts.count - 1].length == 2 &&
                (memcmp(
                     path->content +
                         normalizedParts.items[normalizedParts.count - 1].index,
                     "..",
                     normalizedParts.items[normalizedParts.count - 1].length
                 ) == 0);
            ;
            if (normalizedParts.count != 0 &&
                !previousNormalizedPartIsTwoDots) {
                stack_pop(normalizedParts);
            } else if (!isAbsolutePath(string_asView(*path))) {
                stack_push(normalizedParts, parts.items[i]);
            }
        } else {
            stack_push(normalizedParts, parts.items[i]);
        }
    }

    // Construct normalized path
    size_t normalizedSize = 0;
    if (isAbsolutePath(string_asView(*path))) {
        newBuffer[0] = '/';
        normalizedSize += 1;
    }
    for (size_t i = 0; i < normalizedParts.count; i++) {
        PathComponent part = normalizedParts.items[i];
        memcpy(
            newBuffer + normalizedSize,
            path->content + part.index,
            part.length
        );
        normalizedSize += part.length;
        if (i + 1 != normalizedParts.count) {
            newBuffer[normalizedSize] = '/';
            normalizedSize += 1;
        }
    }
    newBuffer[normalizedSize] = '\0';

    // Free resources
    free(parts.items);
    free(normalizedParts.items);
    free(path->content);

    // Return
    path->content = newBuffer;
    path->count = normalizedSize;
}

String getWorkingDirectory() {
    size_t bufferSize = 256;
    char* buffer = malloc(bufferSize);
    assert(buffer != NULL);

    do {
        char* workingDirectory = getcwd(buffer, bufferSize);
        if (workingDirectory == NULL && errno == ERANGE) {
            bufferSize *= 2;
        } else {
            break;
        }
    } while (true);

    return (String){buffer, strlen(buffer), bufferSize};
}

String getCanonicalPath(StringView path) {
    String canonicalPath = String();
    if (isAbsolutePath(path)) {
        size_t bufferSize = 256;
        canonicalPath.content = malloc(bufferSize);
        canonicalPath.count = path.length;
        canonicalPath.capacity = bufferSize;
        normalizePath(&canonicalPath);
        return canonicalPath;
    } else {
        canonicalPath = getWorkingDirectory();
        appendToPath(&canonicalPath, path);
        normalizePath(&canonicalPath);
        return canonicalPath;
    }
}

String readFile(char* path) {
    char* content = NULL;
    FILE* file = fopen(path, "r");
    assert(file != NULL);

    int result = fseek(file, 0, SEEK_END);
    assert(result == 0);

    long fileSize = ftell(file);
    assert(fileSize != -1);

    content = malloc(sizeof(char) * (fileSize + 1));
    assert(fseek(file, 0, SEEK_SET) == 0);

    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';

    fclose(file);

    return (String){content, fileSize, fileSize};
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