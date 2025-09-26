#include "scopes.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "types.h"

void scopes_addScope(Arena* arena, Scopes* scopes) {
    stack_push(arena, scopes->scopeStart, stack_size(scopes->bindings));
}

void scopes_removeScope(Scopes* scopes) {
    scopes->bindings.count = *stack_top(scopes->scopeStart);
    stack_pop(scopes->scopeStart);
}

Binding* scopes_getBinding(Scopes scopes, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(scopes.bindings);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(scopes.bindings, i);
        if (binding->identifier == literalId) return binding;
    }
    return NULL;
}

Binding* scopes_getBindingInCurrentScope(Scopes scopes, uint32_t literalId) {
    uint32_t start = *stack_top(scopes.scopeStart);
    uint32_t bindingsCount = stack_size(scopes.bindings);
    assert(bindingsCount >= start);
    for (uint32_t i = bindingsCount - 1; start <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(scopes.bindings, i);
        if (binding->identifier == literalId) return binding;
    }
    return NULL;
}

void scopes_addArgumentBinding(
    Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
) {
    Binding binding = {ARGUMENT_BINDING, literalId, type};
    stack_push(arena, scopes->bindings, binding);
}

void scopes_addVariableBinding(
    Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
) {
    Binding binding = {VARIABLE_BINDING, literalId, type};
    stack_push(arena, scopes->bindings, binding);
}

void scopes_addFunctionBinding(
    Arena* arena,
    Scopes* scopes,
    uint32_t literalId,
    uint32_t type,
    uint32_t module
) {
    Binding binding = {FUNCTION_BINDING, literalId, type, module};
    stack_push(arena, scopes->bindings, binding);
}

TypeBinding* scopes_getTypeBinding(Scopes bindings, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(bindings.types);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        TypeBinding* binding = stack_get(bindings.types, i);
        if (binding->identifier == literalId) return binding;
    }
    return NULL;
}

void scopes_addTypeBinding(
    Arena* arena, Scopes* bindings, uint32_t identifier, uint32_t module
) {
    TypeBinding binding = {identifier, module};
    stack_push(arena, bindings->types, binding);
}
