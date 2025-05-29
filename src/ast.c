#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "types.h"

void initAst(Ast* ast, String canonicalPath) {
    ast->canonicalPath = canonicalPath;
    ast->nodes = (Uint8List)List();
    ast->dataOrIndex = (DataList)List();
    ast->data = (DataList)List();
    ast->errors = (ErrorList)List();
    ast->imports = (Imports)List();
}

NodeId ast_createNode(Ast* ast, AstKind kind) {
    list_append(ast->nodes, kind);
    list_append(ast->dataOrIndex, None());
    NodeId id = list_size(ast->nodes) - 1;
    switch (kind) {
        case AST_INCLUDE: break;
        case AST_USE: break;
        case AST_FUNCTION: ast_getData(AstFunction, ast, id); break;
        case AST_INTERFACE: break;
        case AST_EXTERN: break;
        case AST_TYPE_DEFINITION: break;
        case AST_DECLARATION: {
            AstDeclaration* data = ast_getData(AstDeclaration, ast, id);
            data->lastTypeNode = None();
            break;
        }
        case AST_ASSIGNMENT: break;
        case AST_RETURN: break;
        case AST_RETURN_WITH_EXPRESSION: break;
        case AST_BREAK: break;
        case AST_CONTINUE: break;
        case AST_IF_ELSE: {
            AstIfElse* data = ast_getData(AstIfElse, ast, id);
            data->lastElseNode = None();
            break;
        }
        case AST_WHILE: break;
        case AST_CALL: ast_getData(AstCall, ast, id); break;
        case AST_IF_ELSE_EXPRESSION: break;
        case AST_NOT: break;
        case AST_OR: break;
        case AST_AND: break;
        case AST_EQUAL_EQUAL: break;
        case AST_NOT_EQUAL: break;
        case AST_LESS: break;
        case AST_LESS_EQUAL: break;
        case AST_GREATER: break;
        case AST_GREATER_EQUAL: break;
        case AST_ADD: break;
        case AST_SUBTRACT: break;
        case AST_MUL: break;
        case AST_DIV: break;
        case AST_MOD: break;
        case AST_NEGATION: break;
        case AST_DEREFERENCE: break;
        case AST_ADDRESS_OF: break;
        case AST_FIELD_ACCESS: break;
        case AST_INDEX_ACCESS: break;
        case AST_FLOAT: ast_getData(AstFloat, ast, id); break;
        case AST_INTEGER: ast_getData(AstInteger, ast, id); break;
        case AST_IDENTIFIER: ast_getData(AstIdentifier, ast, id); break;
        case AST_BOOLEAN: ast_getData(AstBoolean, ast, id); break;
        case AST_STRING: ast_getData(AstString, ast, id); break;
        case AST_ARRAY: break;
        case AST_STRUCT_LITERAL: break;
        case AST_TYPE: ast_getData(AstType, ast, id); break;
    }
    return id;
}

Data* _ast_getData(Ast* ast, NodeId node, size_t sizeOfData) {
    bool justOneField = sizeOfData == sizeof(Data);
    if (justOneField) {
        return list_get(ast->dataOrIndex, node);
    } else {
        if (*list_get(ast->dataOrIndex, node) == None()) {
            *list_get(ast->dataOrIndex, node) = list_size(ast->data);
            list_ensureExtraCapacity(ast->data, sizeOfData / sizeof(Data));
            list_setSize(
                ast->data,
                list_size(ast->data) + sizeOfData / sizeof(Data)
            );
        }
        return list_get(ast->data, *list_get(ast->dataOrIndex, node));
    }
}

void ast_setType(Ast ast, NodeId id, NodeId type) { todo(); }

static char* getBinaryOp(AstKind kind) {
    switch ((uint8_t)kind) {
        case AST_OR: return "or";
        case AST_AND: return "and";
        case AST_EQUAL_EQUAL: return "==";
        case AST_NOT_EQUAL: return "!=";
        case AST_LESS: return "<";
        case AST_LESS_EQUAL: return "<=";
        case AST_GREATER: return ">";
        case AST_GREATER_EQUAL: return ">=";
        case AST_ADD: return "+";
        case AST_SUBTRACT: return "-";
        case AST_MUL: return "*";
        case AST_DIV: return "/";
        case AST_MOD: return "%";
    }
    unreachable();
}

