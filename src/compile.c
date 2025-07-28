#include "compile.h"

#include <assert.h>
#include <stddef.h>

#include "arena.h"
#include "ast.h"
#include "builtin.h"
#include "parser.h"
#include "program.h"
#include "semantic.h"
#include "stdio.h"
#include "types.h"
#include "utilities.h"

static void findImports(Program* program, uint32_t current) {
    Ast* currentAst = list_get(program->asts, current);
    String path = *list_get(program->paths, current);

    // Parse imports
    arena_newLifetime();
    String source = readFile(string_asCString(path));
    parseImports(currentAst, string_asCString(source));
    arena_destroyCurrentLifetime();

    // Check for errors
    if (list_size(currentAst->errors) != 0) {
        reportErrors(*currentAst, *list_get(program->paths, current));
        exit(EXIT_FAILURE);
    }

    // Parse imported files
    for (uint32_t i = 0; i < list_size(currentAst->imports); i++) {
        uint32_t literal = list_get(currentAst->imports, i)->path;
        StringView view = ast_literalAsView(*currentAst, literal);
        String canonicalPath = getCanonicalPath(view);
        string_concat(&canonicalPath, (StringView){strlen(".dmd"), ".dmd"});

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
            list_append(program->asts, (Ast){});
            list_append(program->paths, canonicalPath);
            ast_init(list_get(program->asts, newAst));
            list_append(currentAst->importedAsts, newAst);
            findImports(program, newAst);
        }
    }
}

typedef struct {
    bool added;
    Uint32List imports;
} ImportsForGraph;

typedef ListType(ImportsForGraph) ImportsList;

static Uint32ListList findDependencyGraph(AstList* asts) {
    Uint32ListList graph = (Uint32ListList)List();

    arena_newLifetime();

    // Copy imports
    ImportsList importsList = List();
    for (uint32_t i = 0; i < asts->size; i++) {
        Uint32List astImports = list_get(*asts, i)->importedAsts;
        Uint32List copy = List();
        for (uint32_t j = 0; j < list_size(astImports); j++) {
            list_append(copy, *list_get(astImports, j));
        }
        ImportsForGraph importsForGraph = (ImportsForGraph){false, copy};
        list_append(importsList, importsForGraph);
    }

    // Construct dependency graph
    while (true) {
        Uint32List list = (Uint32List)ListWithLifetime(graph.lifetime);
        // For the imports of each AST
        for (uint32_t i = 0; i < list_size(importsList); i++) {
            // If AST doesn't have imports
            ImportsForGraph* imports = list_get(importsList, i);
            if (!imports->added && list_size(imports->imports) == 0) {
                // Add to current stage of dependecy graph
                list_append(list, i);

                // Remove from imports list from other ASTs
                for (uint32_t j = 0; j < list_size(importsList); j++) {
                    list_removeFirstMatch(list_get(importsList, j)->imports, i);
                }

                // Set as added
                imports->added = true;
            }
        }
        if (list_size(list) == 0) break;
        list_append(graph, list);
    }

    arena_destroyCurrentLifetime();
    return graph;
}

Program compile(StringView file) {
    // Initialize program
    Program program = {(Ast){}, (AstList)List(), (StringList)List()};
    ast_init(&program.builtin);
    list_append(program.asts, (Ast){});
    ast_init(list_get(program.asts, 0));
    list_append(program.paths, getCanonicalPath(file));

    // Parse builtin
    parseBuiltin(&program.builtin, builtin);
    assert(list_size(program.builtin.errors) == 0);

    // Find what each file imports
    findImports(&program, 0);

    // Find dependecy graph
    program.dependencyGraph = findDependencyGraph(&program.asts);
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
            arena_newLifetime();
            String source = readFile(string_asCString(path));
            parse(ast, string_asCString(source));
            arena_destroyCurrentLifetime();

            // Report errors if they are
            if (list_size(ast->errors) != 0) {
                String path = *list_get(program.paths, astId);
                reportErrors(*ast, path);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Find interface for each module following reverse dependecy graph order

    for (uint32_t i = 0; i < list_size(program.asts); i++) {
        ast_print(*list_get(program.asts, i), *list_get(program.paths, i));
        if (i + 1 < list_size(program.asts)) printf("\n\n");
    }

    // Do semantic analysis following dependecy graph
    for (uint32_t i = 0; i < list_size(program.dependencyGraph); i++) {
        Uint32List stage = *list_get(program.dependencyGraph, i);

        for (uint32_t j = 0; j < list_size(stage); j++) {
            uint32_t astId = *list_get(stage, j);
            analyze(program, astId);

            // Report errors if they are
            Ast ast = *list_get(program.asts, astId);
            if (list_size(ast.errors) != 0) {
                String path = *list_get(program.paths, astId);
                reportErrors(ast, path);
                exit(EXIT_FAILURE);
            }
        }
    }

    for (uint32_t i = 0; i < list_size(program.asts); i++) {
        ast_print(*list_get(program.asts, i), *list_get(program.paths, i));
        if (i + 1 < list_size(program.asts)) printf("\n\n");
    }

    return program;
}