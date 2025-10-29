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
#include "types.h"
#include "utilities.h"

typedef enum {
    FUNCTION_BINDING,
    INTERFACE_BINDING,
    VARIABLE_BINDING,
    ARGUMENT_BINDING
} BindingKind;

typedef struct {
    uint32_t id;
    uint32_t module;
} FunctionBinding;

typedef struct {
    uint32_t id;
    uint32_t module;
} InterfaceBinding;

typedef struct {
    uint32_t id;
} VariableBinding;

typedef struct {
    uint32_t id;
} ArgumentBinding;

typedef struct {
    BindingKind kind;
    uint32_t literal;
    union {
        FunctionBinding asFunction;
        InterfaceBinding asInterface;
        VariableBinding asVariable;
        ArgumentBinding asArgument;
    };
} Binding;

typedef StackType(Binding) BindingStack;

typedef struct {
    uint32_t literal;
    uint32_t module;
} TypeBinding;

typedef ListType(TypeBinding) TypeBindingList;

typedef struct {
    BindingStack bindings;
    Uint32Stack scopeStart;
    TypeBindingList types;
} Scopes;

typedef struct {
    Ast* ast;
    uint32_t astId;
    Scopes scopes;
    Uint32Stack stack;
    uint32_t lastTypeVariable;
    Arena arena;
    Function* functionBeingAnalyzed;
} Context;

static void addScope(Context* context) {
    stack_push(
        &context->arena,
        context->scopes.scopeStart,
        stack_size(context->scopes.bindings)
    );
}

static void removeScope(Context* context) {
    context->scopes.bindings.count = *stack_top(context->scopes.scopeStart);
    stack_pop(context->scopes.scopeStart);
}

