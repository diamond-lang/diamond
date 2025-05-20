#include "semantic.h"

#include <stdint.h>

#include "ast.h"
#include "common.h"
#include "scopes.h"

// bool analyzeProgram(Ast* ast);
// NodeId analyzeStatement(Ast* ast, NodeId);
// NodeId analyzeExpression(Ast* ast, NodeId node);

// typedef struct {
//     Ast* ast;
//     Scopes scopes;
// } Analyzer;

// static bool program(Analyzer* analyzer, NodeId id);
// static bool include(Analyzer* analyzer, NodeId id);
// static bool use(Analyzer* analyzer, NodeId id);
// static bool function(Analyzer* analyzer, NodeId id);
// static bool interface(Analyzer* analyzer, NodeId id);
// static bool externDefintion(Analyzer* analyzer, NodeId id);
// static bool typeDefinition(Analyzer* analyzer, NodeId id);
// static bool block(Analyzer* analyzer, NodeId id);
// static bool declaration(Analyzer* analyzer, NodeId id);

// bool analyze(Ast* ast) {
//     Analyzer analyzer;

//     for (NodeCount i = 0; i <= ast->nodes.count; i++) {
//         AstKind kind = ast->nodes.items[i];
//         switch (kind) {
//             case AST_INCLUDE: todo();
//             case AST_USE: todo();
//             case AST_FUNCTION: todo();
//             case AST_INTERFACE: todo();
//             case AST_EXTERN: todo();
//             case AST_TYPE_DEFINITION: todo();
//             case AST_BLOCK: todo();
//             case AST_DECLARATION: return declaration(&analyzer, i);
//             case AST_ASSIGNMENT: todo();
//             case AST_RETURN: todo();
//             case AST_RETURN_WITH_EXPRESSION: todo();
//             case AST_BREAK: todo();
//             case AST_CONTINUE: todo();
//             case AST_IF_ELSE: todo();
//             case AST_WHILE: todo();
//             case AST_CALL: todo();
//             case AST_IF_ELSE_EXPRESSION: todo();
//             case AST_NOT: todo();
//             case AST_OR: todo();
//             case AST_AND: todo();
//             case AST_EQUAL_EQUAL: todo();
//             case AST_NOT_EQUAL: todo();
//             case AST_LESS: todo();
//             case AST_LESS_EQUAL: todo();
//             case AST_GREATER: todo();
//             case AST_GREATER_EQUAL: todo();
//             case AST_ADD: todo();
//             case AST_SUBTRACT: todo();
//             case AST_MUL: todo();
//             case AST_DIV: todo();
//             case AST_MOD: todo();
//             case AST_NEGATION: todo();
//             case AST_DEREFERENCE: todo();
//             case AST_ADDRESS_OF: todo();
//             case AST_FIELD_ACCESS: todo();
//             case AST_INDEX_ACCESS: todo();
//             case AST_FLOAT: todo();
//             case AST_INTEGER: todo();
//             case AST_IDENTIFIER:
//                 if (ast->nodes.items[i + 1] == AST_DECLARATION) continue;
//                 if (ast->nodes.items[i + 1] == AST_ASSIGNMENT) continue;
//                 break;
//             case AST_BOOLEAN: todo();
//             case AST_STRING: todo();
//             case AST_ARRAY: todo();
//             case AST_STRUCT_LITERAL: todo();
//             case AST_TYPE: todo();
//         }
//     }
//     return false;
// }
