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

static uint32_t typeInContext(Context* context, Ast ast, uint32_t type) {
    uint32_t newType = None();
    TypeKind kind = ast_getTypeKind(ast.types, type);
    switch (kind) {
    case TYPE_VARIABLE: {
        TypeVariable data = *ast_getTypeVariable(ast.types, type);
        newType = ast_addTypeVariable(
            &context->ast->arena,
            &context->ast->types,
            data.id
        );
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams data = *ast_getTypeWithParams(ast.types, type);
        newType = ast_addTypeWithParams(
            &context->ast->arena,
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
            &context->ast->arena,
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
    Context* context, uint32_t id, Uint32Hashmap* mappings, Arena* arena
) {
    TypeKind kind = ast_getTypeKind(context->ast->types, id);
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
                    arena,
                    *mappings,
                    type.literal,
                    ast_addTypeVariable(
                        &context->ast->arena,
                        &context->ast->types,
                        newTypeVariable(context)
                    )
                );
            }
            return *hashmap_get(*mappings, type.literal);
        } else {
            uint32_t firstParameter = list_size(context->ast->types.types) + 1;

            // Instantiate parameters
            for (uint32_t i = 0; i < type.parameterCount; i++) {
                uint32_t parameter =
                    ast_getParameters(context->ast->types, id)[i];
                _instantiateType(context, parameter, mappings, arena);
            }
            // Create new type
            uint32_t newId = ast_addTypeWithParams(
                &context->ast->arena,
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
        uint32_t firstParameter = list_size(context->ast->types.types) + 1;

        // Instantiate arguments
        for (uint32_t i = 0; i < type.parameterCount; i++) {
            uint32_t parameter = ast_getParameters(context->ast->types, id)[i];
            _instantiateType(context, parameter, mappings, arena);
        }

        // Create new type
        uint32_t newReturnType =
            _instantiateType(context, type.returnType, mappings, arena);
        uint32_t newId = ast_addFunctionType(
            &context->ast->arena,
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

static uint32_t instantiateType(Context* context, uint32_t id, Arena scratch) {
    Uint32Hashmap mappings = {0};
    uint32_t result = _instantiateType(context, id, &mappings, &scratch);
    return result;
}

static void makeEqual(
    Context* context,
    uint32_t typeId,
    Types temporary,
    uint32_t expected,
    Arena scratch
) {
    TypeKind kind = ast_getTypeKind(context->ast->types, typeId);
    TypeKind kindExpected = ast_getTypeKind(temporary, expected);
    switch (kind) {
    case TYPE_VARIABLE: {
        switch (kindExpected) {
        case TYPE_VARIABLE: unreachable();
        case TYPE_WITH_PARAMS: {
            *ast_getType(context->ast->types, typeId) =
                *ast_getType(temporary, expected);
            uint32_t parameterCount =
                ast_getParametersCount(temporary, expected);
            uint32_t* params = ast_getParameters(temporary, expected);
            ast_getTypeWithParams(context->ast->types, typeId)->firstParameter =
                list_size(context->ast->types.parameters);
            for (uint32_t i = 0; i < parameterCount; i++) {
                list_append(
                    &context->ast->arena,
                    context->ast->types.parameters,
                    params[i]
                );
            }
            break;
        }
        case FUNCTION_TYPE: {
            *ast_getType(context->ast->types, typeId) =
                *ast_getType(temporary, expected);
            uint32_t parameterCount =
                ast_getParametersCount(temporary, expected);
            uint32_t* params = ast_getParameters(temporary, expected);
            ast_getFunctionType(context->ast->types, typeId)->firstParameter =
                list_size(context->ast->types.parameters);
            for (uint32_t i = 0; i < parameterCount; i++) {
                list_append(
                    &context->ast->arena,
                    context->ast->types.parameters,
                    params[i]
                );
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
            TypeWithParams* type =
                ast_getTypeWithParams(context->ast->types, typeId);
            TypeWithParams* other = ast_getTypeWithParams(temporary, expected);
            if (type->literal != other->literal) {
                todo();
            }
            if (type->parameterCount != other->parameterCount) {
                todo();
            }
            uint32_t* params = ast_getParameters(context->ast->types, typeId);
            uint32_t* otherParams = ast_getParameters(temporary, expected);
            for (uint32_t i = 0; i < type->parameterCount; i++) {
                uint32_t param = params[i];
                uint32_t otherParam = otherParams[i];
                if (ast_getTypeKind(context->ast->types, param) ==
                    TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        param,
                        context->ast->types,
                        otherParam,
                        scratch
                    );
                } else if (ast_getTypeKind(temporary, otherParam) ==
                           TYPE_VARIABLE) {
                    unreachable();
                } else {
                    makeEqual(
                        context,
                        param,
                        context->ast->types,
                        otherParam,
                        scratch
                    );
                }
            }
            break;
        }
        case FUNCTION_TYPE: todo();
        }
    }
    case FUNCTION_TYPE: {
        switch (kindExpected) {
        case TYPE_VARIABLE: unreachable();
        case TYPE_WITH_PARAMS: todo();
        case FUNCTION_TYPE: {
            FunctionType* type =
                ast_getFunctionType(context->ast->types, typeId);
            FunctionType* other = ast_getFunctionType(temporary, expected);
            if (type->parameterCount != other->parameterCount) {
                todo();
            }
            uint32_t* params = ast_getParameters(context->ast->types, typeId);
            uint32_t* otherParams = ast_getParameters(temporary, expected);
            for (uint32_t i = 0; i < type->parameterCount; i++) {
                uint32_t param = params[i];
                uint32_t otherParam = otherParams[i];
                if (ast_getTypeKind(context->ast->types, param) ==
                    TYPE_VARIABLE) {
                    makeEqual(
                        context,
                        param,
                        context->ast->types,
                        otherParam,
                        scratch
                    );
                } else if (ast_getTypeKind(temporary, otherParam) ==
                           TYPE_VARIABLE) {
                    unreachable();
                } else {
                    makeEqual(
                        context,
                        param,
                        context->ast->types,
                        otherParam,
                        scratch
                    );
                }
            }
            break;
        }
        }
    }
    }
}

static bool analyzeTypeDefinition(
    Context* context, TypeDefinition* typeDefinition, Arena scratch
);
static bool analyzeFunction(
    Context* context, Function* function, Arena scratch
);
static bool analyzeCode(Context* context, Code* code, Arena scratch);
static bool analyzeInstruction(
    Context* context, Code* code, uint32_t id, Arena scratch
);

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

    // Get expression called and it's type
    uint32_t called = *stack_get(
        context->stack,
        stack_size(context->stack) - 1 - data->argumentsCount
    );
    uint32_t calledType = ast_getTypeOfInstruction(*context->ast, called);

    // Construct expected type
    Types temporary = {0};
    uint32_t expected = ast_addFunctionType(
        &scratch,
        &temporary,
        data->type,
        data->argumentsCount
    );
    uint32_t* parameters = ast_getParameters(temporary, expected);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount
        );
        parameters[i] = ast_getTypeOfInstruction(*context->ast, arg);
    }

    // Make type of called expression equal to expected type
    makeEqual(context, calledType, temporary, expected, scratch);
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
    stack_pop(context->stack);
    stack_pop(context->stack);
    stack_push(&context->arena, context->stack, id);
    data->type = getBuiltInType(context, "Float64");
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