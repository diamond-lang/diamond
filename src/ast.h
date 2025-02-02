#ifndef ast_h
#define ast_h

#include "token.h"
#include "types.h"

typedef struct {
    int32_t id;
} Type;

typedef struct {
    int32_t id;
} InterfaceType;

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
    AST_STRUCT_FIELD,
    AST_STRUCT_LITERAL,
    AST_FLOAT,
    AST_INTEGER,
    AST_IDENTIFIER,
    AST_BOOLEAN,
    AST_STRING,
    AST_INTERPOLATED_STRING,
    AST_ARRAY,
    AST_FIELD_ACCESS,
    AST_INDEX_ACCESS,
    AST_ADDRESS_OF,
    AST_DEREFERENCE,
    AST_NEW
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
        } functionArgument;

        struct {
            NodeId identifier;
            NodeIdList arguments;
            NodeId body;
        } function;

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
            NodeIdList arguments;
        } interface;

        struct {
            NodeId identifier;
            NodeIdList fields;
            NodeIdList cases;
        } type;

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
        } call;

        struct {
            NodeId identifier;
            NodeId expression;
        } structField;

        struct {
            NodeId identifier;
            NodeIdList fields;
        } structLiteral;

        struct {
            Token value;
        } floatNode;

        struct {
            Token value;
        } integer;

        struct {
            Token value;
        } identifier;

        struct {
            Token value;
        } boolean;

        struct {
            Token value;
        } string;

        struct {
            TokenList strings;
            NodeIdList expressions;
        } interpolatedString;

        struct {
            NodeIdList elements;
        } arrayNode;

        struct {
            NodeId accessed;
            NodeId identifier;
        } fieldAccess;

        struct {
            NodeId accessed;
            NodeId index;
        } indexAccess;

        struct {
            NodeId expression;
        } addressOf;

        struct {
            NodeId expression;
        } deference;

        struct {
            NodeId expression;
        } newNode;
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

AstNode* ast_getNode(Ast ast, NodeId id);
bool ast_isExpression(Ast ast, NodeId id);

void ast_print(Ast ast);
void ast_printNode(Ast ast, NodeId id, BoolStack isLast);

void reportErrors(Ast ast);

#endif