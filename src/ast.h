#ifndef ast_h
#define ast_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"

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

typedef uint32_t Data;
typedef Data NodeId;  // UINT32_MAX value is used to indicate none
typedef Data NodeCount;
typedef Data Boolean;
typedef Data LiteralId;

#define None() UINT32_MAX
#define hasValue(nodeId) (nodeId != UINT32_MAX)

typedef struct {
    LiteralId path;
} AstImport;

typedef struct {
    LiteralId literal;
} AstFunctionArgument;

typedef struct {
    LiteralId literal;
    NodeCount numberOfTypeParameters;
    NodeCount numberOfArguments;
    Data argumentsMutability;
    NodeId lastNode;
} AstFunction;

typedef struct {
} AstInterface;

typedef struct {
} AstExtern;

typedef struct {
    LiteralId literal;
    NodeId lastNode;
} AstTypeDefinition;

typedef struct {
    Boolean mutable;
    NodeId lastExpressionNode;
    NodeId lastTypeNode;
} AstDeclaration;

typedef struct {
    Boolean nonlocal;
    NodeId lastAssignableNode;
    NodeId lastExpressionNode;
    NodeId lastTypeNode;
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
    NodeId lastConditionNode;
    NodeId lastIfNode;
    NodeId lastElseNode;
} AstIfElse;

typedef struct {
} AstWhile;

typedef struct {
    NodeCount numberOfArguments;
    Data argumentsMutability;
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
} AstArray;

typedef struct {
} AstStructLiteral;

typedef struct {
    LiteralId literal;
    NodeId lastNode;
} AstType;

#include "error.h"

typedef Data DataId;
typedef ListType(uint8_t) Uint8List;
typedef ListType(Data) DataList;
typedef ListType(DataId) DataIdList;
typedef ListType(NodeId) NodeIdList;

typedef DataList Literals;
#define ast_literalExpand(ast, id) \
    *list_get(ast.literals, id), (char *)list_get(ast.literals, id + 1)
#define ast_literalAsStringView(ast, id)                                      \
    (StringView) {                                                            \
        *list_get(ast->literals, id), (char *)list_get(ast->literals, id + 1) \
    }

typedef size_t AstId;
typedef ListType(AstId) Imports;

typedef enum { FUNCTION_DEFINITION, TYPE_DEFINITION } DefinitionKind;

typedef struct {
    DefinitionKind kind;
    NodeId id;
} Definition;

typedef ListType(Definition) Definitions;

typedef struct {
    String canonicalPath;
    Uint8List nodes;
    DataList dataOrIndex;
    DataList data;
    Literals literals;
    ErrorList errors;
    Imports imports;
    Definitions definitions;
} Ast;

typedef ListType(Ast) AstList;

void ast_init(Ast *ast, String canonicalPath);
void ast_clear(Ast *ast);
NodeId ast_createNode(Ast *ast, AstKind kind);
NodeId ast_insertNode(
    Ast *ast, AstKind kind, NodeId location, DataId dataLocation
);
size_t ast_numberOfSlotsUsedInData(AstKind kind);
Data *_ast_getData(Ast *ast, NodeId node);
#define ast_getData(type, ast, nodeId) ((type *)_ast_getData(ast, nodeId))
void ast_setBit(Data *data, NodeCount position);
void ast_print(Ast ast);

void reportErrors(Ast ast);

#endif
