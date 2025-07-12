#include "ast.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "common.h"
#include "types.h"

static uint32_t ast_numberOfSlotsUsedInData(AstInstructionKind kind);

static void ast_initCode(Code* code, uint32_t lifetime) {
    code->instructions = (Uint8List)ListWithLifetime(lifetime);
    code->dataOrIndex = (Uint32List)ListWithLifetime(lifetime);
    code->data = (Uint32List)ListWithLifetime(lifetime);
}

static void ast_clearCode(Code* code) {
    code->instructions = (Uint8List)List();
    code->dataOrIndex = (Uint32List)List();
    code->data = (Uint32List)List();
}

void ast_initFunction(Function* function, uint32_t lifetime) {
    function->arguments = (FunctionArgumentList)ListWithLifetime(lifetime);
    ast_initCode(&function->code, lifetime);
    function->returnType = None();
}

void ast_initTypeDefinition(TypeDefinition* typeDefinition, uint32_t lifetime) {
    typeDefinition->fields = (Uint32List)ListWithLifetime(lifetime);
    typeDefinition->fieldTypes = (Uint32List)ListWithLifetime(lifetime);
}

void ast_init(Ast* ast) {
    ast->importedAsts = (Uint32List)List();
    ast->imports = (ImportList)List();
    ast->functions = (FunctionList)List();
    ast->typeDefinitions = (TypeDefinitionList)List();
    ast_initCode(&ast->code, arena_currentLifetime());
    ast->literals = (Uint32List)List();
    ast->types = (TypeList)List();
    ast->errors = (ErrorList)List();
}

void ast_clear(Ast* ast) {
    list_clear(ast->importedAsts);
    list_clear(ast->imports);
    list_clear(ast->functions);
    list_clear(ast->typeDefinitions);
    ast_clearCode(&ast->code);
    list_clear(ast->literals);
    list_clear(ast->types);
    list_clear(ast->errors);
}

uint32_t ast_addInstruction(Code* code, AstInstructionKind kind) {
    list_append(code->instructions, kind);
    list_append(code->dataOrIndex, None());
    uint32_t id = list_size(code->instructions) - 1;
    (void)_ast_getData(code, id);  // Assure data for the instruction is added
    return id;
}

static uint32_t ast_numberOfSlotsUsedInData(AstInstructionKind kind) {
    uint32_t result;
    switch (kind) {
        case AST_DECLARATION: result = sizeof(AstDeclaration); break;
        case AST_ASSIGNMENT: result = sizeof(AstAssignment); break;
        case AST_RETURN: result = sizeof(AstReturn); break;
        case AST_RETURN_EXPRESSION: result = sizeof(AstReturnExpression); break;
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
    }
    return result / sizeof(uint32_t);
}

uint32_t* _ast_getData(Code* code, uint32_t instruction) {
    uint32_t numberOfSlotsUsed =
        ast_numberOfSlotsUsedInData(*list_get(code->instructions, instruction));
    if (numberOfSlotsUsed == 0) return NULL;
    if (numberOfSlotsUsed == 1) {
        return list_get(code->dataOrIndex, instruction);
    } else {
        if (*list_get(code->dataOrIndex, instruction) == None()) {
            *list_get(code->dataOrIndex, instruction) = list_size(code->data);
            list_ensureExtraCapacity(code->data, numberOfSlotsUsed);
            list_setSize(code->data, list_size(code->data) + numberOfSlotsUsed);
        }
        return list_get(code->data, *list_get(code->dataOrIndex, instruction));
    }
}

uint32_t ast_createType(Ast* ast, TypeKind kind) {
    list_append(ast->types, (Type){kind});
    switch (kind) {
        case TYPE_VARIABLE: break;
        case TYPE_APPLICATION:
            list_get(ast->types, list_size(ast->types) - 1)
                ->application.parameterCount = 0;
            break;
    }
    return list_size(ast->types) - 1;
}

uint32_t ast_createTypeVariable(Ast* ast) { todo(); }

