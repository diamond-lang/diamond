#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.h"
#include "error.h"
#include "types.h"

// Code
typedef enum {
    // Statements
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_RETURN_LAST_EXPRESSION,
    AST_BREAK,
    AST_CONTINUE,
    AST_IF_ELSE,
    AST_WHILE,

    // Expressions
    AST_CALL,
    AST_IF_ELSE_EXPRESSION,
    AST_ADDRESS_OF,
    AST_DEREFERENCE,
    AST_FIELD_ACCESS,
    AST_INDEX_ACCESS,

    // Literals
    AST_FLOAT,
    AST_INTEGER,
    AST_IDENTIFIER,
    AST_BOOLEAN,
    AST_STRING,
    AST_ARRAY,
    AST_STRUCT_LITERAL,
} AstInstructionKind;

typedef struct {
    bool mutable;
    uint32_t identifier;
    uint32_t type;
} AstDeclaration;

typedef struct {
    bool nonlocal;
    uint32_t type;
} AstAssignment;

typedef struct {
} AstReturn;

typedef struct {
} AstReturnLastExpression;

typedef struct {
} AstBreak;

typedef struct {
} AstContinue;

typedef struct {
    uint32_t ifBlockEnd;
    uint32_t elseBlockEnd;
} AstIfElse;

typedef struct {
    uint32_t whileEnd;
} AstWhile;

typedef struct {
    uint32_t argumentsCount;
    uint32_t argumentsMutability;
    uint32_t type;
} AstCall;

typedef struct {
    uint32_t type;
} AstIfElseExpression;

typedef struct {
    uint32_t type;
} AstNot;

typedef struct {
    uint32_t type;
} AstOr;

typedef struct {
    uint32_t type;
} AstAnd;

typedef struct {
    uint32_t type;
} AstEqualEqual;

typedef struct {
    uint32_t type;
} AstNotEqual;

typedef struct {
    uint32_t type;
} AstLess;

typedef struct {
    uint32_t type;
} AstLessEqual;

typedef struct {
    uint32_t type;
} AstGreater;

typedef struct {
    uint32_t type;
} AstGreaterEqual;

typedef struct {
    uint32_t type;
    uint32_t operatorType;
} AstAdd;

typedef struct {
    uint32_t type;
} AstSubtract;

typedef struct {
    uint32_t type;
} AstMul;

typedef struct {
    uint32_t type;
} AstDiv;

typedef struct {
    uint32_t type;
} AstMod;

typedef struct {
    uint32_t type;
} AstNegation;

typedef struct {
    uint32_t type;
} AstAddressOf;

typedef struct {
    uint32_t type;
} AstDereference;

typedef struct {
    uint32_t type;
} AstFieldAccess;

typedef struct {
    uint32_t type;
} AstIndexAccess;

typedef struct {
    uint32_t literal;
    uint32_t type;
} AstFloat;

typedef struct {
    uint32_t literal;
    uint32_t type;
} AstInteger;

typedef struct {
    uint32_t literal;
    uint32_t type;
} AstIdentifier;

typedef struct {
    uint32_t value;
    uint32_t type;
} AstBoolean;

typedef struct {
    uint32_t literal;
} AstString;

typedef struct {
} AstArray;

typedef struct {
} AstStructLiteral;

typedef struct {
    Uint8List instructions;
    Uint32List dataOrIndex;
} Code;

// Definitions
typedef struct {
    uint32_t path;
} Import;

typedef ListType(Import) ImportList;

typedef struct {
    uint32_t identifier;
    uint32_t parameter;
    uint32_t module;
} Constraint;

typedef ListType(Constraint) ConstraintList;

typedef struct {
    bool mutable;
    uint32_t identifier;
    uint32_t type;
} FunctionArgument;

typedef ListType(FunctionArgument) FunctionArgumentList;

typedef struct {
    uint32_t identifier;
    Uint32List parameters;
    FunctionArgumentList arguments;
    uint32_t returnType;
    uint32_t type;
    ConstraintList constraints;
    Code code;
    bool beingAnalyzed;
    bool builtin;
    bool private;
    bool isImplementation;
} Function;

typedef ListType(Function) FunctionList;

typedef struct {
    uint32_t identifier;
    FunctionArgumentList arguments;
    uint32_t returnType;
    uint32_t type;
    ConstraintList constraints;
    uint32_t id;
    uint32_t module;
    bool isImplementation;
} ImportedFunction;

typedef ListType(ImportedFunction) ImportedFunctionList;

