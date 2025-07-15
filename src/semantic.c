#include "semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
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
    Uint32Stack expressions;
    uint32_t lasTypeVariable;
} Context;

static uint32_t newTypeVariable(Context* context) {
    return context->lasTypeVariable++;
}

static void addBuiltins(Context* context) {
    // Builtin types
    uint32_t literal;
    TypeBinding newType = (TypeBinding){None(), None()};

    // Bool
    literal = ast_getLiteral(context->ast, cStringAsView("Bool"));
    hashmap_set(context->scopes.types, literal, newType);

    // None
    literal = ast_getLiteral(context->ast, cStringAsView("None"));
    hashmap_set(context->scopes.types, literal, newType);

    // Float64
    literal = ast_getLiteral(context->ast, cStringAsView("Float64"));
    hashmap_set(context->scopes.types, literal, newType);

    // ->
    literal = ast_getLiteral(context->ast, cStringAsView("->"));
    hashmap_set(context->scopes.types, literal, newType);

    // Builtin function print
    uint32_t functionType = ast_createTypeApplication(context->ast);
    ast_getTypeApplication(*context->ast, functionType)->parameterCount = 2;
    ast_getTypeApplication(*context->ast, functionType)->literal =
        ast_getLiteral(context->ast, cStringAsView("->"));

    uint32_t parameter1 = ast_createTypeApplication(context->ast);
    ast_getTypeApplication(*context->ast, parameter1)->literal =
        ast_getLiteral(context->ast, cStringAsView("t"));

    uint32_t parameter2 = ast_createTypeApplication(context->ast);
    ast_getTypeApplication(*context->ast, parameter2)->literal =
        ast_getLiteral(context->ast, cStringAsView("None"));

    Binding binding = {BUILTIN_BINDING, None(), None(), functionType};
    scopes_addBinding(
        scopes_current(&context->scopes),
        ast_getLiteral(context->ast, cStringAsView("print")),
        binding
    );
}