uint32_t ast_createTypeApplication(Ast* ast) {
    list_append(ast->types, (Type){TYPE_APPLICATION});
    list_get(ast->types, list_size(ast->types) - 1)
        ->application.parameterCount = 0;
    return list_size(ast->types) - 1;
}

static inline uint32_t numberOfSlotsNeededForLiteral(uint32_t length) {
    return 1 + length / sizeof(uint32_t) + (length % sizeof(uint32_t) != 0);
}

uint32_t ast_getLiteral(Uint32List* literals, StringView view) {
    char* literal = view.pointer;
    uint32_t length = view.length;

    // Check if literal already exist
    uint32_t literalId = None();
    for (uint32_t i = 0; i < list_size(*literals);
         i += numberOfSlotsNeededForLiteral(*list_get(*literals, i))) {
        if (*list_get(*literals, i) == view.length &&
            memcmp(list_get(*literals, i + 1), literal, length) == 0) {
            literalId = i;
            break;
        }
    }

    // Add literal to literals
    if (literalId == None()) {
        literalId = list_size(*literals);

        uint32_t extraCapacityNeeded = numberOfSlotsNeededForLiteral(length);
        list_ensureExtraCapacity((*literals), extraCapacityNeeded);

        list_size(*literals) += extraCapacityNeeded;
        *list_get(*literals, literalId) = length;
        strncpy((char*)list_get(*literals, literalId + 1), literal, length);
    }

    return literalId;
}

static char* getBinaryOp(AstInstructionKind kind) {
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

void ast_setBit(uint32_t* data, uint32_t position) {
    *data = *data | 1 << position;
}

static void ast_printIndentation(uint32_t indentatioLevel) {
    for (uint32_t i = 0; i < indentatioLevel; i++) printf("   ");
}

static void ast_printType(Ast ast, uint32_t typeId) {
    Type type = *list_get(ast.types, typeId);
    switch (type.kind) {
        case TYPE_VARIABLE: printf("%d", type.variable.id); break;
        case TYPE_APPLICATION:
            printf("%.*s", ast_literalExpand(ast, type.application.identifier));
            if (type.application.parameterCount != 0) {
                printf("[");
                for (uint32_t i = 0; i < type.application.parameterCount; i++) {
                    ast_printType(ast, typeId + 1 + i);
                }
                printf("]");
            }
            break;
    }
}

static void ast_printCode(Ast ast, Code code, uint32_t indentationLevel) {
    arena_newLifetime();
    Uint32Stack indentation = Stack();
    Uint32Stack separations = Stack();

    for (uint32_t i = 0; i < list_size(code.instructions); i += 1) {
        ast_printIndentation(indentationLevel + stack_size(indentation));
        AstInstructionKind kind = *list_get(code.instructions, i);
        switch (kind) {
            case AST_DECLARATION: {
                AstDeclaration data = *ast_getData(AstDeclaration, &code, i);
                if (data.type != None()) {
                    printf(
                        "declaration(%.*s, expectedType: ",
                        ast_literalExpand(ast, data.identifier)
                    );
                    ast_printType(ast, data.type);
                    printf(", mutable: %s)\n", data.mutable ? "true" : "false");
                } else {
                    printf(
                        "declaration(%.*s, mutable: %s)\n",
                        ast_literalExpand(ast, data.identifier),
                        data.mutable ? "true" : "false"
                    );
                }
                break;
            }
            case AST_ASSIGNMENT: printf("assignment\n"); break;
            case AST_RETURN: printf("return\n"); break;
            case AST_RETURN_EXPRESSION: printf("returnExpression\n"); break;
            case AST_BREAK: printf("break\n"); break;
            case AST_CONTINUE: printf("continue\n"); break;
            case AST_IF_ELSE:
                printf("ifElse\n");
                if (ast_getData(AstIfElse, &code, i)->elseBlockEnd != None()) {
                    stack_push(
                        separations,
                        ast_getData(AstIfElse, &code, i)->ifBlockEnd
                    );
                    stack_push(
                        indentation,
                        ast_getData(AstIfElse, &code, i)->elseBlockEnd
                    );
                } else {
                    stack_push(
                        indentation,
                        ast_getData(AstIfElse, &code, i)->ifBlockEnd
                    );
                }
                break;
            case AST_WHILE:
                printf("while\n");
                stack_push(
                    indentation,
                    ast_getData(AstWhile, &code, i)->whileEnd
                );
                break;
            case AST_CALL:
                printf(
                    "call(arguments: %d)\n",
                    ast_getData(AstCall, &code, i)->argumentsCount
                );
                break;
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
                        ast_getData(AstFloat, &code, i)->literal
                    )
                );
                break;
            case AST_INTEGER:
                printf(
                    "integer(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstInteger, &code, i)->literal
                    )
                );
                break;
            case AST_IDENTIFIER:
                printf(
                    "identifier(%.*s)\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstIdentifier, &code, i)->literal
                    )
                );
                break;
            case AST_BOOLEAN:
                printf(
                    "boolean(%s)\n",
                    ast_getData(AstBoolean, &code, i)->value ? "true" : "false"
                );
                break;
            case AST_STRING:
                printf(
                    "string(\"%.*s\")\n",
                    ast_literalExpand(
                        ast,
                        ast_getData(AstString, &code, i)->literal
                    )
                );
                break;
            case AST_ARRAY: printf("arrayLiteral\n"); break;
            case AST_STRUCT_LITERAL: printf("structLiteral\n"); break;
        }

        while (stack_size(indentation) != 0 && i == *stack_top(indentation)) {
            stack_pop(indentation);
        }

        if (stack_size(separations) != 0 && *stack_top(separations) == i) {
            ast_printIndentation(indentationLevel + stack_size(indentation));
            printf("─────────────────\n");
            stack_pop(separations);
        }
    }

    arena_destroyCurrentLifetime();
}

