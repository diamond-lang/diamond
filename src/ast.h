#include "token.h"
#include "types.h"

typedef enum {
    AstBlock,
    AstFunctionArgument,
    AstFunction,
    AstInterface,
    AstTypeDef,
    AstDeclaration,
    AstAssignment,
    AstReturn,
    AstBreak,
    AstContinue,
    AstIfElse,
    AstWhile,
    AstUse,
    AstLinkWith,
    AstCallArgument,
    AstCall,
    AstStructLiteral,
    AstFloat,
    AstInteger,
    AstIdentifier,
    AstBoolean,
    AstString,
    AstInterpolatedString,
    AstArray,
    AstFieldAccess,
    AstAddressOf,
    AstDereference,
    AstNew
} AstKind;

typedef size_t NodeId;
typedef ListType(NodeId) NodeIdList;

typedef struct {
    bool isPresent;
    NodeId nodeId;
} OptionalNodeId;

struct AstNode {
    AstKind kind;
    union {
        struct {
            NodeIdList statemnts;
            NodeIdList imports;
            NodeIdList defintions;
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
        } importNode;

        struct {
            NodeId path;
        } includeNode;

        struct {
            NodeId directives;
        } linkWith;

        struct {
            bool isMutable;
            OptionalNodeId identifier;
            NodeId expression;
        } callArgument;

        struct {
            NodeId identifier;
            NodeIdList arguments;
        } call;

        struct {
            NodeId identifier;
            NodeIdList fields;
        } structLiteral;

        struct {
            double value;
        } floatNode;

        struct {
            int64_t value;
        } integerNode;

        struct {
            Token identifier;
        } identifier;

        struct {
            bool value;
        } boolean;

        struct {
            String string;
        } string;

        struct {
            NodeIdList strings;
            NodeIdList expression;
        } interpolatedString;

        struct {
            NodeIdList elements;
        } arrayNode;

        struct {
            NodeId accessed;
            NodeIdList fieldsAccessed;
        } fieldAccess;

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
};