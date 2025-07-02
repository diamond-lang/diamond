#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "common.h"
#include "types.h"

void ast_init(Ast* ast, String canonicalPath) {
    ast->canonicalPath = canonicalPath;
    ast->nodes = (Uint8List)List();
    ast->dataOrIndex = (DataList)List();
    ast->data = (DataList)List();
    ast->errors = (ErrorList)List();
    ast->imports = (Imports)List();
}

void ast_clear(Ast* ast) {
    list_clear(ast->nodes);
    list_clear(ast->dataOrIndex);
    list_clear(ast->data);
    list_clear(ast->errors);
    list_clear(ast->imports);
}

NodeId ast_createNode(Ast* ast, AstKind kind) {
    list_append(ast->nodes, kind);
    list_append(ast->dataOrIndex, None());
    NodeId id = list_size(ast->nodes) - 1;
    _ast_getData(ast, id);  // Assure data for the node is added
    switch (kind) {
        case AST_IMPORT: break;
        case AST_FUNCTION_ARGUMENT: break;
        case AST_FUNCTION: break;
        case AST_INTERFACE: break;
        case AST_EXTERN: break;
        case AST_TYPE_DEFINITION: break;
        case AST_DECLARATION: {
            AstDeclaration* data = ast_getData(AstDeclaration, ast, id);
            data->lastTypeNode = None();
            break;
        }
        case AST_ASSIGNMENT: {
            AstAssignment* data = ast_getData(AstAssignment, ast, id);
            data->lastTypeNode = None();
            break;
        }
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
        case AST_CALL: break;
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
        case AST_FLOAT: break;
        case AST_INTEGER: break;
        case AST_IDENTIFIER: break;
        case AST_BOOLEAN: break;
        case AST_STRING: break;
        case AST_ARRAY: break;
        case AST_STRUCT_LITERAL: break;
        case AST_TYPE: break;
    }
    return id;
}

NodeId ast_insertNode(
    Ast* ast, AstKind kind, NodeId location, DataId dataLocation
) {
    size_t numberOfSlotsUsed = ast_numberOfSlotsUsedInData(kind);

    // Make space for insertation
    list_ensureExtraCapacity(ast->nodes, 1);
    list_ensureExtraCapacity(ast->dataOrIndex, 1);
    ast->nodes.size += 1;
    ast->dataOrIndex.size += 1;
    for (size_t i = list_size(ast->nodes) - 2;
         location <= i && i <= list_size(ast->nodes) - 2;
         i--) {
        *list_get(ast->nodes, i + 1) = *list_get(ast->nodes, i);
        *list_get(ast->dataOrIndex, i + 1) = *list_get(ast->dataOrIndex, i);
        size_t slotsUsed =
            ast_numberOfSlotsUsedInData(*list_get(ast->nodes, i));
        if (slotsUsed > 1) {
            *list_get(ast->dataOrIndex, i + 1) += numberOfSlotsUsed;
        }
    }
    list_ensureExtraCapacity(ast->data, numberOfSlotsUsed);
    ast->data.size += numberOfSlotsUsed;
    for (size_t i = list_size(ast->data) - 1 - numberOfSlotsUsed;
         dataLocation <= i && i <= list_size(ast->data) - 1 - numberOfSlotsUsed;
         i--) {
        *list_get(ast->data, i + numberOfSlotsUsed) = *list_get(ast->data, i);
    }

    // Remember sizes
    size_t actualSize = ast->nodes.size;
    size_t actualSizeData = ast->data.size;

    // Set size
    ast->nodes.size = location;
    ast->dataOrIndex.size = location;
    ast->data.size = dataLocation;

    // Add node
    NodeId node = ast_createNode(ast, kind);

    // Restore size;
    ast->nodes.size = actualSize;
    ast->dataOrIndex.size = actualSize;
    ast->data.size = actualSizeData;

    // Returm
    return node;
}

