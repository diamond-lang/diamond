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
    uint32_t lastTypeVariable;
    Arena arena;
} Context;

static uint32_t literalInContext(Context* context, Ast ast, uint32_t literal) {
    return ast_getLiteral(context->ast, ast_literalAsString(ast, literal));
}

static uint32_t typeInContext(Context* context, Ast ast, uint32_t typeId) {
    uint32_t newType = None();
    Type* type = ast_getType(ast.types, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        TypeVariable data = type->variable;
        newType = ast_addTypeVariable(
            &context->ast->arena,
            &context->ast->types,
            data.id
        );
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams data = type->withParams;
        newType = ast_addTypeWithParams(
            &context->ast->arena,
            &context->ast->types,
            literalInContext(context, ast, data.literal),
            data.parameterCount
        );
        TypeWithParams* newData =
            &ast_getType(context->ast->types, newType)->withParams;
        uint32_t* params =
            arena_getPointer(ast.arena, uint32_t, data.parameters);
        uint32_t* newParams = arena_getPointer(
            context->ast->arena,
            uint32_t,
            newData->parameters
        );
        for (uint32_t i = 0; i < data.parameterCount; i++) {
            newParams[i] = typeInContext(context, ast, params[i]);
        }
        break;
    }
    case FUNCTION_TYPE: {
        FunctionType data = type->functionType;
        newType = ast_addFunctionType(
            &context->ast->arena,
            &context->ast->types,
            typeInContext(context, ast, data.returnType),
            data.argumentsCount
        );
        FunctionType* newData =
            &ast_getType(context->ast->types, newType)->functionType;
        uint32_t* args = arena_getPointer(ast.arena, uint32_t, data.arguments);
        uint32_t* newArgs =
            arena_getPointer(context->ast->arena, uint32_t, newData->arguments);
        for (uint32_t i = 0; i < data.argumentsCount; i++) {
            newArgs[i] = typeInContext(context, ast, args[i]);
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
        hashmap_set(
            &context->arena,
            context->scopes.types,
            identifier,
            newTypeBinding
        );
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
            hashmap_set(&context->arena, *currentScope, identifier, newBinding);
        }
    }
}

static uint32_t newTypeVariable(Context* context) {
    return context->lastTypeVariable++;
}

static uint32_t getBuiltInType(Context* context, char* type) {
    uint32_t literal = ast_getLiteral(context->ast, type);
    TypeBinding* binding = scopes_getTypeBinding(&context->scopes, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newType = ast_addTypeWithParams(
        &context->ast->arena,
        &context->ast->types,
        literal,
        0
    );
    return newType;
}

static uint32_t _instantiateType(
    Context* context, uint32_t typeId, Uint32Hashmap* mappings, Arena* arena
) {
    Type* type = ast_getType(context->ast->types, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        unreachable();
        break;
    }
    case TYPE_WITH_PARAMS: {
        // Get type with params
        TypeWithParams data = type->withParams;
        bool isTypeVariable =
            isLowerCase(ast_literalAsView(*context->ast, data.literal));

        // If is type variable, eg: t, a, b
        if (isTypeVariable) {
            // Instantiate type variable
            assert(data.parameterCount == 0);
            if (hashmap_get(*mappings, data.literal) == NULL) {
                hashmap_set(
                    arena,
                    *mappings,
                    data.literal,
                    ast_addTypeVariable(
                        &context->ast->arena,
                        &context->ast->types,
                        newTypeVariable(context)
                    )
                );
            }
            return *hashmap_get(*mappings, data.literal);
        } else {
            uint32_t firstParameter = list_size(context->ast->types) + 1;

            // Instantiate parameters
            uint32_t* params = arena_getPointer(
                context->ast->arena,
                uint32_t,
                data.parameters
            );
            for (uint32_t i = 0; i < data.parameterCount; i++) {
                uint32_t parameter = params[i];
                _instantiateType(context, parameter, mappings, arena);
            }

            // Create new type
            uint32_t newId = ast_addTypeWithParams(
                &context->ast->arena,
                &context->ast->types,
                data.literal,
                data.parameterCount
            );
            TypeWithParams newData =
                ast_getType(context->ast->types, newId)->withParams;

            // Set type parameters
            uint32_t* newParams = arena_getPointer(
                context->ast->arena,
                uint32_t,
                newData.parameters
            );
            for (uint32_t i = 0; i < data.parameterCount; i++) {
                Type* param =
                    ast_getType(context->ast->types, firstParameter + i);
                newParams[i] = arena_getId(context->ast->arena, param);
            }
            return newId;
        }
    }
    case FUNCTION_TYPE: {
        FunctionType data = type->functionType;
        uint32_t firstArgument = list_size(context->ast->types) + 1;

        // Instantiate arguments
        uint32_t* args =
            arena_getPointer(context->ast->arena, uint32_t, data.arguments);
        for (uint32_t i = 0; i < data.argumentsCount; i++) {
            uint32_t argument = args[i];
            _instantiateType(context, argument, mappings, arena);
        }

        // Create new type
        uint32_t newReturnTypeId =
            _instantiateType(context, data.returnType, mappings, arena);
        Type* newReturnType = ast_getType(context->ast->types, newReturnTypeId);
        uint32_t newId = ast_addFunctionType(
            &context->ast->arena,
            &context->ast->types,
            arena_getId(context->ast->arena, newReturnType),
            data.argumentsCount
        );
        FunctionType newData =
            ast_getType(context->ast->types, newId)->functionType;

        // Set type arguments
        uint32_t* newArgs =
            arena_getPointer(context->ast->arena, uint32_t, newData.arguments);
        for (uint32_t i = 0; i < data.argumentsCount; i++) {
            Type* arg = ast_getType(context->ast->types, firstArgument + i);
            newArgs[i] = arena_getId(context->ast->arena, arg);
        }
        return newId;
    }
    }
    return None();
}

