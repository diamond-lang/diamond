#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "token.h"
#include "types.h"

NodeId ast_createNode(Ast* ast, AstNode nodeContent) {
    list_append(ast->nodes, nodeContent);
    NodeId id = ast->nodes.count - 1;
    AstNode* node = ast_getNode(*ast, id);
    switch (node->kind) {
        case AST_BLOCK:
            node->block.statements = (NodeIdList)List();
            node->block.definitions = (NodeIdList)List();
            node->block.imports = (NodeIdList)List();
            break;
        case AST_FUNCTION_ARGUMENT: node->functionArgument.type = None(); break;
        case AST_FUNCTION:
            node->function.typeParameters = (NodeIdList)List();
            node->function.arguments = (NodeIdList)List();
            node->function.type = None();
            break;
        case AST_INTERFACE:
            node->interface.arguments = (NodeIdList)List();
            break;
        case AST_BUILTIN: node->builtin.arguments = (NodeIdList)List(); break;
        case AST_EXTERN: node->externDef.arguments = (NodeIdList)List(); break;
        case AST_TYPE_DEFINITION:
            node->typeDefinition.fields = (NodeIdList)List();
            node->typeDefinition.cases = (NodeIdList)List();
            break;
        case AST_CASE_DEFINITION:
            node->caseDefinition.fields = (NodeIdList)List();
            node->caseDefinition.cases = (NodeIdList)List();
            break;
        case AST_DECLARATION: break;
        case AST_ASSIGNMENT: break;
        case AST_RETURN: node->returnNode.expression = None(); break;
        case AST_BREAK: break;
        case AST_CONTINUE: break;
        case AST_IF_ELSE: node->ifElse.elseNode = None(); break;
        case AST_WHILE: break;
        case AST_IMPORT: break;
        case AST_LINK_WITH: break;
        case AST_CALL_ARGUMENT:
            node->callArgument.isMutable = false;
            node->callArgument.identifier = None();
            break;
        case AST_CALL: node->call.type = None(); break;
        case AST_BINARY: node->binary.type = None(); break;
        case AST_UNARY: node->unary.type = None(); break;
        case AST_FLOAT: node->floatNode.type = None(); break;
        case AST_INTEGER: node->integer.type = None(); break;
        case AST_IDENTIFIER: node->identifier.type = None(); break;
        case AST_BOOLEAN: node->boolean.type = None(); break;
        case AST_STRING: node->string.type = None(); break;
        case AST_INTERPOLATED_STRING:
            node->interpolatedString.expressions = (NodeIdList)List();
            node->interpolatedString.type = None();
            break;
        case AST_ARRAY:
            node->arrayNode.elements = (NodeIdList)List();
            node->arrayNode.type = None();
            break;
        case AST_STRUCT_FIELD: break;
        case AST_STRUCT_LITERAL:
            node->structLiteral.fields = (NodeIdList)List();
            node->structLiteral.type = None();
            break;
        case AST_IF_ELSE_EXPR: node->ifElseExpr.type = None(); break;
        case AST_FIELD_ACCESS: node->fieldAccess.type = None(); break;
        case AST_INDEX_ACCESS: node->indexAccess.type = None(); break;
        case AST_NEW: node->newNode.type = None(); break;
        case AST_TYPE: node->type.parameters = (NodeIdList)List(); break;
    }
    return id;
}

AstNode* ast_getNode(Ast ast, NodeId id) {
    assert(0 <= id && id < ast.nodes.count);
    return &ast.nodes.items[id];
}

bool ast_isExpression(AstKind kind) {
    switch (kind) {
        case AST_BLOCK: return false;
        case AST_FUNCTION_ARGUMENT: return false;
        case AST_FUNCTION: return false;
        case AST_INTERFACE: return false;
        case AST_BUILTIN: return false;
        case AST_EXTERN: return false;
        case AST_TYPE_DEFINITION: return false;
        case AST_CASE_DEFINITION: return false;
        case AST_DECLARATION: return false;
        case AST_ASSIGNMENT: return false;
        case AST_RETURN: return false;
        case AST_BREAK: return false;
        case AST_CONTINUE: return false;
        case AST_IF_ELSE: return false;
        case AST_WHILE: return false;
        case AST_IMPORT: return false;
        case AST_LINK_WITH: return false;
        case AST_CALL_ARGUMENT: return false;
        case AST_CALL: return true;
        case AST_BINARY: return true;
        case AST_UNARY: return true;
        case AST_FLOAT: return true;
        case AST_INTEGER: return true;
        case AST_IDENTIFIER: return true;
        case AST_BOOLEAN: return true;
        case AST_STRING: return true;
        case AST_INTERPOLATED_STRING: return true;
        case AST_ARRAY: return true;
        case AST_STRUCT_FIELD: return false;
        case AST_STRUCT_LITERAL: return true;
        case AST_IF_ELSE_EXPR: return true;
        case AST_FIELD_ACCESS: return true;
        case AST_INDEX_ACCESS: return true;
        case AST_NEW: return true;
        case AST_TYPE: return false;
    }
}