size_t ast_numberOfSlotsUsedInData(AstKind kind) {
    size_t result;
    switch (kind) {
        case AST_IMPORT: result = sizeof(AstImport); break;
        case AST_FUNCTION_ARGUMENT: result = sizeof(AstFunctionArgument); break;
        case AST_FUNCTION: result = sizeof(AstFunction); break;
        case AST_INTERFACE: result = sizeof(AstInterface); break;
        case AST_EXTERN: result = sizeof(AstExtern); break;
        case AST_TYPE_DEFINITION: result = sizeof(AstTypeDefinition); break;
        case AST_DECLARATION: result = sizeof(AstDeclaration); break;
        case AST_ASSIGNMENT: result = sizeof(AstAssignment); break;
        case AST_RETURN: result = sizeof(AstReturn); break;
        case AST_RETURN_WITH_EXPRESSION:
            result = sizeof(AstReturnWithExpression);
            break;
        case AST_BREAK: result = sizeof(AstBreak); break;
        case AST_CONTINUE: result = sizeof(AstContinue); break;
        case AST_IF_ELSE: result = sizeof(AstIfElse); break;
        case AST_WHILE: result = sizeof(AstWhile); break;
        case AST_CALL: result = sizeof(AstCall); break;
        case AST_IF_ELSE_EXPRESSION:
            result = sizeof(AstIfElseExpression);
            break;
        case AST_NOT: result = sizeof(AstNot); break;
        case AST_OR: result = sizeof(AstOr); break;
        case AST_AND: result = sizeof(AstAnd); break;
        case AST_EQUAL_EQUAL: result = sizeof(AstEqualEqual); break;
        case AST_NOT_EQUAL: result = sizeof(AstNotEqual); break;
        case AST_LESS: result = sizeof(AstLess); break;
        case AST_LESS_EQUAL: result = sizeof(AstLessEqual); break;
        case AST_GREATER: result = sizeof(AstGreater); break;
        case AST_GREATER_EQUAL: result = sizeof(AstGreaterEqual); break;
        case AST_ADD: result = sizeof(AstAdd); break;
        case AST_SUBTRACT: result = sizeof(AstSubtract); break;
        case AST_MUL: result = sizeof(AstMul); break;
        case AST_DIV: result = sizeof(AstDiv); break;
        case AST_MOD: result = sizeof(AstMod); break;
        case AST_NEGATION: result = sizeof(AstNegation); break;
        case AST_DEREFERENCE: result = sizeof(AstDereference); break;
        case AST_ADDRESS_OF: result = sizeof(AstAddressOf); break;
        case AST_FIELD_ACCESS: result = sizeof(AstFieldAccess); break;
        case AST_INDEX_ACCESS: result = sizeof(AstIndexAccess); break;
        case AST_FLOAT: result = sizeof(AstFloat); break;
        case AST_INTEGER: result = sizeof(AstInteger); break;
        case AST_IDENTIFIER: result = sizeof(AstIdentifier); break;
        case AST_BOOLEAN: result = sizeof(AstBoolean); break;
        case AST_STRING: result = sizeof(AstString); break;
        case AST_ARRAY: result = sizeof(AstArray); break;
        case AST_STRUCT_LITERAL: result = sizeof(AstStructLiteral); break;
        case AST_TYPE: result = sizeof(AstType); break;
    }
    return result / sizeof(Data);
}