static Binding* getBinding(Context* context, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(context->scopes.bindings);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(context->scopes.bindings, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

static Binding* getBindingInCurrentScope(Context* context, uint32_t literalId) {
    uint32_t start = *stack_top(context->scopes.scopeStart);
    uint32_t bindingsCount = stack_size(context->scopes.bindings);
    assert(bindingsCount >= start);
    for (uint32_t i = bindingsCount - 1; start <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(context->scopes.bindings, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

static void addArgumentBinding(
    Context* context, uint32_t literal, uint32_t id
) {
    Binding binding = {
        .kind = ARGUMENT_BINDING,
        .literal = literal,
        .asArgument = (ArgumentBinding){.id = id}
    };
    stack_push(&context->arena, context->scopes.bindings, binding);
}

// static void scopes_addVariableBinding(
//     Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
// ) {
//     Binding binding = {
//         .kind = VARIABLE_BINDING,
//         .identifier = literalId,
//         .type = type
//     };
//     stack_push(arena, scopes->bindings, binding);
// }

static void addFunctionBinding(
    Context* context, uint32_t literal, uint32_t id, uint32_t module
) {
    Binding binding = {
        .kind = FUNCTION_BINDING,
        .literal = literal,
        .asFunction = (FunctionBinding){.id = id, .module = module}
    };
    stack_push(&context->arena, context->scopes.bindings, binding);
}

static void addInterfaceBinding(
    Context* context, uint32_t literalId, uint32_t id, uint32_t module
) {
    Binding binding = {
        .kind = INTERFACE_BINDING,
        .literal = literalId,
        .asInterface = (InterfaceBinding){.id = id, .module = module}
    };
    stack_push(&context->arena, context->scopes.bindings, binding);
}

static void addTypeBinding(
    Context* context, uint32_t literal, uint32_t module
) {
    TypeBinding binding = {.literal = literal, .module = module};
    stack_push(&context->arena, context->scopes.types, binding);
}

static TypeBinding* getTypeBinding(Context* context, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(context->scopes.types);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        TypeBinding* binding = stack_get(context->scopes.types, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

static uint32_t getBindingType(Context* context, Binding binding) {
    switch (binding.kind) {
    case FUNCTION_BINDING: {
        FunctionBinding* b = &binding.asFunction;
        if (b->module == context->astId) {
            Function* function = list_get(context->ast->functions, b->id);
            return function->type;
        } else {
            ImportedFunction* function =
                list_get(context->ast->importedFunctions, b->id);
            return function->type;
        }
        break;
    }
    case INTERFACE_BINDING: {
        todo();
    }
    case VARIABLE_BINDING: {
        todo();
    }
    case ARGUMENT_BINDING: {
        todo();
    }
    }
}

static bool addTopLevelDefinitionsBindings(
    Context* context, uint32_t currentAstId
) {
    // Add types bindings
    for (uint32_t i = 0; i < list_size(context->ast->typeDefinitions); i++) {
        TypeDefinition typeDef = *list_get(context->ast->typeDefinitions, i);
        TypeBinding* binding = getTypeBinding(context, typeDef.identifier);
        if (binding != NULL) {
            todo();
        }
        addTypeBinding(context, typeDef.identifier, currentAstId);
    }

    for (uint32_t i = 0; i < list_size(context->ast->importedTypeDefinitions);
         i++) {
        ImportedTypeDefinition typeDef =
            *list_get(context->ast->importedTypeDefinitions, i);
        TypeBinding* binding = getTypeBinding(context, typeDef.identifier);
        if (binding != NULL) {
            todo();
        }
        addTypeBinding(context, typeDef.identifier, typeDef.module);
    }

    // Add interface bindings
    for (uint32_t i = 0; i < list_size(context->ast->interfaces); i++) {
        Interface* interface = list_get(context->ast->interfaces, i);
        Binding* binding =
            getBindingInCurrentScope(context, interface->identifier);
        if (binding != NULL) {
            todo();
        }
        addInterfaceBinding(context, interface->identifier, i, currentAstId);
    }

    for (uint32_t i = 0; i < list_size(context->ast->importedInterfaces); i++) {
        ImportedInterface* interface =
            list_get(context->ast->importedInterfaces, i);
        Binding* binding =
            getBindingInCurrentScope(context, interface->identifier);
        if (binding != NULL) {
            todo();
        }
        addInterfaceBinding(
            context,
            interface->identifier,
            i,
            interface->module
        );
    }

    // Add function bindings
    for (uint32_t i = 0; i < list_size(context->ast->functions); i++) {
        Function* function = list_get(context->ast->functions, i);
        if (function->isImplementation) continue;
        Binding* binding =
            getBindingInCurrentScope(context, function->identifier);
        if (binding != NULL) {
            todo();
        }
        addFunctionBinding(context, function->identifier, i, currentAstId);
    }

    for (uint32_t i = 0; i < list_size(context->ast->importedFunctions); i++) {
        ImportedFunction* function =
            list_get(context->ast->importedFunctions, i);
        if (function->isImplementation) continue;
        Binding* binding =
            getBindingInCurrentScope(context, function->identifier);
        if (binding != NULL) {
            todo();
        }
        addFunctionBinding(context, function->identifier, i, function->module);
    }

    return true;
}

static uint32_t literalInContext(Context* context, Ast ast, uint32_t literal) {
    return ast_getLiteral(context->ast, ast_literalAsString(ast, literal));
}

static uint32_t typeInContext(
    Context* context, Ast ast, uint32_t typeId, Arena scratch
) {
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
        Uint32List parameters = {0};
        for (uint32_t i = 0; i < data->parameterCount; i++) {
            uint32_t parameter =
                typeInContext(context, ast, data->parameters[i], scratch);
            list_append(&scratch, parameters, parameter);
        }
        newType = ast_addTypeWithParams(
            context->ast,
            literalInContext(context, ast, data->literal),
            parameters
        );
        break;
    }
    }
    return newType;
}

static void importModuleUnqualified(
    Context* context, Ast other, uint32_t otherId, Arena scratch
) {
    // Add types
    for (uint32_t i = 0; i < list_size(other.typeDefinitions); i++) {
        TypeDefinition typeDef = *list_get(other.typeDefinitions, i);
        ImportedTypeDefinition impTypeDef = {0};
        impTypeDef.identifier = ast_getLiteral(
            context->ast,
            ast_literalAsString(other, typeDef.identifier)
        );
        if (list_size(typeDef.fields) != 0) {
            todo();
        }
        list_append(
            &context->ast->arena,
            context->ast->importedTypeDefinitions,
            impTypeDef
        );
    }

    // Add interfaces
    for (uint32_t i = 0; i < list_size(other.interfaces); i++) {
        Interface interface = *list_get(other.interfaces, i);
        ImportedInterface impInterface = {0};
        impInterface.module = otherId;
        impInterface.id = i;
        impInterface.identifier = ast_getLiteral(
            context->ast,
            ast_literalAsString(other, interface.identifier)
        );
        Uint32List argsTypes = {0};
        for (uint32_t i = 0; i < list_size(interface.arguments); i++) {
            FunctionArgument arg = *list_get(interface.arguments, i);
            FunctionArgument newArg = {0};
            newArg.identifier = ast_getLiteral(
                context->ast,
                ast_literalAsString(other, arg.identifier)
            );
            newArg.mutable = arg.mutable;
            newArg.type = typeInContext(context, other, arg.type, scratch);
            list_append(&context->ast->arena, impInterface.arguments, newArg);
            list_append(&scratch, argsTypes, newArg.type);
        }
        impInterface.parameter =
            typeInContext(context, other, interface.parameter, scratch);
        impInterface.returnType =
            typeInContext(context, other, interface.returnType, scratch);
        impInterface.type = ast_addFunctionType(
            context->ast,
            argsTypes,
            impInterface.returnType
        );
        list_append(
            &context->ast->arena,
            context->ast->importedInterfaces,
            impInterface
        );
    }

    // Add function definitions
    for (uint32_t i = 0; i < list_size(other.functions); i++) {
        Function function = *list_get(other.functions, i);
        if (!function.private) {
            ImportedFunction impFunction = {0};
            impFunction.id = i;
            impFunction.module = otherId;
            impFunction.identifier = ast_getLiteral(
                context->ast,
                ast_literalAsString(other, function.identifier)
            );
            Uint32List argsTypes = {0};
            for (uint32_t i = 0; i < list_size(function.arguments); i++) {
                FunctionArgument arg = *list_get(function.arguments, i);
                FunctionArgument newArg = {0};
                newArg.identifier = ast_getLiteral(
                    context->ast,
                    ast_literalAsString(other, arg.identifier)
                );
                newArg.mutable = arg.mutable;
                newArg.type = typeInContext(context, other, arg.type, scratch);
                list_append(
                    &context->ast->arena,
                    impFunction.arguments,
                    newArg
                );
                list_append(&scratch, argsTypes, newArg.type);
            }
            impFunction.returnType =
                typeInContext(context, other, function.returnType, scratch);
            impFunction.type = ast_addFunctionType(
                context->ast,
                argsTypes,
                impFunction.returnType
            );
            list_append(
                &context->ast->arena,
                context->ast->importedFunctions,
                impFunction
            );
        }
    }
}

static void init_context(
    Context* context,
    Arena contextArena,
    Ast* ast,
    uint32_t astId,
    Arena scratch
) {
    context->arena = contextArena;
    context->ast = ast;
    context->astId = astId;
}

static uint32_t newTypeVariable(Context* context) {
    return context->lastTypeVariable++;
}

static uint32_t getBuiltInType(Context* context, char* type) {
    uint32_t literal = ast_getLiteral(context->ast, type);
    TypeBinding* binding = getTypeBinding(context, literal);
    assert(binding);
    assert(binding->module == None());
    uint32_t newType =
        ast_addTypeWithParams(context->ast, literal, (Uint32List){0});
    return newType;
}

static uint32_t _instantiateType(
    Arena* arena, Context* context, uint32_t typeId, Uint32Hashmap* mappings
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
            if (array_hashmap_get(*mappings, data->literal) == NULL) {
                array_hashmap_set(
                    arena,
                    *mappings,
                    data->literal,
                    ast_addTypeVariable(context->ast, newTypeVariable(context))
                );
            }
            uint32_t mappedType = *array_hashmap_get(*mappings, data->literal);
            return mappedType;
        } else {
            // Instantiate parameters
            Uint32List parameters = {0};
            for (uint32_t i = 0; i < data->parameterCount; i++) {
                uint32_t parameter = data->parameters[i];
                uint32_t instType =
                    _instantiateType(arena, context, parameter, mappings);
                list_append(arena, parameters, instType);
            }

            // Instantiate type
            uint32_t newId =
                ast_addTypeWithParams(context->ast, data->literal, parameters);
            return newId;
        }
    }
    }
    return None();
}

static uint32_t instantiateType(Context* context, uint32_t id, Arena scratch) {
    if (id == 0) return 0;
    Uint32Hashmap mappings = {0};
    uint32_t result = _instantiateType(&scratch, context, id, &mappings);
    return result;
}

static uint32_t instantiateInterface(
    Context* context, uint32_t module, uint32_t id, Arena scratch
) {
    uint32_t parameter;
    uint32_t identifier;
    uint32_t type;
    if (module == context->astId) {
        Interface* interface = list_get(context->ast->interfaces, id);
        parameter = interface->parameter;
        identifier = interface->identifier;
        type = interface->type;
    } else {
        ImportedInterface* interface =
            list_get(context->ast->importedInterfaces, id);
        parameter = interface->parameter;
        identifier = interface->identifier;
        type = interface->type;
    }
    Uint32Hashmap mappings = {0};
    uint32_t instParameter =
        _instantiateType(&scratch, context, parameter, &mappings);
    if (context->functionBeingAnalyzed != NULL) {
        Constraint constraint = {
            .identifier = identifier,
            .parameter = instParameter,
            .module = module,
        };
        list_append(
            &context->ast->arena,
            context->functionBeingAnalyzed->constraints,
            constraint
        );
    }
    return _instantiateType(&scratch, context, type, &mappings);
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
    TypeBinding* binding = getTypeBinding(context, type->withParams.literal);
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
    uint32_t exprType = ast_getTypeOfInstruction(context->ast, code, expr);

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
    uint32_t expected = ast_getTypeOfInstruction(context->ast, code, called);
    if (ast_getType(*context->ast, expected)->kind == TYPE_WITH_PARAMS) {
        if (ast_getType(*context->ast, expected)->withParams.literal !=
            ast_getLiteral(context->ast, "->")) {
            todo();
        }
    }

    // Get actual type so far
    Uint32List argsTypes = {0};
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        uint32_t arg = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount + i
        );
        uint32_t argType = ast_getTypeOfInstruction(context->ast, code, arg);
        list_append(&scratch, argsTypes, argType);
    }
    uint32_t actualTypeId =
        ast_addFunctionType(context->ast, argsTypes, data->type);

    // Make type of called expression equal to expected type
    if (expected != None()) {
        makeEqual(context, actualTypeId, expected);
    }

    // Pop arguments and expression called
    for (uint32_t i = 0; i < data->argumentsCount + 1; i++) {
        stack_pop(context->stack);
    }
    // then add call
    stack_push(&context->arena, context->stack, id);

    // Return
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
    Binding* binding = getBinding(context, data->literal);
    if (binding == NULL) {
        todo();
    } else if (binding->kind == FUNCTION_BINDING) {
        if (binding->asFunction.module == context->astId) {
            Function* function =
                list_get(context->ast->functions, binding->asFunction.id);
            if (function->type == None() && !function->beingAnalyzed) {
                analyzeFunctionNotCompletelyTyped(context, function, scratch);
            }
        }
    }
    if (binding->kind == INTERFACE_BINDING) {
        data->type = instantiateInterface(
            context,
            binding->asInterface.module,
            binding->asInterface.id,
            scratch
        );
    } else {
        data->type = instantiateType(
            context,
            getBindingType(context, *binding),
            scratch
        );
    }
    stack_push(&context->arena, context->stack, id);
    return true;
}

