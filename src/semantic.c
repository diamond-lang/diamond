#include "semantic.h"

#include "ast.h"

// #include <stddef.h>
// #include <stdint.h>

// #include "ast.h"
// #include "common.h"
// #include "scopes.h"
#include "scopes.h"
#include "types.h"

// // NodeId analyzeStatement(Ast* ast, NodeId);
// // NodeId analyzeExpression(Ast* ast, NodeId node);

typedef struct {
    Ast* ast;
    BindingMapStack scopes;
} Context;

bool analyzeTypeDefinition(Context* context, TypeDefinition* typeDefinition);
bool analyzeFunction(Context* context, Function* function);
bool analyzeCode(Context* context, Code* function);
// bool analyzeStatement(Context* context, uint32_t statement);
// bool analyzeExpression(
//     Context* context, uint32_t expression, uint32_t lastNode
// );

bool analyze(Ast* ast) {
    // Initialize context
    Context context;
    context.ast = ast;
    context.scopes = (BindingMapStack)Stack();
    scopes_addScope(&context.scopes);

    // Analyze types
    for (uint32_t i = 0; i < list_size(ast->typeDefinitions); i++) {
        analyzeTypeDefinition(&context, list_get(ast->typeDefinitions, i));
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(ast->functions); i++) {
        analyzeFunction(&context, list_get(ast->functions, i));
    }

    // Analyze code
    analyzeCode(&context, &ast->code);
    return true;
}

bool analyzeTypeDefinition(Context* context, TypeDefinition* typeDefinition) {
    return true;
}

bool analyzeFunction(Context* context, Function* function) { return true; }

bool analyzeCode(Context* context, Code* function) { return true; }