static uint32_t instantiateType(Context* context, uint32_t id, Arena scratch) {
    Uint32Hashmap mappings = {0};
    uint32_t result = _instantiateType(context, id, &mappings, &scratch);
    return result;
}

static void makeEqual(
    Context* context,
    Type* type,
    Arena typeArena,
    Type* otherType,
    Arena otherTypeArena,
    Arena scratch
) {
    TypeKind kind = type->kind;
    TypeKind kindExpected = otherType->kind;
    String s1 = ast_typeAsString(&scratch, *context->ast, type, typeArena);
    String s2 =
        ast_typeAsString(&scratch, *context->ast, otherType, otherTypeArena);
    printf("HERE!: %s | %s \n\n", s1.buffer, s2.buffer);
    switch (kind) {
    case TYPE_VARIABLE: {
        switch (kindExpected) {
        case TYPE_VARIABLE: {
            *type = *otherType;
            break;
        }
        case TYPE_WITH_PARAMS: {
            *type = *otherType;
            if (typeArena.buffer != otherTypeArena.buffer) {
                TypeWithParams* data = &type->withParams;
                uint32_t* params = arena_getPointer(
                    context->ast->arena,
                    uint32_t,
                    data->parameters
                );
                uint32_t* newParams = arena_alloc(
                    &context->ast->arena,
                    uint32_t,
                    data->parameterCount
                );
                for (uint32_t i = 0; i < data->parameterCount; i++) {
                    newParams[i] = params[i];
                }
                data->parameters = arena_getId(context->ast->arena, newParams);
            }
            break;
        }
        case FUNCTION_TYPE: {
            *type = *otherType;
            if (typeArena.buffer != otherTypeArena.buffer) {
                FunctionType* data = &type->functionType;
                uint32_t* args = arena_getPointer(
                    context->ast->arena,
                    uint32_t,
                    data->arguments
                );
                uint32_t* newArgs = arena_alloc(
                    &context->ast->arena,
                    uint32_t,
                    data->argumentsCount
                );
                for (uint32_t i = 0; i < data->argumentsCount; i++) {
                    newArgs[i] = args[i];
                }
                data->arguments = arena_getId(context->ast->arena, newArgs);
            }
            break;
        }
        }
        break;
    }
    case TYPE_WITH_PARAMS: {
        switch (kindExpected) {
        case TYPE_VARIABLE: unreachable();
        case TYPE_WITH_PARAMS: {
            TypeWithParams* data = &type->withParams;
            TypeWithParams* otherData = &otherType->withParams;
            if (data->literal != otherData->literal) {
                todo();
            }
            if (data->parameterCount != otherData->parameterCount) {
                todo();
            }
            uint32_t* params =
                arena_getPointer(typeArena, uint32_t, data->parameters);
            uint32_t* otherParams = arena_getPointer(
                otherTypeArena,
                uint32_t,
                otherData->parameters
            );
            for (uint32_t i = 0; i < data->parameterCount; i++) {
                Type* param =
                    arena_getPointer(context->ast->arena, Type, params[i]);
                Type* otherParam =
                    arena_getPointer(context->ast->arena, Type, otherParams[i]);
                if (param->kind == TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        param,
                        context->ast->arena,
                        otherParam,
                        context->ast->arena,
                        scratch
                    );
                } else if (otherParam->kind == TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        otherParam,
                        context->ast->arena,
                        param,
                        context->ast->arena,
                        scratch
                    );
                } else {
                    makeEqual(
                        context,
                        param,
                        context->ast->arena,
                        otherParam,
                        context->ast->arena,
                        scratch
                    );
                }
            }
            break;
        }
        case FUNCTION_TYPE: todo();
        }
        break;
    }
    case FUNCTION_TYPE: {
        switch (kindExpected) {
        case TYPE_VARIABLE: unreachable();
        case TYPE_WITH_PARAMS: {
            todo();
        }
        case FUNCTION_TYPE: {
            FunctionType* data = &type->functionType;
            FunctionType* otherData = &otherType->functionType;
            if (data->argumentsCount != otherData->argumentsCount) {
                todo();
            }
            uint32_t* args =
                arena_getPointer(typeArena, uint32_t, data->arguments);
            uint32_t* otherArgs = arena_getPointer(
                otherTypeArena,
                uint32_t,
                otherData->arguments
            );
            for (uint32_t i = 0; i < data->argumentsCount; i++) {
                Type* arg =
                    arena_getPointer(context->ast->arena, Type, args[i]);
                Type* otherArg =
                    arena_getPointer(context->ast->arena, Type, otherArgs[i]);
                if (arg->kind == TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        arg,
                        context->ast->arena,
                        otherArg,
                        context->ast->arena,
                        scratch
                    );
                } else if (otherArg->kind == TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        otherArg,
                        context->ast->arena,
                        arg,
                        context->ast->arena,
                        scratch
                    );
                } else {
                    makeEqual(
                        context,
                        arg,
                        context->ast->arena,
                        otherArg,
                        context->ast->arena,
                        scratch
                    );
                }
            }
            Type* returnType =
                arena_getPointer(context->ast->arena, Type, data->returnType);
            Type* otherReturnType = arena_getPointer(
                context->ast->arena,
                Type,
                otherData->returnType
            );
            if (returnType->kind == TYPE_VARIABLE) {
                makeEqual(
                    context,
                    returnType,
                    context->ast->arena,
                    otherReturnType,
                    context->ast->arena,
                    scratch
                );
            } else if (otherReturnType->kind == TYPE_VARIABLE) {
                makeEqual(
                    context,
                    otherReturnType,
                    context->ast->arena,
                    returnType,
                    context->ast->arena,
                    scratch
                );
            } else {
                makeEqual(
                    context,
                    returnType,
                    context->ast->arena,
                    otherReturnType,
                    context->ast->arena,
                    scratch
                );
            }
            break;
        }
        }
        break;
    }
    }
}

