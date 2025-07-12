#include "semantic.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"

// #include <stddef.h>
// #include <stdint.h>

// #include "ast.h"
// #include "common.h"
// #include "scopes.h"
#include "common.h"
#include "program.h"
#include "scopes.h"
#include "types.h"

typedef struct {
    Ast* ast;
    Scopes scopes;
    Uint32Stack stack;
} Context;

static uint32_t getBuiltInType(Context* context, char* type) {
    StringView view = cStringAsView(type);
    uint32_t literal = ast_getLiteral(&context->ast->literals, view);
    TypeBinding* binding = scopes_getTypeBinding(&context->scopes, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newTypeId = ast_createTypeApplication(context->ast);
    ast_getTypeApplication(*context->ast, newTypeId)->identifier = literal;
    return newTypeId;
}

static void importModuleUnqualified(Context* context, Ast ast, uint32_t astId) {
    // Add types
    for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
        TypeDefinition typeDef = *list_get(ast.typeDefinitions, i);
        uint32_t key = ast_getLiteral(
            &context->ast->literals,
            ast_literalAsStringView(ast, typeDef.identifier)
        );
        TypeBinding* binding = hashmap_get(context->scopes.types, key);
        if (binding != NULL) {
            todo();
        }
        TypeBinding newTypeBinding = {i, astId};
        hashmap_set(context->scopes.types, key, newTypeBinding);
    }
}

static bool analyzeTypeDefinition(
    Context* context, TypeDefinition* typeDefinition
);
static bool analyzeFunction(Context* context, Function* function);
static bool analyzeCode(Context* context, Code* code);

bool analyze(Program program, uint32_t astId) {
    // Initialize context
    Context context;
    context.ast = list_get(program.asts, astId);
    context.scopes =
        (Scopes){(TypeBindingMap)Hashmap(), (BindingMapStack)Stack()};
    scopes_addScope(&context.scopes);

    // Import core
    importModuleUnqualified(&context, program.builtin, None());

    // Analyze types
    for (uint32_t i = 0; i < list_size(context.ast->typeDefinitions); i++) {
        analyzeTypeDefinition(
            &context,
            list_get(context.ast->typeDefinitions, i)
        );
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        analyzeFunction(&context, list_get(context.ast->functions, i));
    }

    // Analyze code
    analyzeCode(&context, &context.ast->code);
    return true;
}

bool analyzeTypeDefinition(Context* context, TypeDefinition* typeDefinition) {
    todo();
}

bool analyzeFunction(Context* context, Function* function) { todo(); }

bool analyzeCode(Context* context, Code* code) {
    for (uint32_t inst = 0; inst < list_size(code->instructions); inst++) {
        AstInstructionKind kind = *list_get(code->instructions, inst);
        switch (kind) {
            case AST_DECLARATION: todo(); break;
            case AST_ASSIGNMENT: todo(); break;
            case AST_RETURN: todo(); break;
            case AST_RETURN_EXPRESSION: todo(); break;
            case AST_BREAK: todo(); break;
            case AST_CONTINUE: todo(); break;
            case AST_IF_ELSE: todo(); break;
            case AST_WHILE: todo(); break;
            case AST_CALL: todo(); break;
            case AST_IF_ELSE_EXPRESSION: todo(); break;
            case AST_NOT: todo(); break;
            case AST_OR: todo(); break;
            case AST_AND: todo(); break;
            case AST_EQUAL_EQUAL: todo(); break;
            case AST_NOT_EQUAL: todo(); break;
            case AST_LESS: todo(); break;
            case AST_LESS_EQUAL: todo(); break;
            case AST_GREATER: todo(); break;
            case AST_GREATER_EQUAL: todo(); break;
            case AST_ADD: todo(); break;
            case AST_SUBTRACT: todo(); break;
            case AST_MUL: todo(); break;
            case AST_DIV: todo(); break;
            case AST_MOD: todo(); break;
            case AST_NEGATION: todo(); break;
            case AST_DEREFERENCE: todo(); break;
            case AST_ADDRESS_OF: todo(); break;
            case AST_FIELD_ACCESS: todo(); break;
            case AST_INDEX_ACCESS: todo(); break;
            case AST_FLOAT: todo(); break;
            case AST_INTEGER: todo(); break;
            case AST_IDENTIFIER: todo(); break;
            case AST_BOOLEAN: {
                uint32_t type = getBuiltInType(context, "Bool");
                ast_getData(AstBoolean, code, inst)->type = type;
                break;
            }
            case AST_STRING: todo(); break;
            case AST_ARRAY: todo(); break;
            case AST_STRUCT_LITERAL: todo(); break;
        }
    }
    return true;
}