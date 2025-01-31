#include "utilities.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

String readFile(char* path) {
    char* content = NULL;
    FILE* file = fopen(path, "r");
    assert(file != NULL);

    int result = fseek(file, 0, SEEK_END);
    assert(result == 0);

    long fileSize = ftell(file);
    assert(fileSize != -1);

    content = arena_alloc(sizeof(char) * (fileSize + 1));
    assert(fseek(file, 0, SEEK_SET) == 0);

    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';

    fclose(file);

    String result2 = {content, fileSize, fileSize};
    return result2;
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