void ast_setType(Ast ast, NodeId id, NodeId type) {
    AstNode* node = ast_getNode(ast, id);
    switch (node->kind) {
        case AST_BLOCK: unreachable();
        case AST_FUNCTION_ARGUMENT: unreachable();
        case AST_FUNCTION: unreachable();
        case AST_INTERFACE: unreachable();
        case AST_BUILTIN: unreachable();
        case AST_EXTERN: unreachable();
        case AST_TYPE_DEFINITION: unreachable();
        case AST_CASE_DEFINITION: unreachable();
        case AST_DECLARATION: unreachable();
        case AST_ASSIGNMENT: unreachable();
        case AST_RETURN: unreachable();
        case AST_BREAK: unreachable();
        case AST_CONTINUE: unreachable();
        case AST_IF_ELSE: unreachable();
        case AST_WHILE: unreachable();
        case AST_IMPORT: unreachable();
        case AST_LINK_WITH: unreachable();
        case AST_CALL_ARGUMENT: unreachable();
        case AST_CALL: node->call.type = type; break;
        case AST_BINARY: node->binary.type = type; break;
        case AST_UNARY: node->unary.type = type; break;
        case AST_FLOAT: node->floatNode.type = type; break;
        case AST_INTEGER: node->integer.type = type; break;
        case AST_IDENTIFIER: node->identifier.type = type; break;
        case AST_BOOLEAN: node->boolean.type = type; break;
        case AST_STRING: node->string.type = type; break;
        case AST_INTERPOLATED_STRING:
            node->interpolatedString.type = type;
            break;
        case AST_ARRAY: node->arrayNode.type = type; break;
        case AST_STRUCT_FIELD: unreachable();
        case AST_STRUCT_LITERAL: node->structLiteral.type = type; break;
        case AST_IF_ELSE_EXPR: node->ifElseExpr.type = type; break;
        case AST_FIELD_ACCESS: node->fieldAccess.type = type; break;
        case AST_INDEX_ACCESS: node->indexAccess.type = type; break;
        case AST_NEW: node->newNode.type = type; break;
        case AST_TYPE: unreachable();
    }
}

void ast_print(Ast ast) {
    printf("program\n");
    BoolStack isLast = Stack();
    ast_printNode(ast, 0, isLast);
}

