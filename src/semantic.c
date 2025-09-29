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
#include "utilities.h"

typedef struct {
    Ast* builtin;
    Ast* ast;
    uint32_t astId;
    Scopes scopes;
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
    Type* type = ast_getType(ast, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        TypeVariable data = type->variable;
        newType = ast_addTypeVariable(context->ast, data.id);
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams* data = &type->withParams;
        newType = ast_addTypeWithParams(
            context->ast,
            literalInContext(context, ast, data->literal),
            data->parameterCount
        );
        TypeWithParams* newData =
            &ast_getType(*context->ast, newType)->withParams;
        for (uint32_t i = 0; i < data->parameterCount; i++) {
            newData->parameters[i] =
                typeInContext(context, ast, data->parameters[i]);
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
            scopes_getTypeBinding(context->scopes, identifier);
        if (binding != NULL) {
            todo();
        }
        scopes_addTypeBinding(
            &context->arena,
            &context->scopes,
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
                scopes_getBindingInCurrentScope(context->scopes, identifier);
            if (binding != NULL) {
                todo();
            }
            uint32_t functionType = typeInContext(context, ast, function.type);
            scopes_addFunctionBinding(
                &context->arena,
                &context->scopes,
                identifier,
                i,
                functionType,
                astId
            );
        }
    }
}

static void init_context(
    Context* context, Arena contextArena, Ast* ast, uint32_t astId, Ast* builtin
) {
    context->arena = contextArena;
    context->ast = ast;
    context->astId = astId;
    context->builtin = builtin;
    scopes_addScope(&context->arena, &context->scopes);
    importModuleUnqualified(context, *builtin, None());
}

static uint32_t newTypeVariable(Context* context) {
    return context->lastTypeVariable++;
}

static uint32_t getBuiltInType(Context* context, char* type) {
    uint32_t literal = ast_getLiteral(context->ast, type);
    TypeBinding* binding = scopes_getTypeBinding(context->scopes, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newType = ast_addTypeWithParams(context->ast, literal, 0);
    return newType;
}

static uint32_t _instantiateType(
    Context* context, uint32_t typeId, Uint32Hashmap* mappings, Arena* arena
) {
    Type* type = ast_findType(context->ast, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        return typeId;
    }
    case TYPE_WITH_PARAMS: {
        // Get type with params
        TypeWithParams* data = &type->withParams;

        // If is type variable, eg: t, a, b
        bool isTypeVariable = ast_isTypeVariable(*context->ast, data);
        if (isTypeVariable) {
            // Instantiate type variable
            assert(data->parameterCount == 0);
            if (hashmap_get(*mappings, data->literal) == NULL) {
                hashmap_set(
                    arena,
                    *mappings,
                    data->literal,
                    ast_addTypeVariable(context->ast, newTypeVariable(context))
                );
            }
            uint32_t mappedType = *hashmap_get(*mappings, data->literal);
            return mappedType;
        } else {
            // Create new type
            uint32_t newId = ast_addTypeWithParams(
                context->ast,
                data->literal,
                data->parameterCount
            );
            TypeWithParams* newData =
                &ast_getType(*context->ast, newId)->withParams;

            // Instantiate parameters
            for (uint32_t i = 0; i < data->parameterCount; i++) {
                uint32_t parameter = data->parameters[i];
                newData->parameters[i] =
                    _instantiateType(context, parameter, mappings, arena);
            }
            return newId;
        }
    }
    }
    return None();
}

static uint32_t instantiateType(Context* context, uint32_t id, Arena scratch) {
    if (id == 0) return 0;
    Uint32Hashmap mappings = {0};
    uint32_t result = _instantiateType(context, id, &mappings, &scratch);
    return result;
}

static bool contains(Context* context, Type* a, TypeVariable* b) {
    if (a->kind == TYPE_VARIABLE) {
        return a->variable.id == b->id;
    } else if (a->kind == TYPE_WITH_PARAMS) {
        for (uint32_t i = 0; i < a->withParams.parameterCount; i++) {
            Type* param =
                ast_getType(*context->ast, a->withParams.parameters[i]);
            if (contains(context, param, b)) {
                return true;
            }
        }
    }
    return false;
}

static void makeEqual(Context* context, uint32_t aId, uint32_t bId) {
    Type* a = ast_findType(context->ast, aId);
    Type* b = ast_findType(context->ast, bId);
    if (a == b) return;

    if (a->kind == TYPE_VARIABLE) {
        if (contains(context, b, &a->variable)) {
            todo();  // Infinite type detected
        }
        ast_makeEqual(&a->variable, bId);
        return;
    }

    if (b->kind == TYPE_VARIABLE) {
        return makeEqual(context, bId, aId);
    }

    if (a->withParams.literal != b->withParams.literal) {
        todo();  // Couldn't unify types
    }

    if (a->withParams.parameterCount != b->withParams.parameterCount) {
        todo();  // Couldn't unify types
    }

    for (uint32_t i = 0; i < a->withParams.parameterCount; i++) {
        uint32_t paramA = a->withParams.parameters[i];
        uint32_t paramB = b->withParams.parameters[i];
        makeEqual(context, paramA, paramB);
    }
}

static bool analyzeType(Context* context, uint32_t typeId, Arena scratch) {
    Type* type = ast_getType(*context->ast, typeId);
    assert(type->kind == TYPE_WITH_PARAMS);
    TypeBinding* binding =
        scopes_getTypeBinding(context->scopes, type->withParams.literal);
    assert(binding);
    assert(binding->module == None());
    return true;
}

static bool declaration(
    Context* context,
    Code* code,
    uint32_t id,
    AstDeclaration* data,
    Arena scratch
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
    uint32_t returnType = context->functionBeingAnalyzed->returnType;

    // Get type of expression
    uint32_t expr = *stack_top(context->stack);
    uint32_t exprType = ast_getTypeOfInstruction(*code, expr);

    // Make return type equal to type of expression
    makeEqual(context, returnType, exprType);

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
    uint32_t expected = ast_getTypeOfInstruction(*code, called);

    // Get actual type so far
    uint32_t actualTypeId =
        ast_addFunctionType(context->ast, data->argumentsCount);
    Type* actualType = ast_getType(*context->ast, actualTypeId);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount + i
        );
        uint32_t argTypeId = ast_getTypeOfInstruction(*code, arg);
        actualType->withParams.parameters[i] = argTypeId;
    }
    actualType->withParams.parameters[data->argumentsCount] = data->type;

    // Make type of called expression equal to expected type
    if (expected != None()) {
        makeEqual(context, actualTypeId, expected);
    }
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
    todo();
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

