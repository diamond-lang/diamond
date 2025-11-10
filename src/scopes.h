#include <stdint.h>
#ifndef SCOPES_H

#include "ast.h"    // IWYU pragma: keep
#include "types.h"  // IWYU pragma: keep

#define defineScopesWith(T)                                                    \
    typedef enum {                                                             \
        FUNCTION_BINDING,                                                      \
        INTERFACE_BINDING,                                                     \
        VARIABLE_BINDING,                                                      \
        ARGUMENT_BINDING                                                       \
    } BindingKind;                                                             \
                                                                               \
    typedef struct {                                                           \
        uint32_t id;                                                           \
        uint32_t module;                                                       \
    } FunctionBinding;                                                         \
                                                                               \
    typedef struct {                                                           \
        uint32_t id;                                                           \
        uint32_t module;                                                       \
    } InterfaceBinding;                                                        \
                                                                               \
    typedef struct {                                                           \
        T data;                                                                \
    } VariableBinding;                                                         \
                                                                               \
    typedef struct {                                                           \
        T data;                                                                \
    } ArgumentBinding;                                                         \
                                                                               \
    typedef struct {                                                           \
        BindingKind kind;                                                      \
        uint32_t literal;                                                      \
        union {                                                                \
            FunctionBinding asFunction;                                        \
            InterfaceBinding asInterface;                                      \
            VariableBinding asVariable;                                        \
            ArgumentBinding asArgument;                                        \
        };                                                                     \
    } Binding;                                                                 \
                                                                               \
    typedef StackType(Binding) BindingStack;                                   \
                                                                               \
    typedef struct {                                                           \
        uint32_t literalId;                                                    \
        uint32_t moduleId;                                                     \
        uint32_t id;                                                           \
    } TypeBinding;                                                             \
                                                                               \
    typedef ListType(TypeBinding) TypeBindingList;                             \
                                                                               \
    typedef struct {                                                           \
        BindingStack bindings;                                                 \
        Uint32Stack scopeStart;                                                \
        TypeBindingList types;                                                 \
    } Scopes;                                                                  \
                                                                               \
    static void addScope(Arena* arena, Scopes* scopes) {                       \
        stack_push(arena, scopes->scopeStart, stack_size(scopes->bindings));   \
    }                                                                          \
                                                                               \
    static void removeScope(Scopes* scopes) {                                  \
        scopes->bindings.count = *stack_top(scopes->scopeStart);               \
        stack_pop(scopes->scopeStart);                                         \
    }                                                                          \
                                                                               \
    static Binding* getBinding(Scopes* scopes, uint32_t literalId) {           \
        uint32_t bindingsCount = stack_size(scopes->bindings);                 \
        for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount;      \
             i--) {                                                            \
            Binding* binding = stack_get(scopes->bindings, i);                 \
            if (binding->literal == literalId) return binding;                 \
        }                                                                      \
        return NULL;                                                           \
    }                                                                          \
                                                                               \
    static Binding* getBindingInCurrentScope(                                  \
        Scopes* scopes,                                                        \
        uint32_t literalId                                                     \
    ) {                                                                        \
        uint32_t start = *stack_top(scopes->scopeStart);                       \
        uint32_t bindingsCount = stack_size(scopes->bindings);                 \
        assert(bindingsCount >= start);                                        \
        for (uint32_t i = bindingsCount - 1; start <= i && i < bindingsCount;  \
             i--) {                                                            \
            Binding* binding = stack_get(scopes->bindings, i);                 \
            if (binding->literal == literalId) return binding;                 \
        }                                                                      \
        return NULL;                                                           \
    }                                                                          \
                                                                               \
    static void addArgumentBinding(                                            \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        uint32_t literalId,                                                    \
        T data                                                                 \
    ) {                                                                        \
        Binding binding = {                                                    \
            .kind = ARGUMENT_BINDING,                                          \
            .literal = literalId,                                              \
            .asArgument = (ArgumentBinding){.data = data}                      \
        };                                                                     \
                                                                               \
        stack_push(arena, scopes->bindings, binding);                          \
    }                                                                          \
                                                                               \
    static void addVariableBinding(                                            \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        uint32_t literalId,                                                    \
        T data                                                                 \
    ) {                                                                        \
        Binding binding = {                                                    \
            .kind = VARIABLE_BINDING,                                          \
            .literal = literalId,                                              \
            .asVariable = (VariableBinding){.data = data}                      \
        };                                                                     \
        stack_push(arena, scopes->bindings, binding);                          \
    }                                                                          \
                                                                               \
    static void addFunctionBinding(                                            \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        uint32_t literalId,                                                    \
        uint32_t id,                                                           \
        uint32_t module                                                        \
    ) {                                                                        \
        Binding binding = {                                                    \
            .kind = FUNCTION_BINDING,                                          \
            .literal = literalId,                                              \
            .asFunction = (FunctionBinding){.id = id, .module = module}        \
        };                                                                     \
        stack_push(arena, scopes->bindings, binding);                          \
    }                                                                          \
                                                                               \
    static void addInterfaceBinding(                                           \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        uint32_t literalId,                                                    \
        uint32_t id,                                                           \
        uint32_t module                                                        \
    ) {                                                                        \
        Binding binding = {                                                    \
            .kind = INTERFACE_BINDING,                                         \
            .literal = literalId,                                              \
            .asInterface = (InterfaceBinding){.id = id, .module = module}      \
        };                                                                     \
        stack_push(arena, scopes->bindings, binding);                          \
    }                                                                          \
                                                                               \
    static void addTypeBinding(                                                \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        uint32_t literal,                                                      \
        uint32_t id,                                                           \
        uint32_t module                                                        \
    ) {                                                                        \
        TypeBinding binding = {                                                \
            .literalId = literal,                                              \
            .id = id,                                                          \
            .moduleId = module                                                 \
        };                                                                     \
        stack_push(arena, scopes->types, binding);                             \
    }                                                                          \
                                                                               \
    static TypeBinding* getTypeBinding(Scopes* scopes, uint32_t literalId) {   \
        uint32_t bindingsCount = stack_size(scopes->types);                    \
        if (bindingsCount > 0) {                                               \
            for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount;  \
                 i--) {                                                        \
                TypeBinding* binding = stack_get(scopes->types, i);            \
                if (binding->literalId == literalId) return binding;           \
            }                                                                  \
        }                                                                      \
        return NULL;                                                           \
    }                                                                          \
                                                                               \
    static TypeReference typeBindingAsTypeReference(TypeBinding* binding) {    \
        assert(binding);                                                       \
        TypeReference result = {0};                                            \
        result.id = binding->id;                                               \
        result.moduleId = binding->moduleId;                                   \
        return result;                                                         \
    }                                                                          \
                                                                               \
    static bool addTopLevelTypeBindings(                                       \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        Ast* builtin,                                                          \
        Ast* module,                                                           \
        uint32_t moduleId                                                      \
    ) {                                                                        \
        if (moduleId != 0) {                                                   \
            /* Add builtin type bindings */                                    \
            for (uint32_t i = 0; i < list_size(builtin->typeDefinitions);      \
                 i++) {                                                        \
                TypeDefinition* typeDef =                                      \
                    list_get(builtin->typeDefinitions, i);                     \
                char* literal =                                                \
                    ast_literalAsString(*builtin, typeDef->identifier);        \
                uint32_t identifier = ast_getLiteral(module, literal);         \
                TypeBinding* binding = getTypeBinding(scopes, identifier);     \
                if (binding != NULL) {                                         \
                    todo();                                                    \
                }                                                              \
                addTypeBinding(arena, scopes, identifier, i, 0);               \
            }                                                                  \
        }                                                                      \
                                                                               \
        /* Add types bindings */                                               \
        for (uint32_t i = 0; i < list_size(module->typeDefinitions); i++) {    \
            TypeDefinition typeDef = *list_get(module->typeDefinitions, i);    \
            TypeBinding* binding = getTypeBinding(scopes, typeDef.identifier); \
            if (binding != NULL) {                                             \
                todo();                                                        \
            }                                                                  \
            addTypeBinding(arena, scopes, typeDef.identifier, i, moduleId);    \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }                                                                          \
                                                                               \
    static bool addTopLevelBindings(                                           \
        Arena* arena,                                                          \
        Scopes* scopes,                                                        \
        Ast* builtin,                                                          \
        Ast* module,                                                           \
        uint32_t moduleId                                                      \
    ) {                                                                        \
        (void                                                                  \
        )addTopLevelTypeBindings(arena, scopes, builtin, module, moduleId);    \
                                                                               \
        if (moduleId != 0) {                                                   \
            /*  Add builtin interface bindings */                              \
            for (uint32_t i = 0; i < list_size(builtin->interfaces); i++) {    \
                Interface* interface = list_get(builtin->interfaces, i);       \
                char* literal =                                                \
                    ast_literalAsString(*builtin, interface->identifier);      \
                uint32_t identifier = ast_getLiteral(module, literal);         \
                Binding* binding =                                             \
                    getBindingInCurrentScope(scopes, identifier);              \
                if (binding != NULL) {                                         \
                    todo();                                                    \
                }                                                              \
                addInterfaceBinding(arena, scopes, identifier, i, 0);          \
            }                                                                  \
                                                                               \
            /* Add builtin function bindings */                                \
            for (uint32_t i = 0; i < list_size(builtin->functions); i++) {     \
                Function* function = list_get(builtin->functions, i);          \
                char* literal =                                                \
                    ast_literalAsString(*builtin, function->identifier);       \
                uint32_t identifier = ast_getLiteral(module, literal);         \
                if (function->isImplementation) continue;                      \
                Binding* binding =                                             \
                    getBindingInCurrentScope(scopes, identifier);              \
                if (binding != NULL) {                                         \
                    todo();                                                    \
                }                                                              \
                addFunctionBinding(arena, scopes, identifier, i, 0);           \
            }                                                                  \
        }                                                                      \
                                                                               \
        /* Add interface bindings */                                           \
        for (uint32_t i = 0; i < list_size(module->interfaces); i++) {         \
            Interface* interface = list_get(module->interfaces, i);            \
            Binding* binding =                                                 \
                getBindingInCurrentScope(scopes, interface->identifier);       \
            if (binding != NULL) {                                             \
                todo();                                                        \
            }                                                                  \
            addInterfaceBinding(                                               \
                arena,                                                         \
                scopes,                                                        \
                interface->identifier,                                         \
                i,                                                             \
                moduleId                                                       \
            );                                                                 \
        }                                                                      \
                                                                               \
        /* Add function bindings */                                            \
        for (uint32_t i = 0; i < list_size(module->functions); i++) {          \
            Function* function = list_get(module->functions, i);               \
            if (function->isImplementation) continue;                          \
            Binding* binding =                                                 \
                getBindingInCurrentScope(scopes, function->identifier);        \
            if (binding != NULL) {                                             \
                todo();                                                        \
            }                                                                  \
            addFunctionBinding(                                                \
                arena,                                                         \
                scopes,                                                        \
                function->identifier,                                          \
                i,                                                             \
                moduleId                                                       \
            );                                                                 \
        }                                                                      \
                                                                               \
        return true;                                                           \
    }
#endif