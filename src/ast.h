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
    uint32_t type;
    Code code;
} Function;

typedef struct {
    uint32_t identifier;
    Uint32List fields;
    Uint32List fieldTypes;
} TypeDefinition;

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
    uint32_t parameters;
} TypeWithParams;

typedef struct {
    uint32_t returnType;
    uint32_t argumentsCount;
    uint32_t arguments;
} FunctionType;

typedef enum { TYPE_VARIABLE, TYPE_WITH_PARAMS, FUNCTION_TYPE } TypeKind;

typedef struct {
    uint32_t kind;
    union {
        TypeVariable variable;
        TypeWithParams withParams;
        FunctionType functionType;
    };
} Type;

typedef ListType(Type) TypeList;

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
    TypeDefinitionList typeDefinitions;
    Code code;
    LiteralList literals;
    TypeList types;
    ErrorList errors;
    Arena arena;
} Ast;

void ast_clear(Ast *ast);

typedef ListType(Ast) AstList;

// Instructions handling
uint32_t ast_addInstruction(Arena *arena, Code *code, AstInstructionKind kind);
AstInstructionKind ast_getInstruction(Code code, uint32_t id);
uint32_t *ast_getDataOrIndex(Code code, uint32_t id);
void ast_addData(Arena *arena, Code *code, uint32_t instruction);
void *ast_getData(Code *code, uint32_t instruction);

// Types handling
uint32_t ast_addTypeVariable(
    Arena *arena, TypeList *types, uint32_t typeVariable
);
uint32_t ast_addTypeWithParams(
    Arena *arena, TypeList *types, uint32_t literal, uint32_t parameterCount
);
uint32_t ast_addFunctionType(
    Arena *arena, TypeList *types, uint32_t returnType, uint32_t argumentsCount
);
Type *ast_getType(TypeList types, uint32_t type);
uint32_t ast_getTypeOfInstruction(Ast ast, uint32_t instruction);
String ast_typeAsString(Arena *arena, Ast ast, Type *type, Arena typeArena);

// Literals handling
uint32_t ast_getLiteral(Ast *ast, char *literal);
uint32_t ast_getLiteralWithLength(Ast *ast, char *pointer, uint32_t length);
char *ast_literalAsString(Ast ast, uint32_t literal);
#define ast_literalAsView(ast, id)                        \
    (StringView) {                                        \
        list_get((ast).literals, id - 1)->length,         \
            (char *)(arena_getPointer(                    \
                (ast).arena,                              \
                char,                                     \
                list_get((ast).literals, id - 1)->arenaId \
            ))                                            \
    }

// Printing
void ast_printType(Ast ast, uint32_t typeId, Arena scratch);
void ast_print(Ast ast, String path, Arena scratch, Arena otherScratch);
void reportErrors(Ast ast, String path, Arena scratch);

#endif