static bool declaration(
    Context* ctx, Code* code, uint32_t id, AstDeclaration* data, Arena scratch
) {
    todo();
}

static bool assignemnt(
    Context* context,
    Code* code,
    uint32_t id,
    AstAssignment* data,
    Arena scratch
) {
    todo();
}

static bool returnStmt(
    Context* context, Code* code, uint32_t id, AstReturn* data, Arena scratch
) {
    todo();
}

static bool returnExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstReturnExpression* data,
    Arena scratch
) {
    todo();
}

static bool breakStmt(
    Context* context, Code* code, uint32_t id, AstBreak* data, Arena scratch
) {
    todo();
}

static bool continueStmt(
    Context* context, Code* code, uint32_t id, AstContinue* data, Arena scratch
) {
    todo();
}

static bool ifElse(
    Context* context, Code* code, uint32_t id, AstIfElse* data, Arena scratch
) {
    todo();
}

static bool whileStmt(
    Context* context, Code* code, uint32_t id, AstWhile* data, Arena scratch
) {
    todo();
}

static bool call(
    Context* context, Code* code, uint32_t id, AstCall* data, Arena scratch
) {
    assert(ast_getInstruction(*code, id) == AST_CALL);

    // Add new type variable
    data->type = ast_addTypeVariable(
        &context->ast->arena,
        &context->ast->types,
        newTypeVariable(context)
    );
    Type* dataType = ast_getType(context->ast->types, data->type);

    // Get expression called and it's type
    uint32_t called = *stack_get(
        context->stack,
        stack_size(context->stack) - 1 - data->argumentsCount
    );
    uint32_t calledTypeId = ast_getTypeOfInstruction(*context->ast, called);
    Type* calledType = ast_getType(context->ast->types, calledTypeId);

    // Construct expected type
    TypeList temporary = {0};
    uint32_t expectedId = ast_addFunctionType(
        &scratch,
        &temporary,
        arena_getId(context->ast->arena, dataType),
        data->argumentsCount
    );
    Type* expected = ast_getType(temporary, expectedId);
    uint32_t* arguments =
        arena_getPointer(scratch, uint32_t, expected->functionType.arguments);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount
        );
        uint32_t argTypeId = ast_getTypeOfInstruction(*context->ast, arg);
        Type* argType = ast_getType(context->ast->types, argTypeId);
        arguments[i] = arena_getId(context->ast->arena, argType);
    }

    // Make type of called expression equal to expected type
    makeEqual(
        context,
        calledType,
        context->ast->arena,
        expected,
        scratch,
        scratch
    );
    return true;
}

