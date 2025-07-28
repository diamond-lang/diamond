#include "semantic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "common.h"
#include "program.h"
#include "scopes.h"
#include "types.h"

typedef struct {
    Ast* ast;
    Scopes scopes;
    Uint32Stack stack;
    uint32_t lasTypeVariable;
    Types temporaryTypes;
} Context;

static void initContext(Context* context, Program program, uint32_t astId) {
    context->ast = list_get(program.asts, astId);
    context->scopes =
        (Scopes){(TypeBindingMap)Hashmap(), (BindingMapStack)Stack()};
    context->lasTypeVariable = 0;
    context->stack = (Uint32Stack)Stack();
    context->temporaryTypes.types = (TypeList)List();
    context->temporaryTypes.parameters = (Uint32List)List();
    scopes_addScope(&context->scopes);
}

static uint32_t literalInContext(Context* context, Ast ast, uint32_t literal) {
    return ast_getLiteral(context->ast, ast_literalAsString(ast, literal));
}

static uint32_t typeInContext(Context* context, Ast ast, uint32_t type) {
    uint32_t newType = None();
    TypeKind kind = ((Type*)list_get(ast.types.types, type))->kind;
    switch (kind) {
    case TYPE_VARIABLE: {
        TypeVariable data = *ast_getTypeVariable(ast.types, type);
        newType = ast_addTypeVariable(&context->ast->types, data.id);
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams data = *ast_getTypeWithParams(ast.types, type);
        newType = ast_addTypeWithParams(
            &context->ast->types,
            literalInContext(context, ast, data.literal),
            data.parameterCount
        );
        uint32_t* parameters = ast_getParameters(ast.types, type);
        for (uint32_t i = 0; i < data.parameterCount; i++) {
            ast_getParameters(context->ast->types, newType)[i] =
                typeInContext(context, ast, parameters[i]);
        }
        break;
    }
    case FUNCTION_TYPE: {
        FunctionType data = *ast_getFunctionType(ast.types, type);
        newType = ast_addFunctionType(
            &context->ast->types,
            typeInContext(context, ast, data.returnType),
            data.parameterCount
        );
        uint32_t* parameters = ast_getParameters(ast.types, type);
        for (uint32_t i = 0; i < data.parameterCount; i++) {
            ast_getParameters(context->ast->types, newType)[i] =
                typeInContext(context, ast, parameters[i]);
        }
        break;
    }
    }
    return newType;
}

static void importModuleUnqualified(Context* context, Ast ast, uint32_t astId) {
    // Add types
    for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
        TypeDefinition typeDef = *list_get(ast.typeDefinitions, i);
        uint32_t identifier = ast_getLiteral(
            context->ast,
            ast_literalAsString(ast, typeDef.identifier)
        );
        TypeBinding* binding = hashmap_get(context->scopes.types, identifier);
        if (binding != NULL) {
            todo();
        }
        TypeBinding newTypeBinding = {i, astId};
        hashmap_set(context->scopes.types, identifier, newTypeBinding);
    }

    // Add function definitions
    BindingMap* currentScope = scopes_current(&context->scopes);
    for (uint32_t i = 0; i < list_size(ast.functions); i++) {
        Function function = *list_get(ast.functions, i);
        if (function.type != None()) {
            uint32_t identifier = ast_getLiteral(
                context->ast,
                ast_literalAsString(ast, function.identifier)
            );
            Binding* binding = hashmap_get(*currentScope, identifier);
            if (binding != NULL) {
                todo();
            }
            uint32_t functionType = typeInContext(context, ast, function.type);
            Binding newBinding =
                (Binding){FUNCTION_BINDING, identifier, astId, functionType};
            hashmap_set(*currentScope, identifier, newBinding);
        }
    }
}

static uint32_t newTypeVariable(Context* context) {
    return context->lasTypeVariable++;
}

