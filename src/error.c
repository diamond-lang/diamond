#include "error.h"

#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "types.h"
#include "utilities.h"

void printBold(char* str) { printf("\x1b[1m%s\x1b[0m", str); }

void printUnderline(char* str) { printf("\x1b[4m%s\x1b[0m", str); }

void printCyan(char* str) { printf("\x1b[36m%s\x1b[0m", str); }

void printBrightCyan(char* str) { printf("\x1b[96m%s\x1b[0m", str); }

void printRed(char* str) { printf("\x1b[31m%s\x1b[0m", str); }

void printMagenta(char* str) { printf("\x1b[35m%s\x1b[0m", str); }

void printBrightMagenta(char* str) { printf("\x1b[95m%s\x1b[0m", str); }

void printHeader(char* str) { return printBrightCyan(str); }

void underlineUntilLocation(char* filePath, size_t line, size_t column) {
    for (size_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("│ ");
    for (size_t i = 0; i < column - 1; i++) {
        printRed("^");
    }
}

void printCurrentLine(char* filePath, size_t line) {
    String file = readFile(filePath);
    for (size_t i = 0; i < file.count && line >= 1; i++) {
        if (file.content[i] == '\n') {
            line -= 1;
        } else if (file.content[i] == '\r' && file.content[i + 1] == '\n') {
            line -= 2;
            i += 1;
        } else if (line == 1) {
            printf("%c", file.content[i]);
        }
    }
    arena_free(file.content);
}

void reportError(Ast ast, Error error) {
    switch (error.kind) {
        case FILE_NOT_FOUND: todo(); break;
        case UNRECOGNIZED_CHARACTER: todo(); break;
        case UNCLOSE_BLOCK_COMMENT: todo(); break;
        case UNEXPECTED_CHARACTER: todo(); break;
        case UNEXPECTED_IDENTATION:
            printHeader(ast.filePath);
            printHeader(": Unexpected indent\n\n");
            printf("%zu│ ", error.line);
            printCurrentLine(ast.filePath, error.line);
            printf("\n");
            underlineUntilLocation(ast.filePath, error.line, error.column);
            printf("\n");
            printf("%zu│ ", error.line + 1);
            printCurrentLine(ast.filePath, error.line + 1);
            printf("\n\n");
            break;
        case EXPECTING_STATEMENT: todo(); break;
        case EXPECTING_NEW_IDENTATION_LEVEL: todo(); break;
        case UDENFINED_VARIABLE: todo(); break;
        case REASSIGNING_IMMUTABLE_VARIABLE: todo(); break;
        case UDENFINED_FUNCTION: todo(); break;
        case UNHANDLED_RETURN_VALUE: todo(); break;
    }
}

void reportErrors(Ast ast) {
    for (size_t i = 0; i < ast.errors.count; i++) {
        reportError(ast, ast.errors.items[i]);
    }
}