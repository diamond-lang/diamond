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
    Ast* builtin;
    Ast* ast;
    Bindings bindings;
    Uint32Stack stack;
    uint32_t lastTypeVariable;
    Arena arena;
    Function* functionBeingAnalyzed;
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
        newType = ast_addTypeVariable(context->ast, data.id);
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams data = type->withParams;
        newType = ast_addTypeWithParams(
            context->ast,
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
        TypeBinding* binding =
            scopes_getTypeBinding(context->bindings, identifier);
        if (binding != NULL) {
            todo();
        }
        scopes_addTypeBinding(
            &context->arena,
            &context->bindings,
            identifier,
            astId
        );
    }

    // Add function definitions
    for (uint32_t i = 0; i < list_size(ast.functions); i++) {
        Function function = *list_get(ast.functions, i);
        if (function.type != None()) {
            uint32_t identifier = ast_getLiteral(
                context->ast,
                ast_literalAsString(ast, function.identifier)
            );
            Binding* binding =
                scopes_getBindingInCurrentScope(context->bindings, identifier);
            if (binding != NULL) {
                todo();
            }
            uint32_t functionType = typeInContext(context, ast, function.type);
            scopes_addFunctionBinding(
                &context->arena,
                &context->bindings,
                identifier,
                functionType,
                astId
            );
        }
    }
}

static void init_context(
    Context* context, Arena contextArena, Ast* ast, Ast* builtin
) {
    context->arena = contextArena;
    context->ast = ast;
    context->builtin = builtin;
    scopes_addScope(&context->arena, &context->bindings);
    importModuleUnqualified(context, *builtin, None());
}

static uint32_t newTypeVariable(Context* context) {
    return context->lastTypeVariable++;
}

static uint32_t getBuiltInType(Context* context, char* type) {
    uint32_t literal = ast_getLiteral(context->ast, type);
    TypeBinding* binding = scopes_getTypeBinding(context->bindings, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newType = ast_addTypeWithParams(context->ast, literal, 0);
    return newType;
}

static uint32_t _instantiateType(
    Context* context, uint32_t typeId, Uint32Hashmap* mappings, Arena* arena
) {
    Type* type = ast_getType(context->ast->types, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        return typeId;
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
                    ast_addTypeVariable(context->ast, newTypeVariable(context))
                );
            }
            uint32_t mappedType = *hashmap_get(*mappings, data.literal);
            return mappedType;
        } else {
            // Create new type
            uint32_t newId = ast_addTypeWithParams(
                context->ast,
                data.literal,
                data.parameterCount
            );
            TypeWithParams newData =
                ast_getType(context->ast->types, newId)->withParams;

            // Instantiate parameters
            uint32_t* params = arena_getPointer(
                context->ast->arena,
                uint32_t,
                data.parameters
            );
            uint32_t* newParams = arena_getPointer(
                context->ast->arena,
                uint32_t,
                newData.parameters
            );
            for (uint32_t i = 0; i < data.parameterCount; i++) {
                uint32_t parameter = params[i];
                newParams[i] =
                    _instantiateType(context, parameter, mappings, arena);
            }
            return newId;
        }
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
    String str1 = ast_typeAsString(&scratch, *context->ast, type, typeArena);
    String str2 =
        ast_typeAsString(&scratch, *context->ast, otherType, otherTypeArena);
    printf("%s = %s\n", str1.buffer, str2.buffer);
    printf("[");
    for (uint32_t i = 1; i <= list_size(context->ast->types); i++) {
        ast_printType(*context->ast, i, scratch);
        if (i != list_size(context->ast->types)) {
            printf(", ");
        }
    }
    printf("]\n");
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
                Type* param = ast_getType(context->ast->types, params[i]);
                Type* otherParam =
                    ast_getType(context->ast->types, otherParams[i]);
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
        } break;
        }
    }
    }
    printf("[");
    for (uint32_t i = 1; i <= list_size(context->ast->types); i++) {
        ast_printType(*context->ast, i, scratch);
        if (i != list_size(context->ast->types)) {
            printf(", ");
        }
    }
    printf("]\n");
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

