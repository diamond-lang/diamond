#ifndef scopes_h
#define scopes_h

#include <stdint.h>

#include "types.h"

typedef enum {
    FUNCTION_BINDING,
    INTERFACE_BINDING,
    VARIABLE_BINDING,
    ARGUMENT_BINDING
} BindingKind;

typedef struct {
    BindingKind kind;
    uint32_t identifier;
    uint32_t id;
    uint32_t type;
    uint32_t module;
    uint32_t parameter;  // for interfaces
} Binding;

typedef StackType(Binding) BindingStack;

typedef struct {
    uint32_t identifier;
    uint32_t module;
} TypeBinding;

typedef ListType(TypeBinding) TypeBindingList;

typedef struct {
    BindingStack bindings;
    Uint32Stack scopeStart;
    TypeBindingList types;
} Scopes;

void scopes_addScope(Arena* arena, Scopes* scopes);
void scopes_removeScope(Scopes* scopes);
Binding* scopes_getBinding(Scopes scopes, uint32_t literalId);
Binding* scopes_getBindingInCurrentScope(Scopes scopes, uint32_t literalId);
void scopes_addArgumentBinding(
    Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
);
void scopes_addVariableBinding(
    Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
);
void scopes_addFunctionBinding(
    Arena* arena,
    Scopes* scopes,
    uint32_t literalId,
    uint32_t id,
    uint32_t type,
    uint32_t module
);
void scopes_addInterfaceBinding(
    Arena* arena,
    Scopes* scopes,
    uint32_t literalId,
    uint32_t id,
    uint32_t type,
    uint32_t module,
    uint32_t parameter
);
TypeBinding* scopes_getTypeBinding(Scopes scopes, uint32_t literalId);
void scopes_addTypeBinding(
    Arena* arena, Scopes* scopes, uint32_t identifier, uint32_t module
);

#endif