#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "token.h"
#include "types.h"

bool ast_isExpression(Ast ast, NodeId id) {
    AstNode node = *ast_getNode(ast, id);
    switch (node.kind) {
        case AST_BLOCK: return false;
        case AST_FUNCTION_ARGUMENT: return false;
        case AST_FUNCTION: return false;
        case AST_INTERFACE: return false;
        case AST_BUILTIN: return false;
        case AST_EXTERN: return false;
        case AST_TYPE_DEF: return false;
        case AST_DECLARATION: return false;
        case AST_ASSIGNMENT: return false;
        case AST_RETURN: return false;
        case AST_BREAK: return false;
        case AST_CONTINUE: return false;
        case AST_IF_ELSE: {
            if (hasValue(node.ifElse.elseNode) &&
                ast_isExpression(ast, node.ifElse.ifNode) &&
                ast_isExpression(ast, node.ifElse.elseNode)) {
                return true;
            }
            return false;
        }
        case AST_WHILE: return false;
        case AST_IMPORT: return false;
        case AST_LINK_WITH: return false;
        case AST_CALL_ARGUMENT: return false;
        case AST_CALL: return true;
        case AST_FLOAT: return true;
        case AST_INTEGER: return true;
        case AST_IDENTIFIER: return true;
        case AST_BOOLEAN: return true;
        case AST_STRING: return true;
        case AST_INTERPOLATED_STRING: return true;
        case AST_ARRAY: return true;
        case AST_STRUCT_LITERAL: return true;
        case AST_STRUCT_FIELD: return false;
        case AST_FIELD_ACCESS: return true;
        case AST_INDEX_ACCESS: return true;
        case AST_ADDRESS_OF: return true;
        case AST_DEREFERENCE: return true;
        case AST_NEW: return true;
    }
}

AstNode* ast_getNode(Ast ast, NodeId id) {
    assert(0 <= id && id < ast.nodes.count);
    return &ast.nodes.items[id];
}

void ast_print(Ast ast) {
    printf("program\n");
    BoolStack isLast = Stack();
    ast_printNode(ast, 0, isLast);
}

void printIndentation(BoolStack isLast) {
    if (isLast.count == 0) return;
    for (int i = 0; i < isLast.count - 1; i++) {
        bool value = isLast.items[i];
        if (value == true) {
            printf("   ");
        } else {
            printf("│  ");
        }
    }
    bool lastIsLast = isLast.items[isLast.count - 1];
    if (lastIsLast) {
        printf("└──");
    } else {
        printf("├──");
    }
}

