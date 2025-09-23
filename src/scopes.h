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
    uint32_t identifier;
    uint32_t type;
    uint32_t module;
} Binding;

typedef StackType(Binding) BindingStack;

typedef struct {
    BindingStack bindings;
    Uint32Stack scopeStart;
} Scopes;

typedef struct {
    uint32_t identifier;
    uint32_t module;
} TypeBinding;

typedef ListType(TypeBinding) TypeBindingList;

typedef struct {
    TypeBindingList types;
    Scopes scopes;
} Bindings;

void scopes_addScope(Arena* arena, Bindings* bindings);
void scopes_removeScope(Bindings* bindings);
Binding* scopes_getBinding(Bindings bindings, uint32_t literalId);
Binding* scopes_getBindingInCurrentScope(Bindings bindings, uint32_t literalId);
void scopes_addArgumentBinding(
    Arena* arena, Bindings* bindings, uint32_t literalId, uint32_t type
);
void scopes_addVariableBinding(
    Arena* arena, Bindings* bindings, uint32_t literalId, uint32_t type
);
void scopes_addFunctionBinding(
    Arena* arena,
    Bindings* bindings,
    uint32_t literalId,
    uint32_t type,
    uint32_t module
);
TypeBinding* scopes_getTypeBinding(Bindings bindings, uint32_t literalId);
void scopes_addTypeBinding(
    Arena* arena, Bindings* bindings, uint32_t identifier, uint32_t module
);

#endif