static bool ifElseExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstIfElseExpression* data,
    Arena scratch
) {
    todo();
}

static bool not(
    Context * context, Code* code, uint32_t id, AstNot* data, Arena scratch
) {
    todo();
}

static bool orExpr(
    Context* context, Code* code, uint32_t id, AstOr* data, Arena scratch
) {
    todo();
}

static bool andExpr(
    Context* context, Code* code, uint32_t id, AstAnd* data, Arena scratch
) {
    todo();
}

static bool equalEqual(
    Context* context,
    Code* code,
    uint32_t id,
    AstEqualEqual* data,
    Arena scratch
) {
    todo();
}

static bool notEqual(
    Context* context, Code* code, uint32_t id, AstNotEqual* data, Arena scratch
) {
    todo();
}

static bool less(
    Context* context, Code* code, uint32_t id, AstLess* data, Arena scratch
) {
    todo();
}

static bool lessEqual(
    Context* context, Code* code, uint32_t id, AstLessEqual* data, Arena scratch
) {
    todo();
}

static bool greater(
    Context* context, Code* code, uint32_t id, AstGreater* data, Arena scratch
) {
    todo();
}

static bool greaterEqual(
    Context* context,
    Code* code,
    uint32_t id,
    AstGreaterEqual* data,
    Arena scratch
) {
    todo();
}

