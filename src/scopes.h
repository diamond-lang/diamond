#ifndef scopes_h
#define scopes_h

#include <stdint.h>

#include "types.h"

typedef enum {
    FunctionBiding,
    TypeBinding,
    VariableBinding,
    ArgumentBinding
} BindingKind;

typedef struct {
    BindingKind kind;
    uint32_t literalId;
    uint32_t module;
    uint32_t id;
} Binding;

typedef HashmapType(Binding) BindingMap;
typedef StackType(BindingMap) BindingMapStack;

void scopes_addScope(BindingMapStack* scopes);
void scopes_removeScope(BindingMapStack* scopes);
Binding* scopes_getBinding(BindingMapStack* scopes, uint32_t literalId);
void scopes_addBinding(BindingMap* scope, uint32_t literalId, Binding binding);

#endif