Data* _ast_getData(Ast* ast, NodeId node) {
    size_t numberOfSlotsUsed =
        ast_numberOfSlotsUsedInData(*list_get(ast->nodes, node));
    if (numberOfSlotsUsed == 0) return NULL;
    if (numberOfSlotsUsed == 1) {
        return list_get(ast->dataOrIndex, node);
    } else {
        if (*list_get(ast->dataOrIndex, node) == None()) {
            *list_get(ast->dataOrIndex, node) = list_size(ast->data);
            list_ensureExtraCapacity(ast->data, numberOfSlotsUsed);
            list_setSize(ast->data, list_size(ast->data) + numberOfSlotsUsed);
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
    arena_newLifetime();
    SizeTStack indentationLevel = Stack();
    SizeTStack separations = Stack();
    for (size_t i = 0; i < list_size(ast.nodes); i += 1) {
        AstKind kind = *list_get(ast.nodes, i);

        printf("%02zu│ ", i);

        for (size_t j = 0; j < stack_size(indentationLevel); j++) {
            printf("    ");
        }

        switch (kind) {
            case AST_IMPORT:
                printf(
                    "import %.*s\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstImport, &ast, i)->path
                    )
                );
                break;
            case AST_FUNCTION_ARGUMENT:
                printf(
                    "argument(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstFunctionArgument, &ast, i)->literal
                    )
                );
                break;
            case AST_FUNCTION:
                printf(
                    "function %.*s\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstFunction, &ast, i)->literal
                    )
                );
                stack_push(
                    indentationLevel,
                    ast_getData(AstFunction, &ast, i)->lastNode
                );
                break;
            case AST_INTERFACE: printf("interface\n"); break;
            case AST_EXTERN: printf("extern\n"); break;
            case AST_TYPE_DEFINITION:
                printf(
                    "type %.*s\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstTypeDefinition, &ast, i)->literal
                    )
                );
                stack_push(
                    indentationLevel,
                    ast_getData(AstTypeDefinition, &ast, i)->lastNode
                );
                break;
            case AST_DECLARATION: {
                printf("declaration\n");
                stack_push(separations, i + 1);
                if (ast_getData(AstDeclaration, &ast, i)->lastTypeNode !=
                    None()) {
                    stack_push(
                        separations,
                        ast_getData(AstDeclaration, &ast, i)->lastExpressionNode
                    );
                    stack_push(
                        indentationLevel,
                        ast_getData(AstDeclaration, &ast, i)->lastTypeNode
                    );
                } else {
                    stack_push(
                        indentationLevel,
                        ast_getData(AstDeclaration, &ast, i)->lastExpressionNode
                    );
                }
                break;
            }
            case AST_ASSIGNMENT: {
                printf("assignment\n");
                if (ast_getData(AstAssignment, &ast, i)->lastTypeNode !=
                    None()) {
                    stack_push(
                        separations,
                        ast_getData(AstAssignment, &ast, i)->lastExpressionNode
                    );
                    stack_push(
                        indentationLevel,
                        ast_getData(AstAssignment, &ast, i)->lastTypeNode
                    );
                } else {
                    stack_push(
                        indentationLevel,
                        ast_getData(AstAssignment, &ast, i)->lastExpressionNode
                    );
                }
                stack_push(
                    separations,
                    ast_getData(AstAssignment, &ast, i)->lastAssignableNode
                );
                break;
            }
            case AST_RETURN: printf("return\n"); break;
            case AST_RETURN_WITH_EXPRESSION:
                printf("returnWithExpression\n");
                break;
            case AST_BREAK: printf("break\n"); break;
            case AST_CONTINUE: printf("continue\n"); break;
            case AST_IF_ELSE:
                stack_push(
                    separations,
                    ast_getData(AstIfElse, &ast, i)->lastIfNode
                );
                stack_push(
                    separations,
                    ast_getData(AstIfElse, &ast, i)->lastConditionNode
                );
                printf("ifElse\n");
                if (ast_getData(AstIfElse, &ast, i)->lastElseNode != None()) {
                    stack_push(
                        indentationLevel,
                        ast_getData(AstIfElse, &ast, i)->lastElseNode
                    );
                } else {
                    stack_push(
                        indentationLevel,
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
                    "float(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstFloat, &ast, i)->literal
                    )
                );
                break;
            case AST_INTEGER:
                printf(
                    "integer(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstInteger, &ast, i)->literal
                    )
                );
                break;
            case AST_IDENTIFIER:
                printf(
                    "identifier(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstIdentifier, &ast, i)->literal
                    )
                );
                break;
            case AST_BOOLEAN:
                printf(
                    "boolean(%s)\n",
                    ast_getData(AstBoolean, &ast, i)->value ? "true" : "false"
                );
                break;
            case AST_STRING:
                printf(
                    "string(\"%.*s\")\n",
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
                    "type(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstType, &ast, i)->literal
                    )
                );
                stack_push(
                    indentationLevel,
                    ast_getData(AstType, &ast, i)->lastNode
                );
                break;
        }

        while (stack_size(indentationLevel) != 0 &&
               i == *stack_top(indentationLevel)) {
            stack_pop(indentationLevel);
        }

        if (stack_size(separations) != 0 && *stack_top(separations) == i) {
            printf("  │");
            for (size_t j = 0; j < stack_size(indentationLevel); j++) {
                printf("    ");
            }
            printf(" ────────────\n");
            stack_pop(separations);
        }
    }
    arena_destroyCurrentLifetime();
}

void ast_setBit(Data* data, NodeCount position) {
    *data = *data | 1 << position;
}