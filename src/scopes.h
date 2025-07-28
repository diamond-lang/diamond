#ifndef scopes_h
#define scopes_h

#include <stdint.h>

#include "types.h"

typedef enum {
    FUNCTION_BINDING,
    VARIABLE_BINDING,
    ARGUMENT_BINDING
} BindingKind;

typedef struct {
    BindingKind kind;
    uint32_t id;
    uint32_t module;
    uint32_t type;
} Binding;

typedef HashmapType(Binding) BindingMap;
typedef StackType(BindingMap) BindingMapStack;

typedef struct {
    uint32_t id;
    uint32_t module;
} TypeBinding;

typedef HashmapType(TypeBinding) TypeBindingMap;

typedef struct {
    TypeBindingMap types;
    BindingMapStack bindings;
} Scopes;

void scopes_addScope(Scopes* scopes);
void scopes_removeScope(Scopes* scopes);
Binding* scopes_getBinding(Scopes* scopes, uint32_t literalId);
TypeBinding* scopes_getTypeBinding(Scopes* scopes, uint32_t literalId);
BindingMap* scopes_current(Scopes* scopes);
void scopes_addBinding(BindingMap* scope, uint32_t literalId, Binding binding);

#endif