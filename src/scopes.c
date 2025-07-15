#include "scopes.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "types.h"

void scopes_addScope(Scopes* scopes) {
    BindingMap newMap = Hashmap();
    stack_push(scopes->bindings, newMap);
}

void scopes_removeScope(Scopes* scopes) { todo(); }

Binding* scopes_getBinding(Scopes* scopes, uint32_t literalId) {
    Binding* binding = NULL;
    uint32_t scopesCount = stack_size(scopes->bindings);
    for (uint32_t scope = scopesCount - 1; 0 <= scope && scope < scopesCount;
         scope--) {
        binding = hashmap_get(*stack_get(scopes->bindings, scope), literalId);
        if (binding != NULL) break;
    }
    return binding;
}

BindingMap* scopes_current(Scopes* scopes) {
    assert(list_size(scopes->bindings) > 0);
    return stack_get(scopes->bindings, list_size(scopes->bindings) - 1);
}

TypeBinding* scopes_getTypeBinding(Scopes* scopes, uint32_t literalId) {
    return hashmap_get(scopes->types, literalId);
}

void scopes_addBinding(BindingMap* scope, uint32_t literalId, Binding binding) {
    hashmap_set(*scope, literalId, binding);
}