static bool returnLastExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstReturnLastExpression* data,
    Arena scratch
) {
    // Check we are on a function and get it's type
    if (!context->functionBeingAnalyzed) {
        todo();  // Not inside function
    }
    Type* returnType = ast_getType(
        context->ast->types,
        context->functionBeingAnalyzed->returnType
    );

    // Get type of expression
    uint32_t expr = *stack_top(context->stack);
    uint32_t exprTypeId = ast_getTypeOfInstruction(*code, expr);
    Type* exprType = ast_getType(context->ast->types, exprTypeId);

    // Make return type equal to type of expression
    makeEqual(
        context,
        returnType,
        context->ast->arena,
        exprType,
        context->ast->arena,
        scratch
    );

    return true;
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
    data->type = ast_addTypeVariable(context->ast, newTypeVariable(context));

    // Get expected type from expression called
    uint32_t called = *stack_get(
        context->stack,
        stack_size(context->stack) - 1 - data->argumentsCount
    );
    uint32_t expectedId = ast_getTypeOfInstruction(*code, called);
    Type* expected = ast_getType(context->ast->types, expectedId);

    // Get actual type so far
    Type* actualType = ast_createTemporaryTypeWithParams(
        &scratch,
        ast_getLiteral(context->ast, "->"),
        data->argumentsCount + 1
    );
    uint32_t* params =
        arena_getPointer(scratch, uint32_t, actualType->withParams.parameters);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount + i
        );
        uint32_t argTypeId = ast_getTypeOfInstruction(*code, arg);
        params[i] = argTypeId;
    }
    params[data->argumentsCount] = data->type;

    // Make type of called expression equal to expected type
    makeEqual(
        context,
        actualType,
        scratch,
        expected,
        context->ast->arena,
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
    uint32_t resultTypeId =
        ast_addTypeVariable(context->ast, newTypeVariable(context));
    uint32_t operatorTypeId = ast_addFunctionType(context->ast, 2);
    Type* operatorType = ast_getType(context->ast->types, operatorTypeId);
    uint32_t* params = arena_getPointer(
        context->ast->arena,
        uint32_t,
        operatorType->withParams.parameters
    );
    for (uint32_t i = 0; i < 3; i++) {
        params[i] = resultTypeId;
    }

    // Construct expected type
    data->type = ast_addTypeVariable(context->ast, newTypeVariable(context));
    Type* expected = ast_createTemporaryTypeWithParams(
        &scratch,
        ast_getLiteral(context->ast, "->"),
        3
    );
    uint32_t* paramsExpected =
        arena_getPointer(scratch, uint32_t, expected->withParams.parameters);
    for (uint32_t i = 0; i < 2; i++) {
        uint32_t arg =
            *stack_get(context->stack, stack_size(context->stack) - 2);
        uint32_t argTypeId = ast_getTypeOfInstruction(*code, arg);
        paramsExpected[i] = argTypeId;
    }
    paramsExpected[2] = data->type;

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
    Binding* binding = scopes_getBinding(context->bindings, data->literal);
    if (binding == NULL) {
        todo();
    }
    data->type = instantiateType(context, binding->type, scratch);
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
    case AST_RETURN_LAST_EXPRESSION:
        return returnLastExpression(context, code, id, data, scratch);
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

static bool analyzeCode(Context* context, Code* code, Arena scratch);

static bool analyzeFunctionNotCompletelyTyped(
    Context* context, Function* function, Arena scratch
) {
    if (function->beingAnalyzed) return true;
    function->beingAnalyzed = true;

    // Create new context
    Context newContext = {0};
    init_context(&newContext, context->arena, context->ast, context->builtin);
    newContext.functionBeingAnalyzed = function;

    // For every argument
    for (uint32_t i = 0; i < list_size(function->arguments); i++) {
        FunctionArgument* argument = list_get(function->arguments, i);
        if (argument->type == None()) {
            argument->type = ast_addTypeVariable(
                newContext.ast,
                newTypeVariable(&newContext)
            );
        } else {
            todo();  // Analyze type if it has
        }
        scopes_addArgumentBinding(
            &newContext.arena,
            &newContext.bindings,
            argument->identifier,
            argument->type
        );
    }

    // For return type
    if (function->returnType == None()) {
        function->returnType =
            ast_addTypeVariable(newContext.ast, newTypeVariable(&newContext));
    } else {
        todo();  // Analyze type if it has
    }

    // Analyze function
    bool result = analyzeCode(&newContext, &function->code, scratch);
    if (!result) {
        todo();
        return false;
    }

    // Add function type
    uint32_t argsCount = list_size(function->arguments);
    function->type = ast_addFunctionType(newContext.ast, argsCount);
    TypeWithParams* type =
        &ast_getType(newContext.ast->types, function->type)->withParams;
    uint32_t* params =
        arena_getPointer(newContext.ast->arena, uint32_t, type->parameters);
    for (uint32_t i = 0; i < argsCount; i++) {
        params[i] = list_get(function->arguments, i)->type;
    }
    params[argsCount] = function->returnType;

    // Return
    function->beingAnalyzed = false;
    return true;
}

static bool analyzeCode(Context* context, Code* code, Arena scratch) {
    for (uint32_t id = 1; id <= list_size(code->instructions); id++) {
        bool result = analyzeInstruction(context, code, id, scratch);
        if (!result) return false;
    }
    return true;
}

bool analyze(Program program, uint32_t astId, Arena scratch1, Arena scratch2) {
    // Initialize context
    Context context = {0};
    init_context(
        &context,
        scratch1,
        list_get(program.asts, astId),
        &program.builtin
    );

    // Analyze types
    for (uint32_t i = 0; i < list_size(context.ast->typeDefinitions); i++) {
        analyzeTypeDefinition(
            &context,
            list_get(context.ast->typeDefinitions, i),
            scratch2
        );
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        Function* function = list_get(context.ast->functions, i);
        if (function->type != None()) {
            analyzeFunction(&context, function, scratch2);
        } else {
            analyzeFunctionNotCompletelyTyped(&context, function, scratch2);
        }
    }

    // Analyze code
    analyzeCode(&context, &context.ast->code, scratch2);
    return true;
}