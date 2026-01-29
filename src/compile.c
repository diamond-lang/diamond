#include "compile.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "arena.h"
#include "ast.h"
#include "builtin.h"
#include "codegen.h"
#include "common.h"
#include "parser.h"
#include "program.h"
#include "semantic.h"
#include "stdio.h"
#include "types.h"
#include "utilities.h"

static void findImports(
    Arena* arena, Program* program, uint32_t current, Arena scratch
) {
    Ast* currentAst = program_getAst(program, current);
    String path = program_getPath(program, current);

    // Parse imports
    String source = readFile(&scratch, string_asCString(path));
    parseImports(currentAst, string_asCString(source), scratch);

    // Check for errors
    if (list_size(currentAst->errors) != 0) {
        reportErrors(*currentAst, program_getPath(program, current), scratch);
        exit(EXIT_FAILURE);
    }

    // Parse imported files
    for (uint32_t i = 0; i < list_size(currentAst->imports); i++) {
        uint32_t literal = list_get(currentAst->imports, i)->path;
        StringView view = ast_literalAsView(*currentAst, literal);
        String canonicalPath = getCanonicalPath(&scratch, view, *arena);
        StringView extension = (StringView){strlen(".dmd"), ".dmd"};
        string_concat(&scratch, &canonicalPath, extension);

        // Check that it has not been added
        bool founded = false;
        for (uint32_t astId = 1; astId <= list_size(program->asts); astId++) {
            if (string_equal(
                    string_asView(program_getPath(program, astId)),
                    string_asView(canonicalPath)
                )) {
                founded = true;
                break;
            }
        }

        if (!founded) {
            uint32_t newAst = list_size(program->asts);
            list_append(arena, program->asts, (Ast){.arena = arena_new()});
            String moved = {0};
            string_concat(arena, &moved, string_asView(canonicalPath));
            list_append(arena, program->paths, moved);
            list_append(&currentAst->arena, currentAst->importedAsts, newAst);
            findImports(arena, program, newAst, scratch);
        }
    }
}

typedef struct {
    bool added;
    Uint32List imports;
} ImportsForGraph;

typedef ListType(ImportsForGraph) ImportsList;

static Uint32ListList findDependencyGraph(
    Arena* arena, AstList* asts, Arena scratch
) {
    // Copy imports
    ImportsList importsList = {0};
    for (uint32_t i = 0; i < list_size(*asts); i++) {
        Uint32List astImports = list_get(*asts, i)->importedAsts;
        Uint32List copy = {0};
        for (uint32_t j = 0; j < list_size(astImports); j++) {
            list_append(&scratch, copy, *list_get(astImports, j));
        }
        ImportsForGraph importsForGraph = (ImportsForGraph){false, copy};
        list_append(&scratch, importsList, importsForGraph);
    }

    // Construct dependency graph
    Uint32ListList graph = {0};
    while (true) {
        Uint32List list = {0};
        // For the imports of each AST
        for (uint32_t i = 0; i < list_size(importsList); i++) {
            // If AST doesn't have imports
            ImportsForGraph* imports = list_get(importsList, i);
            if (!imports->added && list_size(imports->imports) == 0) {
                uint32_t astId = i + 1;

                // Add to current stage of dependecy graph
                list_append(arena, list, astId);

                // Remove from imports list from other ASTs
                for (uint32_t j = 0; j < list_size(importsList); j++) {
                    list_removeFirstMatch(
                        list_get(importsList, j)->imports,
                        astId
                    );
                }

                // Set as added
                imports->added = true;
            }
        }
        if (list_size(list) == 0) break;
        list_append(arena, graph, list);
    }

    return graph;
}

Program getProgramGraph(Arena* arena, StringView file, Arena scratch) {
    // Initialize program
    Program program = {0};
    list_append(arena, program.asts, (Ast){.arena = arena_new()});
    list_append(arena, program.paths, getCanonicalPath(arena, file, scratch));

    // Parse builtin
    program.builtin.arena = arena_new();
    parseBuiltin(&program.builtin, builtin, scratch);
    assert(list_size(program.builtin.errors) == 0);

    // Find what each file imports
    findImports(arena, &program, 1, scratch);

    // Find dependecy graph
    program.dependencyGraph =
        findDependencyGraph(arena, &program.asts, scratch);

    // Return
    return program;
}

