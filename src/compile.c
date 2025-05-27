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
    Ast* currentAst = &asts->items[current];

    // Parse imports
    parseImports(currentAst);

    // Check for errors
    if (currentAst->errors.count != 0) {
        reportErrors(*currentAst);
        exit(EXIT_FAILURE);
    }

    // Parse imported files
    for (size_t i = 0; i < currentAst->nodes.count; i++) {
        assert(
            currentAst->nodes.items[i] == AST_USE ||
            currentAst->nodes.items[i] == AST_INCLUDE
        );
        LiteralId literal;
        if (currentAst->nodes.items[i] == AST_USE) {
            literal = ast_getData(AstUse, currentAst, i)->path;
        } else {
            literal = ast_getData(AstInclude, currentAst, i)->path;
        }
        StringView view = ast_literalAsStringView(currentAst, literal);
        String canonicalPath = getCanonicalPath(view);
        string_concat(&canonicalPath, (StringView){strlen(".dmd"), ".dmd"});

        // Check that it has not been added
        bool founded = false;
        for (size_t j = 0; j < asts->count; j++) {
            if (string_equals(
                    asts->items[j].canonicalPath.content,
                    canonicalPath.content
                )) {
                founded = true;
                break;
            }
        }

        if (!founded) {
            AstId newAst = asts->count;
            list_append2(asts, (Ast){});
            initAst(&asts->items[newAst], canonicalPath);
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
        for (size_t i = 0; i < asts->count; i++) {
            // If it doens't have imports
            if (asts->items[i].imports.count == 0) {
                bool already = false;
                for (size_t j = 0; j < alreadyAdded.count; j++) {
                    if (alreadyAdded.items[j] == i) {
                        already = true;
                    }
                }
                if (already) continue;

                // Add to current stage of dependecy graph
                list_append(list, i);
                list_append(alreadyAdded, i);
                // Remove from others lists of imports
                for (size_t j = 0; j < asts->count; j++) {
                    list_removeFirstMatch(asts->items[j].imports, i);
                }
            }
        }
        if (list.count == 0) break;
        list_append(graph, list);
    }
    free(alreadyAdded.items);

    return graph;
}

Program compile(char* file) {
    Program program = (Program){List()};
    list_append(program.asts, (Ast){});
    initAst(
        &program.asts.items[0],
        getCanonicalPath((StringView){strlen(file), file})
    );

    // Find dependecy graph
    program.dependencyGraph = findDependencyGraph(&program.asts);

    for (size_t i = 0; i < program.dependencyGraph.count; i++) {
        for (size_t j = 0; j < program.dependencyGraph.items[i].count; j++) {
            AstId ast = program.dependencyGraph.items[i].items[j];
            printf("%s\n", program.asts.items[ast].canonicalPath.content);
        }
        printf("\n");
    }

    // Parse files reverse dependecy graph order

    // Find interface for each module following reverse dependecy graph order

    // Do semantic analysis reverse dependecy graph order
    return program;
}