static bool booleanLiteral(
    Context* context, Code* code, uint32_t id, AstBoolean* data, Arena scratch
) {
    uint32_t type = getBuiltInType(context, "Bool");
    data->type = type;
    stack_push(&context->arena, context->stack, id);
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
    void* data = ast_getData(context->ast, code, id);
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
    addScope(&newContext);
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
        addArgumentBinding(&newContext, argument->identifier, argument->type);
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
    uint32_t newType = ast_addTypeWithParams(ast, newParam, (Uint32List){0});
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
    addScope(&newContext);
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
        addArgumentBinding(&newContext, argument->identifier, argument->type);
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
    Uint32List args = {0};
    for (uint32_t i = 0; i < argsCount; i++) {
        uint32_t argType = list_get(function->arguments, i)->type;
        list_append(&scratch, args, argType);
    }
    function->type =
        ast_addFunctionType(newContext.ast, args, function->returnType);

    // Remove repeated constraints
    for (uint32_t i = 0; i < list_size(function->constraints); i++) {
        Constraint a = *list_get(function->constraints, i);
        for (uint32_t j = i + 1; j < list_size(function->constraints);) {
            bool constraintRemoved = false;
            Constraint b = *list_get(function->constraints, i);
            if (a.identifier == b.identifier) {
                Type* typeA = ast_findType(context->ast, a.parameter);
                Type* typeB = ast_findType(context->ast, a.parameter);
                if (typeA == typeB) {
                    list_removeIndex(function->constraints, j);
                    constraintRemoved = true;
                }
            }
            if (!constraintRemoved) {
                j++;
            }
        }
    }

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

bool analyze(Program program, uint32_t astId, Arena scratch) {
    // Initialize context
    Context context = {0};
    init_context(
        &context,
        arena_new(),
        program_getAst(&program, astId),
        astId,
        scratch
    );
    importModuleUnqualified(&context, program.builtin, 0, scratch);

    // Add top level bindings
    addScope(&context);
    bool result = addTopLevelDefinitionsBindings(&context, astId);
    if (!result) {
        todo();
    }

    // Analyze functions
    for (uint32_t i = 0; i < list_size(context.ast->functions); i++) {
        Function* function = list_get(context.ast->functions, i);
        if (function->type != None()) {
            analyzeFunction(&context, function, scratch);
        } else {
            analyzeFunctionNotCompletelyTyped(&context, function, scratch);
        }
    }

    // Analyze code
    analyzeCode(&context, &context.ast->code, scratch);

    // Remove scope
    removeScope(&context);

    // Free arena
    arena_free(&context.arena);

    // Return
    return true;
}