static void ast_printTypeDefinition(
    Ast ast, TypeDefinition typeDefinition, uint32_t indentationLevel
) {
    ast_printIndentation(indentationLevel);
    printf("type %.*s\n", ast_literalExpand(ast, typeDefinition.identifier));
    assert(
        list_size(typeDefinition.fields) == list_size(typeDefinition.fieldTypes)
    );
    for (uint32_t i = 0; i < list_size(typeDefinition.fields); i++) {
        ast_printIndentation(indentationLevel + 1);
        printf(
            "%.*s: ",
            ast_literalExpand(ast, *list_get(typeDefinition.fields, i))
        );
        ast_printType(ast, *list_get(typeDefinition.fieldTypes, i));
        printf("\n");
    }
}

static void ast_printFunction(
    Ast ast, Function function, uint32_t indentationLevel
) {
    ast_printIndentation(indentationLevel);
    printf("function %.*s", ast_literalExpand(ast, function.identifier));
    printf("(");
    for (uint32_t i = 0; i < list_size(function.arguments); i++) {
        FunctionArgument argument = *list_get(function.arguments, i);
        if (argument.mutable) {
            printf("mut ");
        }
        printf("%.*s", ast_literalExpand(ast, argument.identifier));
        if (argument.type != None()) {
            printf(": ");
            ast_printType(ast, argument.type);
        }
        if (i + 1 != list_size(function.arguments)) {
            printf(", ");
        }
    }
    printf(")");
    if (function.returnType != None()) {
        printf(": ");
        ast_printType(ast, function.returnType);
    }
    printf("\n");
    ast_printCode(ast, function.code, indentationLevel + 1);
}

void ast_print(Ast ast, String path) {
    printf("%s\n", string_asCString(path));
    for (uint32_t i = 0; i < list_size(ast.imports); i++) {
        ast_printIndentation(1);
        printf(
            "import %.*s\n",
            ast_literalExpand(ast, list_get(ast.imports, i)->path)
        );
    }
    ast_printCode(ast, ast.code, 1);
    for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
        ast_printTypeDefinition(ast, *list_get(ast.typeDefinitions, i), 1);
    }
    for (uint32_t i = 0; i < list_size(ast.functions); i++) {
        ast_printFunction(ast, *list_get(ast.functions, i), 1);
    }
}