void parseProgram(Program* program, Arena scratch) {
    // Parse following dependecy graph
    for (uint32_t i = 0; i < list_size(program->dependencyGraph); i++) {
        Uint32List stage = *list_get(program->dependencyGraph, i);

        for (uint32_t j = 0; j < list_size(stage); j++) {
            uint32_t astId = *list_get(stage, j);

            // reset AST
            Ast* ast = program_getAst(program, astId);
            String path = program_getPath(program, astId);
            ast_clear(ast);

            // Parse AST
            Arena newScratch = scratch;
            String source = readFile(&newScratch, string_asCString(path));
            parse(ast, string_asCString(source), newScratch);

            // Report errors if they are
            if (list_size(ast->errors) != 0) {
                String path = program_getPath(program, astId);
                reportErrors(*ast, path, newScratch);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void analyzeProgram(Program* program, Arena scratch1, Arena scratch2) {
    // Check the interface of each module
    analyzeModulesInterfaces(program, scratch1, scratch2);

    // Do semantic analysis following dependecy graph
    for (uint32_t i = 0; i < list_size(program->dependencyGraph); i++) {
        Uint32List stage = *list_get(program->dependencyGraph, i);

        for (uint32_t j = 0; j < list_size(stage); j++) {
            uint32_t astId = *list_get(stage, j);
            analyzeModule(program, astId, scratch1);

            // Report errors if they are
            Ast ast = *program_getAst(program, astId);
            if (list_size(ast.errors) != 0) {
                String path = program_getPath(program, astId);
                reportErrors(ast, path, scratch1);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Check functions used (Reachability analysis)
    checkFunctionsUsed(program, scratch1);
}

void codegenObjectFiles(Program program, Arena scratch1, Arena scratch2) {
    switch (currentPlatform()) {
    case Windows: todo();
    case Linux: todo();
    case MacOS: {
        String cachePath = getCachePath(&scratch1);
        struct stat info;
        assert(
            stat(cachePath.buffer, &info) != 0 &&
            (ENOENT == errno || ENOTDIR == errno)
        );
        String command = {0};
        string_concat(&scratch1, &command, cStringAsView("mkdir -p "));
        string_concat(&scratch1, &command, string_asView(cachePath));
        string_concat(&scratch1, &command, cStringAsView("&& rm -f "));
        string_concat(&scratch1, &command, string_asView(cachePath));
        string_concat(&scratch1, &command, cStringAsView("/*.o"));
        system(command.buffer);
        break;
    }
    }
    String builtinPath = {0};
    string_concat(&scratch1, &builtinPath, cStringAsView("builtin"));
    generateObjectCode(
        &program,
        0,
        getBuiltinObjectFileName(&scratch1),
        builtinPath,
        false,
        scratch2
    );
    for (uint32_t i = 1; i <= list_size(program.asts); i++) {
        String path = program_getPath(&program, i);
        String objectFileName = getObjectFileName(&scratch1, i, path, scratch2);
        bool isEntry = i == 1;
        generateObjectCode(
            &program,
            i,
            objectFileName,
            path,
            isEntry,
            scratch2
        );
    }
}

void linkProgram(Program program, Arena scratch1, Arena scratch2) {
    // Get executable name
    String executableName =
        getExecutableName(&scratch1, *list_get(program.paths, 0));

    // Get object files
    StringList objectFiles = {0};
    list_append(&scratch1, objectFiles, getBuiltinObjectFileName(&scratch1));
    for (uint32_t i = 1; i <= list_size(program.asts); i++) {
        String objectFile = getObjectFileName(
            &scratch1,
            i,
            program_getPath(&program, i),
            scratch2
        );
        list_append(&scratch1, objectFiles, objectFile);
    }

    // Link
    linkObjectFiles(executableName, objectFiles, scratch1);

    // Delete object files
    String cachePath = getCachePath(&scratch1);
    String command = {0};
    string_concat(&scratch1, &command, cStringAsView("rm -rf "));
    string_concat(&scratch1, &command, string_asView(cachePath));
    system(command.buffer);
}
