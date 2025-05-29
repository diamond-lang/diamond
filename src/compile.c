#include "compile.h"

#include <assert.h>
#include <stddef.h>

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
            initAst(list_get(*asts, newAst), canonicalPath);
            list_append(currentAst->imports, newAst);
            findImports(asts, newAst);
        }
    }
}

static DependencyGraph findDependencyGraph(AstList* asts) {
    DependencyGraph graph = (DependencyGraph)List();

    // Find imports rercursively
    findImports(asts, 0);

    // Construct dependency graph
    AstIdList alreadyAdded = (AstIdList)List();
    while (true) {
        AstIdList list = (AstIdList)List();
        // For each ast
        for (size_t i = 0; i < list_size(*asts); i++) {
            // If it doens't have imports
            if (list_size(list_get(*asts, i)->imports) == 0) {
                bool already = false;
                for (size_t j = 0; j < list_size(alreadyAdded); j++) {
                    if (*list_get(alreadyAdded, j) == i) {
                        already = true;
                    }
                }
                if (already) continue;

                // Add to current stage of dependecy graph
                list_append(list, i);
                list_append(alreadyAdded, i);
                // Remove from others lists of imports
                for (size_t j = 0; j < list_size(*asts); j++) {
                    list_removeFirstMatch(list_get(*asts, j)->imports, i);
                }
            }
        }
        if (list_size(list) == 0) break;
        list_append(graph, list);
    }

    return graph;
}

Program compile(char* file) {
    Program program = (Program){List()};
    list_append(program.asts, (Ast){});
    initAst(
        list_get(program.asts, 0),
        getCanonicalPath((StringView){strlen(file), file})
    );

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

    // Parse files reverse dependecy graph order

    // Find interface for each module following reverse dependecy graph order

    // Do semantic analysis reverse dependecy graph order
    return program;
}