typedef struct {
    uint32_t astId;
    uint32_t functionId;
} Implementation;

typedef ListType(Implementation) ImplementationList;

typedef struct {
    uint32_t identifier;
    uint32_t parameter;
    FunctionArgumentList arguments;
    uint32_t returnType;
    uint32_t type;
    ImplementationList implementations;
} Interface;

typedef ListType(Interface) InterfaceList;

typedef struct {
    uint32_t identifier;
    uint32_t parameter;
    FunctionArgumentList arguments;
    uint32_t returnType;
    uint32_t type;
    uint32_t id;
    uint32_t module;
} ImportedInterface;

typedef ListType(ImportedInterface) ImportedInterfaceList;

typedef struct {
    uint32_t identifier;
    Uint32List fields;
    Uint32List fieldTypes;
} TypeDefinition;

typedef ListType(TypeDefinition) TypeDefinitionList;

typedef struct {
    uint32_t identifier;
    Uint32List fields;
    Uint32List fieldTypes;
    uint32_t id;
    uint32_t module;
} ImportedTypeDefinition;

typedef ListType(ImportedTypeDefinition) ImportedTypeDefinitionList;

// Types
typedef struct {
    uint32_t id;
    uint32_t forwarded;
} TypeVariable;

typedef struct {
    uint32_t literal;
    uint32_t parameterCount;
    uint32_t parameters[];
} TypeWithParams;

typedef enum { TYPE_VARIABLE, TYPE_WITH_PARAMS } TypeKind;

typedef struct {
    uint32_t kind;
    union {
        TypeVariable variable;
        TypeWithParams withParams;
    };
} Type;

// Literals
typedef struct {
    uint32_t hash;
    uint32_t length;
    uint32_t arenaId;
} Literal;

typedef ListType(Literal) LiteralList;

// Ast
typedef struct Ast {
    Uint32List importedAsts;
    ImportList imports;
    FunctionList functions;
    ImportedFunctionList importedFunctions;
    InterfaceList interfaces;
    ImportedInterfaceList importedInterfaces;
    TypeDefinitionList typeDefinitions;
    ImportedTypeDefinitionList importedTypeDefinitions;
    Code code;
    LiteralList literals;
    ErrorList errors;
    Arena arena;
} Ast;

void ast_clear(Ast *ast);

typedef ListType(Ast) AstList;

// Instructions handling
uint32_t ast_addInstruction(Ast *ast, Code *code, AstInstructionKind kind);
uint32_t ast_insertInst(
    Ast *ast, Code *code, AstInstructionKind kind, uint32_t offset
);
AstInstructionKind ast_getInstruction(Code code, uint32_t id);
uint32_t *ast_getDataOrIndex(Code code, uint32_t id);
void ast_addData(Ast *ast, Code *code, uint32_t instruction);
void *ast_getData(Ast *ast, Code *code, uint32_t instruction);

// Types handling
uint32_t ast_addTypeVariable(Ast *ast, uint32_t typeVariable);
uint32_t ast_addTypeWithParams(
    Ast *ast, uint32_t literal, Uint32List parameters
);
uint32_t ast_addFunctionType(
    Ast *ast, Uint32List arguments, uint32_t returnType
);
Type *ast_findType(Ast *ast, uint32_t type);
void ast_makeEqual(TypeVariable *typeVariable, uint32_t other);
Type *ast_getType(Ast ast, uint32_t type);
uint32_t ast_getTypeOfInstruction(Ast *ast, Code *code, uint32_t instruction);
String ast_typeAsString(Arena *arena, Ast ast, uint32_t typeId);
bool ast_isTypeVariable(Ast ast, TypeWithParams *type);

// Literals handling
uint32_t ast_getLiteral(Ast *ast, char *literal);
uint32_t ast_getLiteralWithLength(Ast *ast, char *pointer, uint32_t length);
char *ast_literalAsString(Ast ast, uint32_t literal);
#define ast_literalAsView(ast, id)                        \
    (StringView) {                                        \
        list_get((ast).literals, id - 1)->length,         \
            (char *)(arena_getPointer(                    \
                (ast).arena,                              \
                list_get((ast).literals, id - 1)->arenaId \
            ))                                            \
    }

// Printing
void ast_printType(Ast ast, uint32_t typeId, Arena scratch);
void ast_print(Ast ast, String path, Arena scratch1, Arena scratch2);
void reportErrors(Ast ast, String path, Arena scratch);

#endif