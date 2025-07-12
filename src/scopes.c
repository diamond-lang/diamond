#include "scopes.h"

#include "common.h"
#include "types.h"

void scopes_addScope(Scopes* scopes) {
    BindingMap newMap = Hashmap();
    stack_push(scopes->bindings, newMap);
}

void scopes_removeScope(Scopes* scopes) { todo(); }

Binding* scopes_getBinding(Scopes* scopes, uint32_t literalId) { todo(); }

TypeBinding* scopes_getTypeBinding(Scopes* scopes, uint32_t literalId) {
    todo();
}

void scopes_addBinding(BindingMap* scope, uint32_t literalId, Binding binding) {
    todo();
}