static uint32_t getBuiltInType(Context* context, char* type) {
    StringView view = cStringAsView(type);
    uint32_t literal = ast_getLiteral(context->ast, view);
    TypeBinding* binding = scopes_getTypeBinding(&context->scopes, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newTypeId = ast_createTypeApplication(context->ast);
    ast_getTypeApplication(*context->ast, newTypeId)->literal = literal;
    return newTypeId;
}

static uint32_t _instantiateType(
    Context* context, uint32_t id, Uint32Hashmap* mappings
) {
    Type* type = list_get(context->ast->types, id);
    if (type->kind == TYPE_APPLICATION) {
        TypeApplication* app = ast_getTypeApplication(*context->ast, id);
        bool isTypeVariable =
            isLowerCase(ast_literalAsStringView(*context->ast, app->literal));
        if (isTypeVariable) {
            assert(app->parameterCount == 0);
            if (hashmap_get(*mappings, app->literal) != NULL) {
                return *hashmap_get(*mappings, app->literal);
            }
            hashmap_set(
                *mappings,
                app->literal,
                ast_createTypeVariable(context->ast, newTypeVariable(context))
            );
            return *hashmap_get(*mappings, app->literal);
        } else {
            uint32_t newId = ast_createTypeApplication(context->ast);
            ast_getTypeApplication(*context->ast, newId)->literal =
                app->literal;
            uint32_t parameterCount =
                ast_getTypeApplication(*context->ast, id)->parameterCount;
            for (uint32_t i = id + 1; parameterCount > 0; parameterCount--) {
                _instantiateType(context, i, mappings);
                i = ast_getNextParameter(context->ast, i);
            }
            ast_getTypeApplication(*context->ast, newId)->parameterCount =
                ast_getTypeApplication(*context->ast, id)->parameterCount;
            return newId;
        }
    } else if (type->kind == TYPE_VARIABLE) {
    }
    return None();
}

static uint32_t instantiateType(Context* context, uint32_t id) {
    arena_newLifetime();
    Uint32Hashmap mappings = (Uint32Hashmap)Hashmap();
    uint32_t result = _instantiateType(context, id, &mappings);
    arena_destroyCurrentLifetime();
    return result;
}

// static void importModuleUnqualified(Context* context, Ast ast, uint32_t astId) {
//     // Add types
//     for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
//         TypeDefinition typeDef = *list_get(ast.typeDefinitions, i);
//         uint32_t key = ast_getLiteral(
//             context->ast,
//             ast_literalAsStringView(ast, typeDef.identifier)
//         );
//         TypeBinding* binding = hashmap_get(context->scopes.types, key);
//         if (binding != NULL) {
//             todo();
//         }
//         TypeBinding newTypeBinding = {i, astId};
//         hashmap_set(context->scopes.types, key, newTypeBinding);
//     }
// }

static bool analyzeTypeDefinition(
    Context* context, TypeDefinition* typeDefinition
);
static bool analyzeFunction(Context* context, Function* function);
static bool analyzeCode(Context* context, Code* code);
static bool analyzeInstruction(Context* context, Code* code, uint32_t id);

bool analyze(Program program, uint32_t astId) {
    // Initialize context
    Context context;
    context.ast = list_get(program.asts, astId);
    context.scopes =
        (Scopes){(TypeBindingMap)Hashmap(), (BindingMapStack)Stack()};
    context.lasTypeVariable = 0;
    context.expressions = (Uint32Stack)Stack();
    scopes_addScope(&context.scopes);

    // Add builtin types
    addBuiltins(&context);

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
    for (uint32_t id = 0; id < list_size(code->instructions); id++) {
        bool result = analyzeInstruction(context, code, id);
        if (!result) return false;
    }
    return true;
}

static bool declaration(
    Context* ctx, Code* code, uint32_t id, AstDeclaration* data
) {
    todo();
}

static bool assignemnt(
    Context* context, Code* code, uint32_t id, AstAssignment* data
) {
    todo();
}

static bool returnStmt(
    Context* context, Code* code, uint32_t id, AstReturn* data
) {
    todo();
}

static bool returnExpression(
    Context* context, Code* code, uint32_t id, AstReturnExpression* data
) {
    todo();
}

static bool breakStmt(
    Context* context, Code* code, uint32_t id, AstBreak* data
) {
    todo();
}

static bool continueStmt(
    Context* context, Code* code, uint32_t id, AstContinue* data
) {
    todo();
}

static bool ifElse(Context* context, Code* code, uint32_t id, AstIfElse* data) {
    todo();
}

static bool whileStmt(
    Context* context, Code* code, uint32_t id, AstWhile* data
) {
    todo();
}

static bool call(Context* context, Code* code, uint32_t id, AstCall* data) {
    assert(*list_get(code->instructions, id) == AST_CALL);
    uint32_t argsCount = data->argumentsCount;
    while (argsCount != 0) {
        stack_pop(context->expressions);
        argsCount--;
    }
    stack_pop(context->expressions);
    stack_push(context->expressions, id);
    return true;
}
static bool ifElseExpression(
    Context* context, Code* code, uint32_t id, AstIfElseExpression* data
) {
    todo();
}

static bool not(Context * context, Code* code, uint32_t id, AstNot* data) {
    todo();
}

static bool orExpr(Context* context, Code* code, uint32_t id, AstOr* data) {
    todo();
}

static bool andExpr(Context* context, Code* code, uint32_t id, AstAnd* data) {
    todo();
}

static bool equalEqual(
    Context* context, Code* code, uint32_t id, AstEqualEqual* data
) {
    todo();
}

static bool notEqual(
    Context* context, Code* code, uint32_t id, AstNotEqual* data
) {
    todo();
}

static bool less(Context* context, Code* code, uint32_t id, AstLess* data) {
    todo();
}

static bool lessEqual(
    Context* context, Code* code, uint32_t id, AstLessEqual* data
) {
    todo();
}

static bool greater(
    Context* context, Code* code, uint32_t id, AstGreater* data
) {
    todo();
}

static bool greaterEqual(
    Context* context, Code* code, uint32_t id, AstGreaterEqual* data
) {
    todo();
}

static bool add(Context* context, Code* code, uint32_t id, AstAdd* data) {
    stack_pop(context->expressions);
    stack_pop(context->expressions);
    stack_push(context->expressions, id);
    data->type = getBuiltInType(context, "Float64");
    return true;
}

static bool subtract(
    Context* context, Code* code, uint32_t id, AstSubtract* data
) {
    todo();
}

static bool mul(Context* context, Code* code, uint32_t id, AstMul* data) {
    todo();
}

static bool divExpr(Context* context, Code* code, uint32_t id, AstDiv* data) {
    todo();
}

static bool mod(Context* context, Code* code, uint32_t id, AstMod* data) {
    todo();
}

static bool negation(
    Context* context, Code* code, uint32_t id, AstNegation* data
) {
    todo();
}

static bool addressOf(
    Context* context, Code* code, uint32_t id, AstAddressOf* data
) {
    todo();
}

static bool dereference(
    Context* context, Code* code, uint32_t id, AstDereference* data
) {
    todo();
}

static bool fieldAccess(
    Context* context, Code* code, uint32_t id, AstFieldAccess* data
) {
    todo();
}

static bool indexAccess(
    Context* context, Code* code, uint32_t id, AstIndexAccess* data
) {
    todo();
}

static bool floatLiteral(
    Context* context, Code* code, uint32_t id, AstFloat* data
) {
    data->type = getBuiltInType(context, "Float64");
    stack_push(context->expressions, id);
    return true;
}

static bool integerLiteral(
    Context* context, Code* code, uint32_t id, AstInteger* data
) {
    todo();
}

static bool identifier(
    Context* context, Code* code, uint32_t id, AstIdentifier* data
) {
    Binding* binding = scopes_getBinding(&context->scopes, data->literal);
    if (binding == NULL) {
        todo();
    }
    data->type = instantiateType(context, binding->type);
    stack_push(context->expressions, id);
    return true;
}

static bool booleanLiteral(
    Context* context, Code* code, uint32_t id, AstBoolean* data
) {
    uint32_t type = getBuiltInType(context, "Bool");
    data->type = type;
    return true;
}

static bool stringLiteral(
    Context* context, Code* code, uint32_t id, AstString* data
) {
    todo();
}

static bool arrayLiteral(
    Context* context, Code* code, uint32_t id, AstArray* data
) {
    todo();
}

static bool structLiteral(
    Context* context, Code* code, uint32_t id, AstStructLiteral* data
) {
    todo();
}

static bool analyzeInstruction(Context* context, Code* code, uint32_t id) {
    AstInstructionKind kind = *list_get(code->instructions, id);
    void* data = ast_getData(code, id);
    switch (kind) {
    case AST_DECLARATION: return declaration(context, code, id, data);
    case AST_ASSIGNMENT: return assignemnt(context, code, id, data);
    case AST_RETURN: return returnStmt(context, code, id, data);
    case AST_RETURN_EXPRESSION:
        return returnExpression(context, code, id, data);
    case AST_BREAK: return breakStmt(context, code, id, data);
    case AST_CONTINUE: return continueStmt(context, code, id, data);
    case AST_IF_ELSE: return ifElse(context, code, id, data);
    case AST_WHILE: return whileStmt(context, code, id, data);
    case AST_CALL: return call(context, code, id, data);
    case AST_IF_ELSE_EXPRESSION: ifElseExpression(context, code, id, data);
    case AST_NOT: return not(context, code, id, data);
    case AST_OR: return orExpr(context, code, id, data);
    case AST_AND: return andExpr(context, code, id, data);
    case AST_EQUAL_EQUAL: return equalEqual(context, code, id, data);
    case AST_NOT_EQUAL: return notEqual(context, code, id, data);
    case AST_LESS: return less(context, code, id, data);
    case AST_LESS_EQUAL: return lessEqual(context, code, id, data);
    case AST_GREATER: return greater(context, code, id, data);
    case AST_GREATER_EQUAL: return greaterEqual(context, code, id, data);
    case AST_ADD: return add(context, code, id, data);
    case AST_SUBTRACT: return subtract(context, code, id, data);
    case AST_MUL: return mul(context, code, id, data);
    case AST_DIV: return divExpr(context, code, id, data);
    case AST_MOD: return mod(context, code, id, data);
    case AST_NEGATION: return negation(context, code, id, data);
    case AST_ADDRESS_OF: return addressOf(context, code, id, data);
    case AST_DEREFERENCE: return dereference(context, code, id, data);
    case AST_FIELD_ACCESS: return fieldAccess(context, code, id, data);
    case AST_INDEX_ACCESS: return indexAccess(context, code, id, data);
    case AST_FLOAT: return floatLiteral(context, code, id, data);
    case AST_INTEGER: return integerLiteral(context, code, id, data);
    case AST_IDENTIFIER: return identifier(context, code, id, data);
    case AST_BOOLEAN: return booleanLiteral(context, code, id, data);
    case AST_STRING: return stringLiteral(context, code, id, data);
    case AST_ARRAY: return arrayLiteral(context, code, id, data);
    case AST_STRUCT_LITERAL: return structLiteral(context, code, id, data);
    default: unreachable();
    }
}