static bool add(
    Context* context, Code* code, uint32_t id, AstAdd* data, Arena scratch
) {
    // stack_pop(context->stack);
    // stack_pop(context->stack);
    // stack_push(&context->arena, context->stack, id);
    // data->type = getBuiltInType(context, "Float64");

    // Construct add type
    uint32_t resultTypeId = ast_addTypeVariable(
        &context->ast->arena,
        &context->ast->types,
        newTypeVariable(context)
    );
    Type* resultType = ast_getType(context->ast->types, resultTypeId);
    uint32_t operatorTypeId = ast_addFunctionType(
        &context->ast->arena,
        &context->ast->types,
        arena_getId(context->ast->arena, resultType),
        2
    );
    Type* operatorType = ast_getType(context->ast->types, operatorTypeId);
    uint32_t* args = arena_getPointer(
        context->ast->arena,
        uint32_t,
        operatorType->functionType.arguments
    );
    for (uint32_t i = 0; i < 2; i++) {
        args[i] = arena_getId(context->ast->arena, resultType);
    }
    String s1 = ast_typeAsString(
        &scratch,
        *context->ast,
        operatorType,
        context->ast->arena
    );
    //String s2 = ast_typeAsString(&scratch, *context->ast, otherType);
    printf("%s !!\n", s1.buffer);

    // Construct expected type
    TypeList temporary = {0};
    data->type = ast_addTypeVariable(
        &context->ast->arena,
        &context->ast->types,
        newTypeVariable(context)
    );
    Type* dataType = ast_getType(context->ast->types, data->type);
    uint32_t expectedId = ast_addFunctionType(
        &scratch,
        &temporary,
        arena_getId(context->ast->arena, dataType),
        2
    );
    Type* expected = ast_getType(temporary, expectedId);
    uint32_t* arguments =
        arena_getPointer(scratch, uint32_t, expected->functionType.arguments);
    for (uint32_t i = 0; i < 2; i++) {
        uint32_t arg =
            *stack_get(context->stack, stack_size(context->stack) - 2);
        uint32_t argTypeId = ast_getTypeOfInstruction(*context->ast, arg);
        Type* argType = ast_getType(context->ast->types, argTypeId);
        arguments[i] = arena_getId(context->ast->arena, argType);
    }

    String s2 = ast_typeAsString(&scratch, *context->ast, expected, scratch);
    printf("%s !!!\n", s2.buffer);

    // Make type of called expression equal to expected type
    makeEqual(
        context,
        operatorType,
        context->ast->arena,
        expected,
        scratch,
        scratch
    );

    // Modify stack
    stack_pop(context->stack);
    stack_pop(context->stack);
    stack_push(&context->arena, context->stack, id);

    // Return
    return true;
}

static bool subtract(
    Context* context, Code* code, uint32_t id, AstSubtract* data, Arena scratch
) {
    todo();
}

static bool mul(
    Context* context, Code* code, uint32_t id, AstMul* data, Arena scratch
) {
    todo();
}

static bool divExpr(
    Context* context, Code* code, uint32_t id, AstDiv* data, Arena scratch
) {
    todo();
}

static bool mod(
    Context* context, Code* code, uint32_t id, AstMod* data, Arena scratch
) {
    todo();
}

static bool negation(
    Context* context, Code* code, uint32_t id, AstNegation* data, Arena scratch
) {
    todo();
}

static bool addressOf(
    Context* context, Code* code, uint32_t id, AstAddressOf* data, Arena scratch
) {
    todo();
}

static bool dereference(
    Context* context,
    Code* code,
    uint32_t id,
    AstDereference* data,
    Arena scratch
) {
    todo();
}

static bool fieldAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstFieldAccess* data,
    Arena scratch
) {
    todo();
}

static bool indexAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstIndexAccess* data,
    Arena scratch
) {
    todo();
}

static bool floatLiteral(
    Context* context, Code* code, uint32_t id, AstFloat* data, Arena scratch
) {
    data->type = getBuiltInType(context, "Float64");
    stack_push(&context->arena, context->stack, id);
    return true;
}

static bool integerLiteral(
    Context* context, Code* code, uint32_t id, AstInteger* data, Arena scratch
) {
    todo();
}

static bool identifier(
    Context* context,
    Code* code,
    uint32_t id,
    AstIdentifier* data,
    Arena scratch
) {
    Binding* binding = scopes_getBinding(&context->scopes, data->literal);
    if (binding == NULL) {
        todo();
    }
    data->type = instantiateType(context, binding->type, scratch);
    String string = ast_typeAsString(
        &scratch,
        *context->ast,
        ast_getType(context->ast->types, data->type),
        context->ast->arena
    );
    printf("%s\n", string.buffer);
    stack_push(&context->arena, context->stack, id);
    return true;
}

static bool booleanLiteral(
    Context* context, Code* code, uint32_t id, AstBoolean* data, Arena scratch
) {
    uint32_t type = getBuiltInType(context, "Bool");
    data->type = type;
    return true;
}

static bool stringLiteral(
    Context* context, Code* code, uint32_t id, AstString* data, Arena scratch
) {
    todo();
}

static bool arrayLiteral(
    Context* context, Code* code, uint32_t id, AstArray* data, Arena scratch
) {
    todo();
}

static bool structLiteral(
    Context* context,
    Code* code,
    uint32_t id,
    AstStructLiteral* data,
    Arena scratch
) {
    todo();
}