void ast_print(Ast ast) {
    for (size_t i = 0; i < list_size(ast.nodes); i += 1) {
        AstKind kind = *list_get(ast.nodes, i);
        printf("%02zu│ ", i);
        switch (kind) {
            case AST_USE:
                printf(
                    "use %.*s\n",
                    ast_literalExpand(ast, ast_getData(AstUse, &ast, i)->path)
                );
                break;
            case AST_INCLUDE:
                printf(
                    "include %.*s\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstInclude, &ast, i)->path
                    )
                );
                break;
            case AST_FUNCTION: printf("function\n"); break;
            case AST_INTERFACE: printf("interface\n"); break;
            case AST_EXTERN: printf("extern\n"); break;
            case AST_TYPE_DEFINITION: printf("typeDefinition\n"); break;
            case AST_DECLARATION: {
                if (ast_getData(AstDeclaration, &ast, i)->lastTypeNode !=
                    None()) {
                    printf(
                        "declaration(lastExpressionNode: %u, lastTypeNode: "
                        "%u)\n",
                        ast_getData(AstDeclaration, &ast, i)
                            ->lastExpressionNode,
                        ast_getData(AstDeclaration, &ast, i)->lastTypeNode
                    );
                } else {
                    printf(
                        "declaration(lastExpressionNode: %u, lastTypeNode: "
                        "none)\n",
                        ast_getData(AstDeclaration, &ast, i)->lastExpressionNode
                    );
                }
                break;
            }
            case AST_ASSIGNMENT: {
                if (ast_getData(AstAssignment, &ast, i)->lastTypeNode !=
                    None()) {
                    printf(
                        "assignment(lastExpressionNode: %u, lastTypeNode: "
                        "%u)\n",
                        ast_getData(AstAssignment, &ast, i)->lastExpressionNode,
                        ast_getData(AstAssignment, &ast, i)->lastTypeNode
                    );
                } else {
                    printf(
                        "assignment(lastExpressionNode: %u, lastTypeNode: "
                        "none)\n",
                        ast_getData(AstAssignment, &ast, i)->lastExpressionNode
                    );
                }
                break;
            }
            case AST_RETURN: printf("return\n"); break;
            case AST_RETURN_WITH_EXPRESSION:
                printf("returnWithExpression\n");
                break;
            case AST_BREAK: printf("break\n"); break;
            case AST_CONTINUE: printf("continue\n"); break;
            case AST_IF_ELSE:
                if (ast_getData(AstIfElse, &ast, i)->lastElseNode != None()) {
                    printf(
                        "ifElse(lastConditionNode: %u, lastIfNode: %u, "
                        "lastElseNode: %u)\n",
                        ast_getData(AstIfElse, &ast, i)->lastConditionNode,
                        ast_getData(AstIfElse, &ast, i)->lastIfNode,
                        ast_getData(AstIfElse, &ast, i)->lastElseNode
                    );
                } else {
                    printf(
                        "ifElse(lastConditionNode: %u, lastIfNode: %u, "
                        "lastElseNode: none)\n",
                        ast_getData(AstIfElse, &ast, i)->lastConditionNode,
                        ast_getData(AstIfElse, &ast, i)->lastIfNode
                    );
                }
                break;
            case AST_WHILE: printf("while\n"); break;
            case AST_CALL: printf("call\n"); break;
            case AST_IF_ELSE_EXPRESSION: printf("ifElseExpression\n"); break;
            case AST_NOT: printf("not\n"); break;
            case AST_OR:
            case AST_AND:
            case AST_EQUAL_EQUAL:
            case AST_NOT_EQUAL:
            case AST_LESS:
            case AST_LESS_EQUAL:
            case AST_GREATER:
            case AST_GREATER_EQUAL:
            case AST_ADD:
            case AST_SUBTRACT:
            case AST_MUL:
            case AST_DIV:
            case AST_MOD: printf("%s\n", getBinaryOp(kind)); break;
            case AST_NEGATION: printf("-\n"); break;
            case AST_DEREFERENCE: printf("dereference\n"); break;
            case AST_ADDRESS_OF: printf("addressOf\n"); break;
            case AST_FIELD_ACCESS: printf("fieldAccess\n"); break;
            case AST_INDEX_ACCESS: printf("indexAccess\n"); break;
            case AST_FLOAT:
                printf(
                    "float(value: %.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstFloat, &ast, i)->literal
                    )
                );
                break;
            case AST_INTEGER:
                printf(
                    "integer(value: %.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstInteger, &ast, i)->literal
                    )
                );
                break;
            case AST_IDENTIFIER:
                printf(
                    "identifier(value: %.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstIdentifier, &ast, i)->literal
                    )
                );
                break;
            case AST_BOOLEAN:
                printf(
                    "boolean(value: %s)\n",
                    ast_getData(AstBoolean, &ast, i)->value ? "true" : "false"
                );
                break;
            case AST_STRING:
                printf(
                    "string(value: \"%.*s\")\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstString, &ast, i)->literal
                    )
                );
                break;
            case AST_ARRAY: printf("arrayLiteral\n"); break;
            case AST_STRUCT_LITERAL: printf("structLiteral\n"); break;
            case AST_TYPE:
                printf(
                    "type (parameters: %d)\n",
                    ast_getData(AstType, &ast, i)->parameters
                );
                break;
        }
    }
}

void ast_setBit(Data* data, NodeCount position) {
    *data = *data | 1 << position;
}