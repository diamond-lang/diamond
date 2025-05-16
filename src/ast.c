#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "types.h"

void initAst(Ast* ast, char* filePath) {
    ast->filePath = filePath;
    ast->nodes = (Uint8List)List();
    ast->dataOrIndex = (DataList)List();
    ast->data = (DataList)List();
    ast->errors = (ErrorList)List();
}

NodeId ast_createNode(Ast* ast, AstKind kind) {
    list_append(ast->nodes, kind);
    list_setCapacity(ast->dataOrIndex, ast->nodes.capacity);
    ast->dataOrIndex.items[ast->nodes.count - 1] = None();
    return ast->nodes.count - 1;
}

Data* _ast_getData(Ast* ast, NodeId node, size_t sizeOfData) {
    bool justOneField = sizeOfData == sizeof(Data);
    if (justOneField) {
        return &ast->dataOrIndex.items[node];
    } else {
        if (ast->dataOrIndex.items[node] == None()) {
            ast->dataOrIndex.items[node] = ast->data.count;
            list_appendCapacity(ast->data, sizeOfData);
        }
        return ast->data.items + ast->dataOrIndex.items[node];
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
    for (size_t i = 0; i < ast.nodes.count; i += 1) {
        AstKind kind = ast.nodes.items[i];
        printf("%02lu│ ", i);
        switch (kind) {
            case AST_INCLUDE: printf("include\n"); break;
            case AST_USE: printf("use\n"); break;
            case AST_FUNCTION: printf("function\n"); break;
            case AST_INTERFACE: printf("interface\n"); break;
            case AST_EXTERN: printf("extern\n"); break;
            case AST_TYPE_DEFINITION: printf("typeDefinitionm\n"); break;
            case AST_BLOCK: printf("block\n"); break;
            case AST_DECLARATION: printf("declaration\n"); break;
            case AST_ASSIGNMENT: printf("assignment\n"); break;
            case AST_RETURN: printf("return\n"); break;
            case AST_RETURN_WITH_EXPRESSION:
                printf("returnWithExpression\n");
                break;
            case AST_BREAK: printf("break\n"); break;
            case AST_CONTINUE: printf("continue\n"); break;
            case AST_IF_ELSE: printf("ifElse\n"); break;
            case AST_WHILE: printf("while\n"); break;
            case AST_EXPRESSION: printf("expression\n"); break;
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
            case AST_MOD:
                printf("%s\n", getBinaryOp(ast.nodes.items[i]));
                break;
            case AST_NEGATION: printf("-\n"); break;
            case AST_DEREFERENCE: printf("dereference\n"); break;
            case AST_ADDRESS_OF: printf("addressOf\n"); break;
            case AST_FIELD_ACCESS: printf("fieldAccess\n"); break;
            case AST_INDEX_ACCESS: printf("indexAccess\n"); break;
            case AST_FLOAT:
                printf(
                    "float(%s)\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
            case AST_INTEGER:
                printf(
                    "integer(%s)\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
            case AST_IDENTIFIER:
                printf(
                    "identifier(%s)\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
            case AST_BOOLEAN:
                printf(
                    "boolean(%s)\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
            case AST_STRING:
                printf(
                    "string(\"%s\")\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
            case AST_ARRAY: printf("arrayLiteral\n"); break;
            case AST_STRUCT_LITERAL: printf("structLiteral\n"); break;
            case AST_TYPE:
                printf(
                    "type(%s)\n",
                    &ast.literals.items[ast.dataOrIndex.items[i]]
                );
                break;
        }
    }
}

void ast_setBit(Data* data, NodeCount position) {
    *data = *data | 1 << position;
}