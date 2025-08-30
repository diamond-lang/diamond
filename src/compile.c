#include "compile.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "arena.h"
#include "ast.h"
#include "builtin.h"
#include "codegen.h"
#include "parser.h"
#include "program.h"
#include "semantic.h"
#include "stdio.h"
#include "types.h"
#include "utilities.h"

static void findImports(
    Arena* arena, Program* program, uint32_t current, Arena scratch
) {
    Ast* currentAst = list_get(program->asts, current);
    String path = *list_get(program->paths, current);

    // Parse imports
    String source = readFile(&scratch, string_asCString(path));
    parseImports(currentAst, string_asCString(source), scratch);

    // Check for errors
    if (list_size(currentAst->errors) != 0) {
        reportErrors(*currentAst, *list_get(program->paths, current), scratch);
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
        for (uint32_t j = 0; j < list_size(program->asts); j++) {
            if (string_equal(
                    string_asView(*list_get(program->paths, j)),
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
                // Add to current stage of dependecy graph
                list_append(arena, list, i);

                // Remove from imports list from other ASTs
                for (uint32_t j = 0; j < list_size(importsList); j++) {
                    list_removeFirstMatch(list_get(importsList, j)->imports, i);
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

Program compile(Arena* arena, StringView file, Arena scratch) {
    // Initialize program
    Program program = {0};
    list_append(arena, program.asts, (Ast){.arena = arena_new()});
    list_append(arena, program.paths, getCanonicalPath(arena, file, scratch));

    // Parse builtin
    program.builtin.arena = arena_new();
    parseBuiltin(&program.builtin, builtin, scratch);
    assert(list_size(program.builtin.errors) == 0);

    // Find what each file imports
    findImports(arena, &program, 0, scratch);

    // Find dependecy graph
    program.dependencyGraph =
        findDependencyGraph(arena, &program.asts, scratch);
    for (uint32_t i = 0; i < list_size(program.dependencyGraph); i++) {
        for (uint32_t j = 0;
             j < list_size(*list_get(program.dependencyGraph, i));
             j++) {
            uint32_t ast = *list_get(*list_get(program.dependencyGraph, i), j);
            printf("%s\n", string_asCString(*list_get(program.paths, ast)));
        }
        printf("\n");
    }

    // Parse following dependecy graph
    for (uint32_t i = 0; i < list_size(program.dependencyGraph); i++) {
        Uint32List stage = *list_get(program.dependencyGraph, i);

        for (uint32_t j = 0; j < list_size(stage); j++) {
            uint32_t astId = *list_get(stage, j);

            // reset AST
            Ast* ast = list_get(program.asts, astId);
            String path = *list_get(program.paths, astId);
            ast_clear(ast);

            // Parse AST
            clock_t start, end;
            double timeSpent;
            start = clock();
            Arena newScratch = scratch;
            String source = readFile(&newScratch, string_asCString(path));
            parse(ast, string_asCString(source), newScratch);
            end = clock();
            timeSpent = (double)(end - start) / CLOCKS_PER_SEC;
            printf("Total 1: %g[s]\n\n", timeSpent);

            // Report errors if they are
            if (list_size(ast->errors) != 0) {
                String path = *list_get(program.paths, astId);
                reportErrors(*ast, path, newScratch);
                exit(EXIT_FAILURE);
            }

            uint32_t sum = 0;
            for (uint32_t i = 0; i < list_size(ast->code.instructions); i++) {
                printf("%d ", ast_getInstruction(ast->code, i + 1));
                sum += ast_getInstruction(ast->code, i + 1);
            }
            printf(
                "\n#instructions: %u, sum: %u, data: %u\n",
                list_size(ast->code.instructions),
                sum,
                list_size(ast->code.data)
            );
            printf(
                "arena usage: %g\n",
                ((double)ast->arena.offset / UINT32_MAX) * 100
            );
        }
    }

    // Find interface for each module following reverse dependecy graph order

    // Print
    for (uint32_t i = 0; i < list_size(program.asts); i++) {
        ast_print(
            *list_get(program.asts, i),
            *list_get(program.paths, i),
            scratch
        );
        if (i + 1 < list_size(program.asts)) printf("\n\n");
    }

    // Do semantic analysis following dependecy graph
    for (uint32_t i = 0; i < list_size(program.dependencyGraph); i++) {
        Uint32List stage = *list_get(program.dependencyGraph, i);

        for (uint32_t j = 0; j < list_size(stage); j++) {
            uint32_t astId = *list_get(stage, j);
            analyze(program, astId, *arena, scratch);

            // Report errors if they are
            Ast ast = *list_get(program.asts, astId);
            if (list_size(ast.errors) != 0) {
                String path = *list_get(program.paths, astId);
                reportErrors(ast, path, scratch);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Print
    for (uint32_t i = 0; i < list_size(program.asts); i++) {
        ast_print(
            *list_get(program.asts, i),
            *list_get(program.paths, i),
            scratch
        );
        if (i + 1 < list_size(program.asts)) printf("\n\n");
    }

    // Generate object codes
    for (uint32_t i = 0; i < list_size(program.asts); i++) {
        generateObjectCode(program, i, *arena, scratch);
    }
    generateExecutable(program, scratch);

    return program;
}