static bool analyzeInstruction(
    Context* context, Code* code, uint32_t id, Arena scratch
) {
    AstInstructionKind kind = ast_getInstruction(*code, id);
    void* data = ast_getData(code, id);
    switch (kind) {
    case AST_DECLARATION: return declaration(context, code, id, data, scratch);
    case AST_ASSIGNMENT: return assignemnt(context, code, id, data, scratch);
    case AST_RETURN: return returnStmt(context, code, id, data, scratch);
    case AST_RETURN_EXPRESSION:
        return returnExpression(context, code, id, data, scratch);
    case AST_BREAK: return breakStmt(context, code, id, data, scratch);
    case AST_CONTINUE: return continueStmt(context, code, id, data, scratch);
    case AST_IF_ELSE: return ifElse(context, code, id, data, scratch);
    case AST_WHILE: return whileStmt(context, code, id, data, scratch);
    case AST_CALL: return call(context, code, id, data, scratch);
    case AST_IF_ELSE_EXPRESSION:
        ifElseExpression(context, code, id, data, scratch);
    case AST_NOT: return not(context, code, id, data, scratch);
    case AST_OR: return orExpr(context, code, id, data, scratch);
    case AST_AND: return andExpr(context, code, id, data, scratch);
    case AST_EQUAL_EQUAL: return equalEqual(context, code, id, data, scratch);
    case AST_NOT_EQUAL: return notEqual(context, code, id, data, scratch);
    case AST_LESS: return less(context, code, id, data, scratch);
    case AST_LESS_EQUAL: return lessEqual(context, code, id, data, scratch);
    case AST_GREATER: return greater(context, code, id, data, scratch);
    case AST_GREATER_EQUAL:
        return greaterEqual(context, code, id, data, scratch);
    case AST_ADD: return add(context, code, id, data, scratch);
    case AST_SUBTRACT: return subtract(context, code, id, data, scratch);
    case AST_MUL: return mul(context, code, id, data, scratch);
    case AST_DIV: return divExpr(context, code, id, data, scratch);
    case AST_MOD: return mod(context, code, id, data, scratch);
    case AST_NEGATION: return negation(context, code, id, data, scratch);
    case AST_ADDRESS_OF: return addressOf(context, code, id, data, scratch);
    case AST_DEREFERENCE: return dereference(context, code, id, data, scratch);
    case AST_FIELD_ACCESS: return fieldAccess(context, code, id, data, scratch);
    case AST_INDEX_ACCESS: return indexAccess(context, code, id, data, scratch);
    case AST_FLOAT: return floatLiteral(context, code, id, data, scratch);
    case AST_INTEGER: return integerLiteral(context, code, id, data, scratch);
    case AST_IDENTIFIER: return identifier(context, code, id, data, scratch);
    case AST_BOOLEAN: return booleanLiteral(context, code, id, data, scratch);
    case AST_STRING: return stringLiteral(context, code, id, data, scratch);
    case AST_ARRAY: return arrayLiteral(context, code, id, data, scratch);
    case AST_STRUCT_LITERAL:
        return structLiteral(context, code, id, data, scratch);
    default: unreachable();
    }
}

static bool analyzeTypeDefinition(
    Context* context, TypeDefinition* typeDefinition, Arena scratch
) {
    todo();
}

static bool analyzeFunction(
    Context* context, Function* function, Arena scratch
) {
    todo();
}

static bool analyzeCode(Context* context, Code* code, Arena scratch) {
    for (uint32_t id = 1; id <= list_size(code->instructions); id++) {
        bool result = analyzeInstruction(context, code, id, scratch);
        if (!result) return false;
    }
    return true;
}

bool analyze(
    Program program, uint32_t astId, Arena scratch, Arena otherScratch
) {
    // Initialize context
    Context context = {0};
    context.arena = scratch;
    context.ast = list_get(program.asts, astId);
    scopes_addScope(&context.arena, &context.scopes);

    // Add builtin types
    importModuleUnqualified(&context, program.builtin, None());

    // Analyze types
    for (uint32_t i = 0; i < list_size(context.ast->typeDefinitions); i++) {
        analyzeTypeDefinition(
            &context,
            list_get(context.ast->typeDefinitions, i),
            otherScratch
        );
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        analyzeFunction(
            &context,
            list_get(context.ast->functions, i),
            otherScratch
        );
    }

    // Analyze code
    analyzeCode(&context, &context.ast->code, otherScratch);
    return true;
}