static bool analyzeFunctionNotCompletelyTyped(
    Context* context, Function* function, Arena scratch
);

static bool identifier(
    Context* context,
    Code* code,
    uint32_t id,
    AstIdentifier* data,
    Arena scratch
) {
    Binding* binding = scopes_getBinding(context->scopes, data->literal);
    if (binding == NULL) {
        todo();
    } else if (binding->kind == FUNCTION_BINDING && binding->type == None()) {
        assert(binding->module == context->astId);
        Function* function = list_get(context->ast->functions, binding->id);
        if (!function->beingAnalyzed) {
            analyzeFunctionNotCompletelyTyped(context, function, scratch);
            binding->type = function->type;
        }
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

static bool analyzeCode(Context* context, Code* code, Arena scratch);

static bool analyzeFunction(
    Context* context, Function* function, Arena scratch
) {
    assert(!function->beingAnalyzed);
    function->beingAnalyzed = true;

    // Create new context
    Context newContext = *context;
    newContext.functionBeingAnalyzed = function;

    // Reset scopes
    newContext.scopes.bindings = (BindingStack){0};
    newContext.scopes.scopeStart = (Uint32Stack){0};
    scopes_addScope(&newContext.arena, &newContext.scopes);
    for (uint32_t i = 0; i < stack_size(context->scopes.bindings); i++) {
        Binding binding = *stack_get(context->scopes.bindings, i);
        stack_push(&newContext.arena, newContext.scopes.bindings, binding);
        if (stack_size(context->scopes.scopeStart) >= 2 &&
            i == *stack_get(context->scopes.scopeStart, 1)) {
            break;
        }
    }

    // For every argument
    for (uint32_t i = 0; i < list_size(function->arguments); i++) {
        FunctionArgument* argument = list_get(function->arguments, i);
        analyzeType(context, argument->type, scratch);
        scopes_addArgumentBinding(
            &newContext.arena,
            &newContext.scopes,
            argument->identifier,
            argument->type
        );
    }

    // For return type
    analyzeType(context, function->returnType, scratch);

    // Analyze function
    bool result = analyzeCode(&newContext, &function->code, scratch);
    if (!result) {
        todo();  // print errors
        return false;
    }

    // Return
    function->beingAnalyzed = false;
    return true;
}

static uint32_t addNewParameter(
    Ast* ast, uint32_t* id, Uint32List* parameters, Arena scratch
) {
    String result;
    uint32_t newParam;
    char letters[] = "abcdefghijklmnopqrstuvwxyz";
    while (true) {
        result = (String){0};
        string_append(&scratch, &result, letters[(*id + 19) % 26]);
        if (*id > 25) {
            String num = numberAsString(&scratch, 1 + *id / 26);
            string_concat(&scratch, &result, string_asView(num));
        }
        newParam = ast_getLiteral(ast, result.buffer);

        bool alreadyUsed = false;
        for (uint32_t i = 0; i < list_size(*parameters); i++) {
            uint32_t param = *list_get(*parameters, i);
            if (param == newParam) {
                alreadyUsed = true;
                break;
            }
        }
        if (!alreadyUsed) break;
        *id = *id + 1;
    }
    list_append(&ast->arena, *parameters, newParam);
    uint32_t newType = ast_addTypeWithParams(ast, newParam, 0);
    return newType;
}

static void generalizeType(
    Context* context,
    uint32_t typeId,
    uint32_t* newParametersCount,
    Uint32List* parameters,
    Arena scratch
) {
    Type* type = ast_findType(context->ast, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: {
        uint32_t newParam = addNewParameter(
            context->ast,
            newParametersCount,
            parameters,
            scratch
        );
        ast_makeEqual(&type->variable, newParam);
        break;
    }
    case TYPE_WITH_PARAMS: {
        for (uint32_t i = 0; i < type->withParams.parameterCount; i++) {
            uint32_t param = type->withParams.parameters[i];
            generalizeType(
                context,
                param,
                newParametersCount,
                parameters,
                scratch
            );
        }
    }
    }
}

static bool analyzeFunctionNotCompletelyTyped(
    Context* context, Function* function, Arena scratch
) {
    assert(!function->beingAnalyzed);
    function->beingAnalyzed = true;

    // Create new context
    Context newContext = *context;
    newContext.functionBeingAnalyzed = function;

    // Reset scopes
    newContext.scopes.bindings = (BindingStack){0};
    newContext.scopes.scopeStart = (Uint32Stack){0};
    scopes_addScope(&newContext.arena, &newContext.scopes);
    for (uint32_t i = 0; i < stack_size(context->scopes.bindings); i++) {
        Binding binding = *stack_get(context->scopes.bindings, i);
        stack_push(&newContext.arena, newContext.scopes.bindings, binding);
        if (stack_size(context->scopes.scopeStart) >= 2 &&
            i == *stack_get(context->scopes.scopeStart, 1)) {
            break;
        }
    }

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
            &newContext.scopes,
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
        todo();  // print errors
        return false;
    }

    // Generalize types
    Uint32List* params = &function->parameters;
    uint32_t count = 0;
    uint32_t argsCount = list_size(function->arguments);
    for (uint32_t i = 0; i < argsCount; i++) {
        uint32_t argType = list_get(function->arguments, i)->type;
        generalizeType(context, argType, &count, params, scratch);
    }
    generalizeType(context, function->returnType, &count, params, scratch);

    // Create function type
    function->type = ast_addFunctionType(newContext.ast, argsCount);
    TypeWithParams* type =
        &ast_getType(*newContext.ast, function->type)->withParams;
    for (uint32_t i = 0; i < argsCount; i++) {
        uint32_t argType = list_get(function->arguments, i)->type;
        type->parameters[i] = argType;
    }
    type->parameters[argsCount] = function->returnType;

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
        astId,
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

    // Add function binding
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        Function* function = list_get(context.ast->functions, i);

        // Add function binding
        Binding* binding = scopes_getBindingInCurrentScope(
            context.scopes,
            function->identifier
        );
        if (binding != NULL) {
            todo();
        }
        scopes_addFunctionBinding(
            &context.arena,
            &context.scopes,
            function->identifier,
            function->type,
            i,
            astId
        );
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        Function* function = list_get(context.ast->functions, i);
        if (function->type != None()) {
            analyzeFunction(&context, function, scratch2);
        } else {
            analyzeFunctionNotCompletelyTyped(&context, function, scratch2);
            Binding* binding =
                scopes_getBinding(context.scopes, function->identifier);
            assert(binding);
            assert(binding->kind == FUNCTION_BINDING);
            binding->type = function->type;
        }
    }

    // Analyze code
    analyzeCode(&context, &context.ast->code, scratch2);
    return true;
}