#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stdint.h>

#include "error.h"
#include "types.h"

#define None() UINT32_MAX

// Code
typedef enum {
    // Statements
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_RETURN_EXPRESSION,
    AST_BREAK,
    AST_CONTINUE,
    AST_IF_ELSE,
    AST_WHILE,

    // Expressions
    AST_CALL,
    AST_IF_ELSE_EXPRESSION,
    AST_NOT,
    AST_OR,
    AST_AND,
    AST_EQUAL_EQUAL,
    AST_NOT_EQUAL,
    AST_LESS,
    AST_LESS_EQUAL,
    AST_GREATER,
    AST_GREATER_EQUAL,
    AST_ADD,
    AST_SUBTRACT,
    AST_MUL,
    AST_DIV,
    AST_MOD,
    AST_NEGATION,
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
} AstReturnExpression;

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
} AstCall;

typedef struct {
} AstIfElseExpression;

typedef struct {
} AstNot;

typedef struct {
} AstOr;

typedef struct {
} AstAnd;

typedef struct {
} AstEqualEqual;

typedef struct {
} AstNotEqual;

typedef struct {
} AstLess;

typedef struct {
} AstLessEqual;

typedef struct {
} AstGreater;

typedef struct {
} AstGreaterEqual;

typedef struct {
} AstAdd;

typedef struct {
} AstSubtract;

typedef struct {
} AstMul;

typedef struct {
} AstDiv;

typedef struct {
} AstMod;

typedef struct {
} AstNegation;

typedef struct {
} AstAddressOf;

typedef struct {
} AstDereference;

typedef struct {
} AstFieldAccess;

typedef struct {
} AstIndexAccess;

typedef struct {
    uint32_t literal;
} AstFloat;

typedef struct {
    uint32_t literal;
} AstInteger;

typedef struct {
    uint32_t literal;
} AstIdentifier;

typedef struct {
    uint32_t value;
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
    Uint32List data;
} Code;

// Definitions
typedef struct {
    uint32_t path;
} Import;

typedef struct {
    bool mutable;
    uint32_t identifier;
    uint32_t type;
} FunctionArgument;

typedef ListType(FunctionArgument) FunctionArgumentList;

typedef struct {
    uint32_t identifier;
    FunctionArgumentList arguments;
    uint32_t returnType;
    Code code;
} Function;

void ast_initFunction(Function *function, uint32_t lifetime);

typedef struct {
    uint32_t identifier;
    Uint32List fields;
    Uint32List fieldTypes;
} TypeDefinition;

void ast_initTypeDefinition(TypeDefinition *typeDefinition, uint32_t lifetime);

typedef ListType(Import) ImportList;
typedef ListType(Function) FunctionList;
typedef ListType(TypeDefinition) TypeDefinitionList;

// Types
typedef struct {
    uint32_t id;
} TypeVariable;

typedef struct {
    uint32_t identifier;
    uint32_t parameterCount;
} TypeApplication;

typedef enum { TYPE_VARIABLE, TYPE_APPLICATION } TypeKind;

typedef struct {
    TypeKind kind;
    union {
        TypeVariable variable;
        TypeApplication application;
    };
} Type;

typedef ListType(Type) TypeList;

// Ast
typedef struct {
    Uint32List importedAsts;
    String canonicalPath;
    ImportList imports;
    FunctionList functions;
    TypeDefinitionList typeDefinitions;
    Code code;
    Uint32List literals;
    TypeList types;
    ErrorList errors;
} Ast;

typedef ListType(Ast) AstList;

void ast_init(Ast *ast, String canonicalPath);
void ast_clear(Ast *ast);
uint32_t ast_addInstruction(Code *code, AstInstructionKind kind);
uint32_t *_ast_getData(Code *code, uint32_t instruction);
#define ast_getData(type, code, instruction) \
    ((type *)_ast_getData(code, instruction))
uint32_t ast_createType(Ast *ast, TypeKind kind);
#define ast_literalExpand(ast, id) \
    *list_get(ast.literals, id), (char *)list_get(ast.literals, id + 1)
#define ast_literalAsStringView(ast, id)                                      \
    (StringView) {                                                            \
        *list_get(ast->literals, id), (char *)list_get(ast->literals, id + 1) \
    }
void ast_setBit(uint32_t *data, uint32_t position);
void ast_print(Ast ast);
void reportErrors(Ast ast);

#endif