static uint32_t getBuiltInType(Context* context, char* type) {
    uint32_t literal = ast_getLiteral(context->ast, type);
    TypeBinding* binding = scopes_getTypeBinding(&context->scopes, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newTypeId = list_size(context->ast->types.types);
    (void)ast_addTypeWithParams(&context->ast->types, literal, 0);
    return newTypeId;
}

static uint32_t _instantiateType(
    Context* context, uint32_t id, Uint32Hashmap* mappings
) {
    TypeKind kind = ((Type*)list_get(context->ast->types.types, id))->kind;
    switch (kind) {
    case TYPE_VARIABLE: {
        unreachable();
        break;
    }
    case TYPE_WITH_PARAMS: {
        // Get type with params
        TypeWithParams type = *ast_getTypeWithParams(context->ast->types, id);
        bool isTypeVariable =
            isLowerCase(ast_literalAsView(*context->ast, type.literal));

        // If is type variable, eg: t, a, b
        if (isTypeVariable) {
            // Instantiate type variable
            assert(type.parameterCount == 0);
            if (hashmap_get(*mappings, type.literal) == NULL) {
                hashmap_set(
                    *mappings,
                    type.literal,
                    ast_addTypeVariable(
                        &context->ast->types,
                        newTypeVariable(context)
                    )
                );
            }
            return *hashmap_get(*mappings, type.literal);
        } else {
            uint32_t firstParameter = list_size(context->ast->types.types);

            // Instantiate parameters
            for (uint32_t i = 0; i < type.parameterCount; i++) {
                uint32_t parameter =
                    ast_getParameters(context->ast->types, id)[i];
                _instantiateType(context, parameter, mappings);
            }
            // Create new type
            uint32_t newId = ast_addTypeWithParams(
                &context->ast->types,
                type.literal,
                type.parameterCount
            );

            // Set type parameters
            uint32_t* newParameters =
                ast_getParameters(context->ast->types, newId);
            for (uint32_t i = 0; i < type.parameterCount; i++) {
                newParameters[i] = firstParameter + i;
            }
            return newId;
        }
    }
    case FUNCTION_TYPE: {
        FunctionType type = *ast_getFunctionType(context->ast->types, id);
        uint32_t firstParameter = list_size(context->ast->types.types);

        // Instantiate arguments
        for (uint32_t i = 0; i < type.parameterCount; i++) {
            uint32_t parameter = ast_getParameters(context->ast->types, id)[i];
            _instantiateType(context, parameter, mappings);
        }

        // Create new type
        uint32_t newReturnType =
            _instantiateType(context, type.returnType, mappings);
        uint32_t newId = ast_addFunctionType(
            &context->ast->types,
            newReturnType,
            type.parameterCount
        );

        // Set type arguments
        uint32_t* newParameters = ast_getParameters(context->ast->types, newId);
        for (uint32_t i = 0; i < type.parameterCount; i++) {
            newParameters[i] = firstParameter + i;
        }
        return newId;
    }
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

static void makeEqual(
    Context* context, uint32_t typeId, uint32_t temporaryType
) {
    todo();
    // TypeKind kind = ((Type*)list_get(context->ast->types.types, typeId))->kind;
    // switch (kind) {
    // case TYPE_VARIABLE: {
    //     list_get(context->ast->types.types, typeId).break;
    // }
    // }
}

static bool analyzeTypeDefinition(
    Context* context, TypeDefinition* typeDefinition
);
static bool analyzeFunction(Context* context, Function* function);
static bool analyzeCode(Context* context, Code* code);
static bool analyzeInstruction(Context* context, Code* code, uint32_t id);

bool analyze(Program program, uint32_t astId) {
    arena_newLifetime();

    // Initialize context
    Context context;
    initContext(&context, program, astId);

    // Add builtin types
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

    arena_destroyCurrentLifetime();
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

    // Add new type variable
    data->type =
        ast_addTypeVariable(&context->ast->types, newTypeVariable(context));

    // Get expression called
    uint32_t called = *stack_get(
        context->stack,
        stack_size(context->stack) - 1 - data->argumentsCount
    );

    // Make call expression type equal to expected type
    uint32_t expected = ast_addFunctionType(
        &context->temporaryTypes,
        data->type,
        data->argumentsCount
    );
    uint32_t* parameters = ast_getParameters(context->temporaryTypes, expected);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount
        );
        parameters[i] = ast_getType(*context->ast, arg);
    }
    makeEqual(context, ast_getType(*context->ast, called), expected);
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
    stack_pop(context->stack);
    stack_pop(context->stack);
    stack_push(context->stack, id);
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
    stack_push(context->stack, id);
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
    stack_push(context->stack, id);
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