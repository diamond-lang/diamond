#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

typedef enum {
    AST_USE,
    AST_INCLUDE,
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

typedef uint32_t Data;
typedef Data NodeId;  // UINT32_MAX value is used to indicate none
typedef Data NodeCount;
typedef Data Boolean;
typedef Data LiteralId;

#define None() UINT32_MAX
#define hasValue(nodeId) (nodeId != UINT32_MAX)

typedef struct {
    LiteralId path;
} AstUse;

typedef struct {
    LiteralId path;
} AstInclude;

typedef struct {
    NodeCount numberOfTypeParameters;
    NodeCount numberOfArguments;
    Data argumentsMutability;
} AstFunction;

typedef struct {
    NodeId lastExpressionNode;
    NodeId lastTypeNode;
} AstDeclaration;

typedef struct {
    NodeId lastExpressionNode;
    NodeId lastTypeNode;
} AstAssignment;

typedef struct {
    NodeId lastConditionNode;
    NodeId lastIfNode;
    NodeId lastElseNode;
} AstIfElse;

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
    Boolean value;
} AstBoolean;

typedef struct {
    LiteralId literal;
} AstString;

typedef struct {
    NodeCount parameters;
} AstType;

#include "error.h"

typedef Data DataId;
typedef ListType(uint8_t) Uint8List;
typedef ListType(Data) DataList;
typedef ListType(DataId) DataIdList;
typedef ListType(NodeId) NodeIdList;

typedef struct {
    Uint8List nodes;
    DataList dataOrIndex;
    DataList data;
} Code;

typedef DataList Literals;
#define ast_literalExpand(ast, id) \
    *list_get(ast.literals, id), (char *)list_get(ast.literals, id + 1)
#define ast_literalAsStringView(ast, id)                                      \
    (StringView) {                                                            \
        *list_get(ast->literals, id), (char *)list_get(ast->literals, id + 1) \
    }

typedef size_t AstId;
typedef ListType(AstId) Imports;

typedef struct {
    String canonicalPath;
    Uint8List nodes;
    DataList dataOrIndex;
    DataList data;
    Literals literals;
    ErrorList errors;
    Imports imports;
} Ast;

typedef ListType(Ast) AstList;

void initAst(Ast *ast, String canonicalPath);
NodeId ast_createNode(Ast *ast, AstKind kind);
Data *_ast_getData(Ast *ast, NodeId node, size_t sizeOfData);
#define ast_getData(type, ast, nodeId) \
    ((type *)_ast_getData(ast, nodeId, sizeof(type)))
void ast_setBit(Data *data, NodeCount position);
void ast_print(Ast ast);

void reportErrors(Ast ast);

#endif