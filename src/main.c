#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "codegen.h"
#include "compile.h"
#include "error.h"
#include "program.h"
#include "types.h"
#include "utilities.h"

#ifdef _WIN32
#include <Windows.h>

void enableColoredTextAndUnicode() {
    // Colored text
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(
        handle,
        ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING
    );

    // Unicode
    SetConsoleOutputCP(65001);
}
#endif

typedef enum { BuildCommand, RunCommand, EmitCommand } CommandKind;

typedef struct {
    CommandKind kind;
    String path;
    StringList options;
} Command;

static void printUsageAndExit() {
    error_printUsage();
    exit(EXIT_FAILURE);
}

static void checkUsage(int argc, char* argv[]) {
    if (argc < 3) {
        printUsageAndExit();
    }
    if (strcmp(argv[1], "run") == 0 && argc < 3) {
        printUsageAndExit();
    }
    if (strcmp(argv[1], "emit") == 0 &&
        (argc < 4 || !((strcmp(argv[2], "--dependency-graph") == 0) ||
                       (strcmp(argv[2], "--ast") == 0) ||
                       (strcmp(argv[2], "--ast-with-types") == 0) ||
                       (strcmp(argv[2], "--llvm-ir") == 0)))) {
        printUsageAndExit();
    }
}

static Command get_command(Arena* arena, int argc, char* argv[]) {
    String path = {0};
    StringList options = {0};
    if (strcmp(argv[1], "build") == 0) {
        string_concat(arena, &path, cStringAsView(argv[2]));
        return (Command){BuildCommand, path, options};
    }
    if (strcmp(argv[1], "run") == 0) {
        string_concat(arena, &path, cStringAsView(argv[2]));
        return (Command){RunCommand, path, options};
    }
    if (strcmp(argv[1], "emit") == 0) {
        String option = {0};
        string_concat(arena, &option, cStringAsView(argv[2]));
        string_concat(arena, &path, cStringAsView(argv[3]));
        list_append(arena, options, option);
        return (Command){EmitCommand, path, options};
    }
    assert(false);
}

static void build(Command command, Arena scratch1, Arena scratch2) {
    Program program =
        getProgramGraph(&scratch1, string_asView(command.path), scratch2);
    parseProgram(&program, scratch1);
    analyzeProgram(&program, scratch1);
    codegenObjectFiles(program, scratch1);
    linkProgram(program, scratch1);
}

static void run(Command command, Arena scratch1, Arena scratch2) {
    Program program =
        getProgramGraph(&scratch1, string_asView(command.path), scratch2);
    String executableName =
        getExecutableName(&scratch1, *list_get(program.paths, 0));
    bool alreadyExisted = fileExists(executableName.buffer);
    parseProgram(&program, scratch1);
    analyzeProgram(&program, scratch1);
    codegenObjectFiles(program, scratch1);
    linkProgram(program, scratch1);
    system(executableName.buffer);
    if (!alreadyExisted) {
        remove(executableName.buffer);
    }
}

static void emit(Command command, Arena scratch1, Arena scratch2) {
    String workingDirectory = getWorkingDirectory(&scratch1);
    Program program =
        getProgramGraph(&scratch1, string_asView(command.path), scratch2);
    if (strcmp(list_get(command.options, 0)->buffer, "--dependency-graph") ==
        0) {
        for (uint32_t i = 0; i < list_size(program.dependencyGraph); i++) {
            Uint32List stage = *list_get(program.dependencyGraph, i);
            printf("stage%d\n", i + 1);
            for (uint32_t j = 0; j < list_size(stage); j++) {
                uint32_t ast = *list_get(stage, j);
                Arena copyScratch1 = scratch1;
                String relativePath = getRelativePath(
                    &copyScratch1,
                    workingDirectory,
                    *list_get(program.paths, ast),
                    scratch2
                );
                printf("    %s\n", relativePath.buffer);
            }
        }
        return;
    }
    parseProgram(&program, scratch1);
    if (strcmp(list_get(command.options, 0)->buffer, "--ast") == 0) {
        for (uint32_t i = 0; i < list_size(program.asts); i++) {
            ast_print(
                *list_get(program.asts, i),
                *list_get(program.paths, i),
                scratch1,
                scratch2
            );
            if (i + 1 < list_size(program.asts)) printf("\n\n");
        }
        return;
    }
    analyzeProgram(&program, scratch1);
    if (strcmp(list_get(command.options, 0)->buffer, "--ast-with-types") == 0) {
        for (uint32_t i = 0; i < list_size(program.asts); i++) {
            ast_print(
                *list_get(program.asts, i),
                *list_get(program.paths, i),
                scratch1,
                scratch2
            );
            if (i + 1 < list_size(program.asts)) printf("\n\n");
        }
        return;
    }
    if (strcmp(list_get(command.options, 0)->buffer, "--llvm-ir") == 0) {
        printLLVMIR(program, 0, scratch1);
        return;
    }
}

int main(int argc, char* argv[]) {
    Arena arena1 = arena_new();
    Arena arena2 = arena_new();

    // Enable colored text and unicode on windows
#ifdef _WIN32
    enableColoredTextAndUnicode();
#endif

    // Check usage
    checkUsage(argc, argv);

    // Get command line arguments
    Command command = get_command(&arena1, argc, argv);

    // Execute command
    switch (command.kind) {
    case BuildCommand: {
        build(command, arena1, arena2);
        break;
    }
    case RunCommand: {
        run(command, arena1, arena2);
        break;
    }
    case EmitCommand: {
        emit(command, arena1, arena2);
        break;
    }
    }

    return 0;
}