#ifndef ast_h
#define ast_h

#include "token.h"
#include "types.h"

typedef enum {
    AST_BLOCK,
    AST_FUNCTION_ARGUMENT,
    AST_FUNCTION,
    AST_INTERFACE,
    AST_BUILTIN,
    AST_EXTERN,
    AST_TYPE_DEF,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_IF_ELSE,
    AST_WHILE,
    AST_IMPORT,
    AST_LINK_WITH,
    AST_CALL_ARGUMENT,
    AST_CALL,
    AST_FLOAT,
    AST_INTEGER,
    AST_IDENTIFIER,
    AST_BOOLEAN,
    AST_STRING,
    AST_INTERPOLATED_STRING,
    AST_ARRAY,
    AST_STRUCT_FIELD,
    AST_STRUCT_LITERAL,
    AST_IF_ELSE_EXPR,
    AST_FIELD_ACCESS,
    AST_INDEX_ACCESS,
    AST_ADDRESS_OF,
    AST_DEREFERENCE,
    AST_NEW,
    AST_TYPE
} AstKind;

typedef int32_t NodeId;
typedef ListType(NodeId) NodeIdList;
typedef int32_t OptionalNodeId;
#define hasValue(optional) (optional >= 0)
#define None() -1

typedef struct {
    AstKind kind;
    union {
        struct {
            NodeIdList statements;
            NodeIdList imports;
            NodeIdList definitions;
        } block;

        struct {
            bool isMutable;
            NodeId identifier;
            OptionalNodeId type;
        } functionArgument;

        struct {
            NodeId identifier;
            NodeIdList typeParameters;
            NodeIdList arguments;
            NodeId body;
            OptionalNodeId type;
        } function;

        struct {
            NodeId identifier;
            NodeIdList arguments;
        } interface;

        struct {
            NodeId identifier;
            NodeIdList arguments;
            NodeId body;
        } builtin;

        struct {
            NodeId identifier;
            NodeIdList arguments;
            NodeId body;
        } externDef;

        struct {
            NodeId identifier;
            NodeIdList fields;
            NodeIdList cases;
        } typeDefinition;

        struct {
            bool isMutable;
            NodeId identifier;
            NodeId expression;
        } declaration;

        struct {
            NodeId assignable;
            NodeId expression;
        } assignment;

        struct {
            OptionalNodeId expression;
        } returnNode;

        struct {
        } breakNode;

        struct {
        } continuekNode;

        struct {
            NodeId condition;
            NodeId ifNode;
            OptionalNodeId elseNode;
        } ifElse;

        struct {
            NodeId condition;
            NodeId body;
        } whileNode;

        struct {
            NodeId path;
            bool includes;
        } importNode;

        struct {
            NodeId directives;
        } linkWith;

        struct {
            bool isMutable;
            OptionalNodeId identifier;
            NodeId expression;
        } callArgument;

        struct {
            NodeId callable;
            NodeIdList arguments;
            OptionalNodeId type;
        } call;

        struct {
            Token value;
            OptionalNodeId type;
        } floatNode;

        struct {
            Token value;
            OptionalNodeId type;
        } integer;

        struct {
            Token value;
            OptionalNodeId type;
        } identifier;

        struct {
            Token value;
            OptionalNodeId type;
        } boolean;

        struct {
            Token value;
            OptionalNodeId type;
        } string;

        struct {
            TokenList strings;
            NodeIdList expressions;
            OptionalNodeId type;
        } interpolatedString;

        struct {
            NodeIdList elements;
            OptionalNodeId type;
        } arrayNode;

        struct {
            NodeId identifier;
            NodeId expression;
        } structField;

        struct {
            NodeId identifier;
            NodeIdList fields;
            OptionalNodeId type;
        } structLiteral;

        struct {
            NodeId condition;
            NodeId ifNode;
            NodeId elseNode;
            OptionalNodeId type;
        } ifElseExpr;

        struct {
            NodeId accessed;
            NodeId identifier;
            OptionalNodeId type;
        } fieldAccess;

        struct {
            NodeId accessed;
            NodeId index;
            OptionalNodeId type;
        } indexAccess;

        struct {
            NodeId expression;
            OptionalNodeId type;
        } addressOf;

        struct {
            NodeId expression;
            OptionalNodeId type;
        } dereference;

        struct {
            NodeId expression;
            OptionalNodeId type;
        } newNode;

        struct {
            Token token;
            NodeIdList parameters;
        } type;
    };
} AstNode;

typedef ListType(AstNode) AstNodeList;

#include "error.h"

typedef struct {
    char* filePath;
    TokenList tokens;
    AstNodeList nodes;
    ErrorList errors;
} Ast;

NodeId ast_createNode(Ast* ast, AstNode nodeContent);
AstNode* ast_getNode(Ast ast, NodeId id);
bool ast_isExpression(AstKind kind);
void ast_setType(Ast ast, NodeId id, NodeId type);

void ast_print(Ast ast);
void ast_printNode(Ast ast, NodeId id, BoolStack isLast);

void reportErrors(Ast ast);

#endif