static void printIndentation(BoolStack isLast) {
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

static void printTypeParameter(Ast ast, NodeId id) {
    AstNode node = *ast_getNode(ast, id);
    assert(node.kind == AST_IDENTIFIER);
    printf("%s", node.identifier.value.literal.content);
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
            totalNodes -= node.block.statements.count;
            for (size_t i = 0; i < node.block.definitions.count; i++) {
                stack_push(isLast, i + 1 == totalNodes);
                ast_printNode(
                    ast,
                    node.block.definitions.items[i],
                    isLast

                );
                stack_pop(isLast);
            }
            totalNodes -= node.block.definitions.count;
            break;
        }
        case AST_FUNCTION_ARGUMENT: todo(); break;
        case AST_FUNCTION:
            printIndentation(isLast);
            printf(
                "function %s",
                ast_getNode(ast, node.function.identifier)
                    ->identifier.value.literal.content
            );
            if (node.function.typeParameters.count != 0) {
                printf("[");
                for (size_t i = 0; i < node.function.typeParameters.count;
                     i++) {
                    printTypeParameter(
                        ast,
                        node.function.typeParameters.items[i]
                    );
                    if (i + 1 != node.function.typeParameters.count)
                        printf(", ");
                }
                printf("]");
            }
            printf("(");
            for (size_t i = 0; i < node.function.arguments.count; i++) {
                AstNode* arg =
                    ast_getNode(ast, node.function.arguments.items[i]);
                if (arg->functionArgument.isMutable) {
                    printf("mut ");
                }
                printf(
                    "%s",
                    ast_getNode(ast, arg->functionArgument.identifier)
                        ->identifier.value.literal.content
                );
                if (hasValue(arg->functionArgument.type)) {
                    printf(": ");
                    ast_printNode(ast, arg->functionArgument.type, isLast);
                }
                if (i + 1 != node.function.arguments.count) printf(", ");
            }
            printf(")");
            if (hasValue(node.function.type)) {
                printf(": ");
                ast_printNode(ast, node.function.type, isLast);
            }
            printf("\n");
            ast_printNode(ast, node.function.body, isLast);
            break;
        case AST_INTERFACE: todo(); break;
        case AST_BUILTIN: todo(); break;
        case AST_EXTERN: todo(); break;
        case AST_TYPE_DEFINITION:
            printIndentation(isLast);
            printf(
                "type %s\n",
                ast_getNode(ast, node.function.identifier)
                    ->identifier.value.literal.content
            );

            size_t totalChilds = node.typeDefinition.fields.count +
                                 node.typeDefinition.cases.count;
            for (size_t i = 0; i < node.typeDefinition.fields.count; i++) {
                stack_push(isLast, i + 1 == totalChilds);
                ast_printNode(ast, node.typeDefinition.fields.items[i], isLast);
                stack_pop(isLast);
            }

            totalChilds -= node.typeDefinition.fields.count;
            for (size_t i = 0; i < node.typeDefinition.cases.count; i++) {
                stack_push(isLast, i + 1 == totalChilds);
                ast_printNode(ast, node.typeDefinition.cases.items[i], isLast);
                stack_pop(isLast);
            }

            break;
        case AST_CASE_DEFINITION: todo(); break;
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
            if (ast_getNode(ast, node.assignment.assignable)->kind ==
                AST_IDENTIFIER)
                printf(":=\n");
            else printf("=\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.assignment.assignable, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.assignment.expression, isLast);
            stack_pop(isLast);
            break;
        }
        case AST_RETURN:
            printIndentation(isLast);
            printf("return\n");
            if (hasValue(node.returnNode.expression)) {
                stack_push(isLast, true);
                ast_printNode(ast, node.returnNode.expression, isLast);
                stack_pop(isLast);
            }
            break;
        case AST_BREAK:
            printIndentation(isLast);
            printf("break\n");
            break;
        case AST_CONTINUE:
            printIndentation(isLast);
            printf("continue\n");
            break;
        case AST_IF_ELSE: {
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
            printf("()");
            if (hasValue(node.call.type)) {
                printf(": ");
                ast_printNode(ast, node.call.type, isLast);
            }
            printf("\n");
            stack_push(isLast, node.call.arguments.count == 0);
            ast_printNode(ast, node.call.called, isLast);
            stack_pop(isLast);
            for (size_t i = 0; i < node.call.arguments.count; i++) {
                stack_push(isLast, i + 1 == node.call.arguments.count);
                ast_printNode(ast, node.call.arguments.items[i], isLast);
                stack_pop(isLast);
            }
            break;
        case AST_BINARY:
            printIndentation(isLast);
            printf("%s", token_getLiteral(node.binary.operator));
            if (hasValue(node.binary.type)) {
                printf(": ");
                ast_printNode(ast, node.binary.type, isLast);
            }
            printf("\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.binary.left, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.binary.right, isLast);
            stack_pop(isLast);
            break;
        case AST_UNARY:
            printIndentation(isLast);
            printf("%s", token_getLiteral(node.unary.operator));
            if (hasValue(node.unary.type)) {
                printf(": ");
                ast_printNode(ast, node.unary.type, isLast);
            }
            printf("\n");
            stack_push(isLast, true);
            ast_printNode(ast, node.unary.expression, isLast);
            stack_pop(isLast);
            break;
        case AST_FLOAT:
            printIndentation(isLast);
            printf("%s\n", node.floatNode.value.literal.content);
            break;
        case AST_INTEGER:
            printIndentation(isLast);
            printf("%s\n", node.integer.value.literal.content);
            break;
        case AST_IDENTIFIER:
            printIndentation(isLast);
            printf("%s", token_getLiteral(node.identifier.value));
            if (hasValue(node.identifier.type)) {
                printf(": ");
                ast_printNode(ast, node.identifier.type, isLast);
            }
            printf("\n");
            break;
        case AST_BOOLEAN:
            printIndentation(isLast);
            printf("%s\n", node.boolean.value.kind == TRUE ? "true" : "false");
            break;
        case AST_STRING:
            printIndentation(isLast);
            printf("\"%s\"\n", node.string.value.literal.content);
            break;
        case AST_INTERPOLATED_STRING:
            printIndentation(isLast);
            printf("\"\"\n");
            if (hasValue(node.interpolatedString.type)) {
                printf(": ");
                ast_printNode(ast, node.interpolatedString.type, isLast);
            }
            for (size_t i = 0; i < node.interpolatedString.strings.count; i++) {
                bool last = i + 1 == node.interpolatedString.strings.count;
                stack_push(isLast, last);
                printIndentation(isLast);
                printf("\"");
                if (node.interpolatedString.strings.items[i].literal.content)
                    printf(
                        "%s",
                        node.interpolatedString.strings.items[i].literal.content
                    );
                printf("\"\n");
                stack_pop(isLast);

                if (!last) {
                    stack_push(isLast, false);
                    ast_printNode(
                        ast,
                        node.interpolatedString.expressions.items[i],
                        isLast
                    );
                    stack_pop(isLast);
                }
            }
            break;
        case AST_ARRAY:
            printIndentation(isLast);
            printf("[]");
            if (hasValue(node.arrayNode.type)) {
                printf(": ");
                ast_printNode(ast, node.arrayNode.type, isLast);
            }
            printf("\n");
            for (size_t i = 0; i < node.arrayNode.elements.count; i++) {
                stack_push(isLast, i + 1 == node.arrayNode.elements.count);
                ast_printNode(ast, node.arrayNode.elements.items[i], isLast);
                stack_pop(isLast);
            }
            break;
        case AST_STRUCT_FIELD:
            printIndentation(isLast);
            printf(
                "%s:\n",
                ast_getNode(ast, node.structField.identifier)
                    ->identifier.value.literal.content
            );
            stack_push(isLast, true);
            ast_printNode(ast, node.structField.expression, isLast);
            stack_pop(isLast);
            break;
        case AST_STRUCT_LITERAL: {
            printIndentation(isLast);
            printf(
                "%s",
                ast_getNode(ast, node.structLiteral.identifier)
                    ->identifier.value.literal.content
            );
            if (hasValue(node.structLiteral.type)) {
                printf(": ");
                ast_printNode(ast, node.structLiteral.type, isLast);
            }
            printf("\n");
            for (size_t i = 0; i < node.structLiteral.fields.count; i++) {
                stack_push(isLast, i + 1 == node.structLiteral.fields.count);
                ast_printNode(ast, node.structLiteral.fields.items[i], isLast);
                stack_pop(isLast);
            }
            break;
        }
        case AST_IF_ELSE_EXPR: {
            bool lastNode = isLast.items[isLast.count - 1];
            if (lastNode == true) {
                stack_pop(isLast);
                stack_push(isLast, false);
            }
            printIndentation(isLast);
            printf("if");
            if (hasValue(node.ifElseExpr.type)) {
                printf(": ");
                ast_printNode(ast, node.ifElseExpr.type, isLast);
            }
            printf("\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.ifElseExpr.condition, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.ifElseExpr.ifNode, isLast);
            stack_pop(isLast);
            if (hasValue(node.ifElseExpr.elseNode)) {
                stack_pop(isLast);
                stack_push(isLast, lastNode);
                printIndentation(isLast);
                printf("else\n");
                stack_push(isLast, true);
                ast_printNode(ast, node.ifElseExpr.elseNode, isLast);
                stack_pop(isLast);
            }
            break;
        }
        case AST_FIELD_ACCESS:
            printIndentation(isLast);
            printf(
                ".%s",
                ast_getNode(ast, node.fieldAccess.identifier)
                    ->identifier.value.literal.content
            );
            if (hasValue(node.fieldAccess.type)) {
                printf(": ");
                ast_printNode(ast, node.fieldAccess.type, isLast);
            }
            printf("\n");
            stack_push(isLast, true);
            ast_printNode(ast, node.fieldAccess.accessed, isLast);
            stack_pop(isLast);
            break;
        case AST_INDEX_ACCESS:
            printIndentation(isLast);
            printf("[]");
            if (hasValue(node.indexAccess.type)) {
                printf(": ");
                ast_printNode(ast, node.indexAccess.type, isLast);
            }
            printf("\n");
            stack_push(isLast, false);
            ast_printNode(ast, node.indexAccess.accessed, isLast);
            stack_pop(isLast);
            stack_push(isLast, true);
            ast_printNode(ast, node.indexAccess.index, isLast);
            stack_pop(isLast);
            break;
        case AST_NEW: todo(); break;
        case AST_TYPE:
            printf("%s", token_getLiteral(node.type.token));
            if (node.type.parameters.count > 0) {
                printf("[");
                for (size_t i = 0; i < node.type.parameters.count; i++) {
                    ast_printNode(ast, node.type.parameters.items[i], isLast);
                }
                printf("]");
            }
            break;
    }
}