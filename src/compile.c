#include "compile.h"

#include <assert.h>
#include <stddef.h>

#include "arena.h"
#include "ast.h"
#include "parser.h"
#include "program.h"
#include "stdio.h"
#include "types.h"
#include "utilities.h"

static void findImports(AstList* asts, AstId current) {
    Ast* currentAst = list_get(*asts, current);

    // Parse imports
    parseImports(currentAst);

    // Check for errors
    if (list_size(currentAst->errors) != 0) {
        reportErrors(*currentAst);
        exit(EXIT_FAILURE);
    }

    // Parse imported files
    for (size_t i = 0; i < list_size(currentAst->nodes); i++) {
        assert(
            *list_get(currentAst->nodes, i) == AST_USE ||
            *list_get(currentAst->nodes, i) == AST_INCLUDE
        );
        LiteralId literal;
        if (list_get(currentAst->nodes, i) == AST_USE) {
            literal = ast_getData(AstUse, currentAst, i)->path;
        } else {
            literal = ast_getData(AstInclude, currentAst, i)->path;
        }
        StringView view = ast_literalAsStringView(currentAst, literal);
        String canonicalPath = getCanonicalPath(view);
        string_concat(&canonicalPath, (StringView){strlen(".dmd"), ".dmd"});

        // Check that it has not been added
        bool founded = false;
        for (size_t j = 0; j < list_size(*asts); j++) {
            if (string_equal(
                    string_asView(list_get(*asts, j)->canonicalPath),
                    string_asView(canonicalPath)
                )) {
                founded = true;
                break;
            }
        }

        if (!founded) {
            AstId newAst = list_size(*asts);
            list_append(*asts, (Ast){});
            ast_init(list_get(*asts, newAst), canonicalPath);
            list_append(currentAst->imports, newAst);
            findImports(asts, newAst);
        }
    }
}

typedef struct {
    bool added;
    Imports imports;
} ImportsForGraph;

typedef ListType(ImportsForGraph) ImportsList;

static DependencyGraph findDependencyGraph(AstList* asts) {
    DependencyGraph graph = (DependencyGraph)List();

    arena_newLifetime();

    // Copy imports
    ImportsList importsList = List();
    for (size_t i = 0; i < asts->size; i++) {
        Imports astImports = list_get(*asts, i)->imports;
        Imports copy = List();
        for (size_t j = 0; j < list_size(astImports); j++) {
            list_append(copy, *list_get(astImports, j));
        }
        ImportsForGraph importsForGraph = (ImportsForGraph){false, copy};
        list_append(importsList, importsForGraph);
    }

    // Construct dependency graph
    while (true) {
        AstIdList list = (AstIdList)ListWithLifetime(graph.lifetime);
        // For the imports of each AST
        for (AstId i = 0; i < list_size(importsList); i++) {
            // If AST doesn't have imports
            ImportsForGraph* imports = list_get(importsList, i);
            if (!imports->added && list_size(imports->imports) == 0) {
                // Add to current stage of dependecy graph
                list_append(list, i);

                // Remove from imports list from other ASTs
                for (size_t j = 0; j < list_size(importsList); j++) {
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
    Program program;
    program.asts = (AstList)List();
    list_append(program.asts, (Ast){});
    ast_init(list_get(program.asts, 0), getCanonicalPath(file));
    findImports(&program.asts, 0);

    // Find dependecy graph
    program.dependencyGraph = findDependencyGraph(&program.asts);

    for (size_t i = 0; i < list_size(program.dependencyGraph); i++) {
        for (size_t j = 0; j < list_size(*list_get(program.dependencyGraph, i));
             j++) {
            AstId ast = *list_get(*list_get(program.dependencyGraph, i), j);
            printf(
                "%s\n",
                string_pointer(list_get(program.asts, ast)->canonicalPath)
            );
        }
        printf("\n");
    }

    // Parse following dependecy graph
    for (size_t i = 0; i < list_size(program.dependencyGraph); i++) {
        // Get stage
        AstIdList stage = *list_get(program.dependencyGraph, i);

        // For each AST
        for (size_t j = 0; j < list_size(stage); j++) {
            // Parse AST
            Ast* ast = list_get(program.asts, *list_get(stage, j));
            ast_clear(ast);
            parse(ast);

            // Report errors if they are
            if (list_size(ast->errors) != 0) {
                reportErrors(*ast);
                exit(EXIT_FAILURE);
            }
        }
    }

    for (size_t i = 0; i < list_size(program.asts); i++) {
        ast_print(*list_get(program.asts, i));
        if (i + 1 < list_size(program.asts)) printf("\n\n");
    }

    // Find interface for each module following reverse dependecy graph order

    // Do semantic analysis reverse dependecy graph order
    return program;
}