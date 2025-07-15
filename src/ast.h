#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stdint.h>

#include "error.h"
#include "types.h"

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
    uint32_t type;
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
    uint32_t type;
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
    uint32_t literal;
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
    ImportList imports;
    FunctionList functions;
    TypeDefinitionList typeDefinitions;
    Code code;
    Uint32List literals;
    TypeList types;
    ErrorList errors;
} Ast;

typedef ListType(Ast) AstList;

void ast_init(Ast *ast);
void ast_clear(Ast *ast);

uint32_t ast_addInstruction(Code *code, AstInstructionKind kind);
void *ast_getData(Code *code, uint32_t instruction);
uint32_t ast_createTypeVariable(Ast *ast, uint32_t typeVariable);
uint32_t ast_createTypeApplication(Ast *ast);
#define ast_getTypeApplication(ast, id)                           \
    (assert(list_get((ast).types, id)->kind == TYPE_APPLICATION), \
     (TypeApplication *)&list_get((ast).types, id)->application)
uint32_t ast_getNextParameter(Ast *ast, uint32_t typeId);
void ast_printType(Ast ast, uint32_t typeId);
uint32_t ast_getLiteral(Ast *ast, StringView view);
#define ast_literalExpand(ast, id) \
    *list_get((ast).literals, id), (char *)list_get((ast).literals, id + 1)
#define ast_literalAsStringView(ast, id)             \
    (StringView) {                                   \
        *list_get((ast).literals, id),               \
            (char *)list_get((ast).literals, id + 1) \
    }
void ast_setBit(uint32_t *data, uint32_t position);
void ast_print(Ast ast, String path);
void reportErrors(Ast ast, String path);

#endif