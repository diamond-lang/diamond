#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stdint.h>

#include "types.h"

typedef enum {
    AST_INCLUDE,
    AST_USE,
    AST_FUNCTION,
    AST_INTERFACE,
    AST_EXTERN,
    AST_TYPE_DEFINITION,
    AST_BLOCK,

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
    AST_EXPRESSION,
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

typedef uint32_t Data;
typedef Data NodeId;  // UINT32_MAX value is used to indicate none
typedef Data LiteralId;
typedef Data NodeCount;

#define None() UINT32_MAX
#define hasValue(nodeId) (nodeId != UINT32_MAX)

typedef struct {
    NodeCount numberOfTypeParameters;
    NodeCount numberOfArguments;
    Data argumentsMutability;
} AstFunction;

typedef struct {
    NodeId start;
} AstExpression;

typedef struct {
    NodeCount numberOfArguments;
    Data argumentsMutability;
} AstCall;

typedef struct {
    LiteralId literal;
} AstFloat;

typedef struct {
    LiteralId literal;
} AstInteger;

typedef struct {
    LiteralId literal;
} AstIdentifier;

typedef struct {
    Data value;
} AstBoolean;

typedef struct {
    LiteralId literal;
} AstString;

typedef struct {
} AstType;

#include "error.h"

typedef Data DataId;
typedef ListType(uint8_t) Uint8List;
typedef ListType(Data) DataList;
typedef ListType(DataId) DataIdList;
typedef ListType(NodeId) NodeIdList;

typedef struct {
    char *filePath;
    Uint8List nodes;
    DataList dataOrIndex;
    DataList data;
    Uint8List literals;
    ErrorList errors;
} Ast;

void initAst(Ast *ast, char *filePath);
NodeId ast_createNode(Ast *ast, AstKind kind);
bool ast_isExpression(AstKind kind);
void ast_setType(Ast ast, NodeId id, NodeId type);
void ast_setBit(Data *data, NodeCount position);
#define ast_appendData(ast, toAppend, id)                                     \
    do {                                                                      \
        ast.dataOrIndex.items[id] = ast.data.count;                           \
        list_appendCapacity(ast.data, sizeof(toAppend));                      \
        memcpy(ast.data.items + ast.data.count, &toAppend, sizeof(toAppend)); \
    } while (false);

void *ast_getNode(Ast ast, NodeId id);
void ast_print(Ast ast);

void reportErrors(Ast ast);

#endif