#include "semantic.h"

// #include <stddef.h>
// #include <stdint.h>

// #include "ast.h"
// #include "common.h"
// #include "scopes.h"
// #include "types.h"

// // NodeId analyzeStatement(Ast* ast, NodeId);
// // NodeId analyzeExpression(Ast* ast, NodeId node);

// typedef struct {
//     Ast* ast;
//     Uint32ListStack scopes;
// } Context;

// bool analyzeFunction(Context* context, uint32_t function);
// bool analyzeTypeDefinition(Context* context, uint32_t type);
// bool analyzeStatement(Context* context, uint32_t statement);
// bool analyzeExpression(
//     Context* context, uint32_t expression, uint32_t lastNode
// );

// bool analyze(Ast* ast) {
//     // Initialize context
//     Context context;
//     context.ast = ast;
//     context.scopes = (Uint32ListStack){};

//     // Analyze definitions
//     for (size_t i = 0; i < list_size(ast->definitions); i++) {
//         uint32_t node = *list_get(ast->definitions, i);
//         uint32_t kind = *list_get(ast->nodes, node);
//         if (kind == AST_FUNCTION) {
//             bool result = analyzeFunction(&context, node);
//             if (!result) return false;
//         } else if (kind == AST_TYPE_DEFINITION) {
//             bool result = analyzeTypeDefinition(&context, node);
//             if (!result) return false;
//         } else {
//             unreachable();
//         }
//     }

//     // Analyze code
//     for (uint32_t node = 0; node <= list_size(ast->nodes); node++) {
//         AstKind kind = *list_get(ast->nodes, node);
//         if (kind == AST_IMPORT) {
//             continue;
//         } else if (kind == AST_FUNCTION_ARGUMENT) {
//             unreachable();
//         } else if (kind == AST_FUNCTION) {
//             node = ast_getData(AstFunction, context.ast, node)->lastNode;
//             continue;
//         } else if (kind == AST_INTERFACE) {
//             todo();
//         } else if (kind == AST_EXTERN) {
//             todo();
//         } else if (kind == AST_TYPE_DEFINITION) {
//             node = ast_getData(AstTypeDefinition, context.ast, node)->lastNode;
//             continue;
//         }

//         bool result = analyzeStatement(&context, node);
//         if (!result) return result;
//     }
//     return true;
// }

// bool analyzeFunction(Context* context, uint32_t function) {
//     //AstFunction data = ast_getData(AstFunction, context->ast, function);
//     return true;
// }

// bool analyzeStatement(Context* context, uint32_t statement) {
//     AstKind kind = *list_get(context->ast->nodes, statement);
//     switch (kind) {
//         case AST_IMPORT: unreachable();
//         case AST_FUNCTION_ARGUMENT: unreachable();
//         case AST_FUNCTION: unreachable();
//         case AST_INTERFACE: unreachable();
//         case AST_EXTERN: unreachable();
//         case AST_TYPE_DEFINITION: unreachable();
//         case AST_DECLARATION: {
//             AstDeclaration* data =
//                 ast_getData(AstDeclaration, context->ast, statement);
//             analyzeExpression(context, ) break;
//         }
//         case AST_ASSIGNMENT: todo();
//         case AST_RETURN: todo();
//         case AST_RETURN_WITH_EXPRESSION: todo();
//         case AST_BREAK: todo();
//         case AST_CONTINUE: todo();
//         case AST_IF_ELSE: todo();
//         case AST_WHILE: todo();
//         case AST_CALL: todo();
//         case AST_IF_ELSE_EXPRESSION: todo();
//         case AST_NOT: todo();
//         case AST_OR: todo();
//         case AST_AND: todo();
//         case AST_EQUAL_EQUAL: todo();
//         case AST_NOT_EQUAL: todo();
//         case AST_LESS: todo();
//         case AST_LESS_EQUAL: todo();
//         case AST_GREATER: todo();
//         case AST_GREATER_EQUAL: todo();
//         case AST_ADD: todo();
//         case AST_SUBTRACT: todo();
//         case AST_MUL: todo();
//         case AST_DIV: todo();
//         case AST_MOD: todo();
//         case AST_NEGATION: todo();
//         case AST_DEREFERENCE: todo();
//         case AST_ADDRESS_OF: todo();
//         case AST_FIELD_ACCESS: todo();
//         case AST_INDEX_ACCESS: todo();
//         case AST_FLOAT: todo();
//         case AST_INTEGER: todo();
//         case AST_IDENTIFIER: todo();
//         case AST_BOOLEAN: todo();
//         case AST_STRING: todo();
//         case AST_ARRAY: todo();
//         case AST_STRUCT_LITERAL: todo();
//         case AST_TYPE: todo();
//     }
//     return true;
// }

// bool analyzeExpression(
//     Context* context, uint32_t expression, uint32_t lastNode
// ) {}