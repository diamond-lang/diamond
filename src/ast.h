#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

#define None() UINT32_MAX

typedef enum {
    AST_IMPORT,
    AST_FUNCTION_ARGUMENT,
    AST_FUNCTION,
    AST_INTERFACE,
    AST_EXTERN,
    AST_TYPE_DEFINITION,

    // Statements
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_RETURN_WITH_EXPRESSION,
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

    // Type
    AST_TYPE
} AstKind;

typedef struct {
    uint32_t path;
} AstImport;

typedef struct {
    uint32_t literal;
} AstFunctionArgument;

typedef struct {
    uint32_t literal;
    uint32_t numberOfTypeParameters;
    uint32_t numberOfArguments;
    uint32_t argumentsMutability;
    uint32_t lastNode;
} AstFunction;

typedef struct {
} AstInterface;

typedef struct {
} AstExtern;

typedef struct {
    uint32_t literal;
    uint32_t lastNode;
} AstTypeDefinition;

typedef struct {
    uint32_t mutable;
    uint32_t lastExpressionNode;
    uint32_t lastTypeNode;
} AstDeclaration;

typedef struct {
    uint32_t nonlocal;
    uint32_t lastAssignableNode;
    uint32_t lastExpressionNode;
    uint32_t lastTypeNode;
} AstAssignment;

typedef struct {
} AstReturn;

typedef struct {
} AstReturnWithExpression;

typedef struct {
} AstBreak;

typedef struct {
} AstContinue;

typedef struct {
    uint32_t lastConditionNode;
    uint32_t lastIfNode;
    uint32_t lastElseNode;
} AstIfElse;

typedef struct {
} AstWhile;

typedef struct {
    uint32_t numberOfArguments;
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
    uint32_t literal;
    uint32_t lastNode;
} AstType;

#include "error.h"

#define ast_literalExpand(ast, id) \
    *list_get(ast.literals, id), (char *)list_get(ast.literals, id + 1)
#define ast_literalAsStringView(ast, id)                                      \
    (StringView) {                                                            \
        *list_get(ast->literals, id), (char *)list_get(ast->literals, id + 1) \
    }

typedef enum { FUNCTION_DEFINITION, TYPE_DEFINITION } DefinitionKind;

typedef struct {
    DefinitionKind kind;
    uint32_t id;
} Definition;

typedef ListType(Definition) Definitions;

typedef struct {
    String canonicalPath;
    Uint8List nodes;
    Uint32List dataOrIndex;
    Uint32List data;
    Uint32List literals;
    ErrorList errors;
    Uint32List imports;
    Definitions definitions;
} Ast;

typedef ListType(Ast) AstList;

void ast_init(Ast *ast, String canonicalPath);
void ast_clear(Ast *ast);
uint32_t ast_createNode(Ast *ast, AstKind kind);
uint32_t ast_insertNode(
    Ast *ast, AstKind kind, uint32_t location, uint32_t dataLocation
);
size_t ast_numberOfSlotsUsedInData(AstKind kind);
uint32_t *_ast_getData(Ast *ast, uint32_t node);
#define ast_getData(type, ast, nodeId) ((type *)_ast_getData(ast, nodeId))
void ast_setBit(uint32_t *data, uint32_t position);
void ast_print(Ast ast);

void reportErrors(Ast ast);

#endif