void ast_printNode(Ast ast, NodeId id, BoolStack isLast) {
    AstNode node = *ast_getNode(ast, id);
    switch (node.kind) {
        case AST_BLOCK: {
            size_t totalNodes = node.block.statements.count +
                                node.block.definitions.count +
                                node.block.imports.count;
            for (size_t i = 0; i < node.block.statements.count; i++) {
                stack_push(isLast, i + 1 == totalNodes);
                ast_printNode(
                    ast,
                    node.block.statements.items[i],
                    isLast

                );
                stack_pop(isLast);
            }
            break;
        }
        case AST_FUNCTION_ARGUMENT: todo(); break;
        case AST_FUNCTION: todo(); break;
        case AST_INTERFACE: todo(); break;
        case AST_BUILTIN: todo(); break;
        case AST_EXTERN: todo(); break;
        case AST_TYPE_DEF: todo(); break;
        case AST_DECLARATION:
            printIndentation(isLast);
            printf("%s\n", node.declaration.isMutable ? "=" : "be");
            stack_push(isLast, false);
            ast_printNode(ast, node.declaration.identifier, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.declaration.expression, isLast);
            stack_pop(isLast);
            break;
        case AST_ASSIGNMENT: {
            printIndentation(isLast);
            printf(":=\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.assignment.assignable, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.assignment.expression, isLast);
            stack_pop(isLast);
            break;
        }
        case AST_RETURN: todo(); break;
        case AST_BREAK:
            printIndentation(isLast);
            printf("break\n");
            break;
        case AST_CONTINUE:
            printIndentation(isLast);
            printf("continue\n");
            break;
        case AST_IF_ELSE: {
            if (ast_isExpression(ast, id)) {
                bool lastNode = isLast.items[isLast.count - 1];
                if (lastNode == true) {
                    stack_pop(isLast);
                    stack_push(isLast, false);
                }
                printIndentation(isLast);
                printf("if\n");
                stack_push(isLast, false);
                ast_printNode(ast, node.ifElse.condition, isLast);
                stack_pop(isLast);
                stack_push(isLast, true);
                ast_printNode(ast, node.ifElse.ifNode, isLast);
                stack_pop(isLast);
                if (hasValue(node.ifElse.elseNode)) {
                    stack_pop(isLast);
                    stack_push(isLast, lastNode);
                    printIndentation(isLast);
                    printf("else\n");
                    stack_push(isLast, true);
                    ast_printNode(ast, node.ifElse.elseNode, isLast);
                    stack_pop(isLast);
                }
            } else {
                bool lastNode = isLast.items[isLast.count - 1];
                if (lastNode == true && hasValue(node.ifElse.elseNode)) {
                    stack_pop(isLast);
                    stack_push(isLast, false);
                }
                printIndentation(isLast);
                printf("if\n");
                stack_push(isLast, false);
                ast_printNode(ast, node.ifElse.condition, isLast);
                stack_pop(isLast);
                ast_printNode(ast, node.ifElse.ifNode, isLast);
                if (hasValue(node.ifElse.elseNode)) {
                    stack_pop(isLast);
                    stack_push(isLast, lastNode);
                    printIndentation(isLast);
                    printf("else\n");
                    ast_printNode(ast, node.ifElse.elseNode, isLast);
                }
            }
            break;
        }
        case AST_WHILE: {
            printIndentation(isLast);
            printf("while\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.whileNode.condition, isLast);
            stack_pop(isLast);
            ast_printNode(ast, node.whileNode.body, isLast);
            break;
        }
        case AST_IMPORT: todo(); break;
        case AST_LINK_WITH: todo(); break;
        case AST_CALL_ARGUMENT:
            if (node.callArgument.isMutable) {
                printIndentation(isLast);
                printf("mut\n");
                stack_push(isLast, true);
                ast_printNode(ast, node.callArgument.expression, isLast);
                stack_pop(isLast);
            } else {
                ast_printNode(ast, node.callArgument.expression, isLast);
            }
            break;
        case AST_CALL:
            printIndentation(isLast);
            if (ast_getNode(ast, node.call.callable)->kind == AST_IDENTIFIER) {
                char* literal = token_getLiteral(
                    ast_getNode(ast, node.call.callable)->identifier.value
                );
                printf("%s\n", literal);
            } else {
                todo();
            }
            for (size_t i = 0; i < node.call.arguments.count; i++) {
                stack_push(isLast, i + 1 == node.call.arguments.count);
                ast_printNode(ast, node.call.arguments.items[i], isLast);
                stack_pop(isLast);
            }
            break;
        case AST_FLOAT:
            printIndentation(isLast);
            printf("%s\n", node.floatNode.value.literal.content);
            break;
        case AST_INTEGER:
            printIndentation(isLast);
            printf("%s\n", node.floatNode.value.literal.content);
            break;
        case AST_IDENTIFIER:
            printIndentation(isLast);
            printf("%s\n", node.floatNode.value.literal.content);
            break;
        case AST_BOOLEAN:
            printIndentation(isLast);
            printf("%s\n", node.integer.value.kind == TRUE ? "true" : "false");
            break;
        case AST_STRING:
            printIndentation(isLast);
            printf("%s\n", node.floatNode.value.literal.content);
            break;
        case AST_INTERPOLATED_STRING: todo(); break;
        case AST_ARRAY: todo(); break;
        case AST_STRUCT_LITERAL: todo(); break;
        case AST_STRUCT_FIELD: todo(); break;
        case AST_FIELD_ACCESS: todo(); break;
        case AST_INDEX_ACCESS: todo(); break;
        case AST_ADDRESS_OF: todo(); break;
        case AST_DEREFERENCE: todo(); break;
        case AST_NEW: todo(); break;
    }
}