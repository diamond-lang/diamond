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
    function->type = None();
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
    ast->types.types = (TypeList)List();
    ast->types.parameters = (Uint32List)List();
    ast->errors = (ErrorList)List();
}

void ast_clear(Ast* ast) {
    list_clear(ast->importedAsts);
    list_clear(ast->imports);
    list_clear(ast->functions);
    list_clear(ast->typeDefinitions);
    ast_clearCode(&ast->code);
    list_clear(ast->literals);
    list_clear(ast->types.types);
    list_clear(ast->types.parameters);
    list_clear(ast->errors);
}

uint32_t ast_addInstruction(Code* code, AstInstructionKind kind) {
    list_append(code->instructions, kind);
    list_append(code->dataOrIndex, None());
    uint32_t id = list_size(code->instructions) - 1;
    (void)ast_getData(code, id);  // Assure data for the instruction is added
    switch (kind) {
    case AST_DECLARATION: {
        AstDeclaration* data = ast_getData(code, id);
        data->mutable = false;
        data->identifier = None();
        data->type = None();
        break;
    }
    case AST_ASSIGNMENT: {
        AstAssignment* data = ast_getData(code, id);
        data->nonlocal = false;
        data->type = None();
        break;
    }
    case AST_RETURN: {
        break;
    }
    case AST_RETURN_EXPRESSION: {
        break;
    }
    case AST_BREAK: {
        break;
    }
    case AST_CONTINUE: {
        break;
    }
    case AST_IF_ELSE: {
        AstIfElse* data = ast_getData(code, id);
        data->ifBlockEnd = None();
        data->elseBlockEnd = None();
        break;
    }
    case AST_WHILE: {
        AstWhile* data = ast_getData(code, id);
        data->whileEnd = None();
        break;
    }
    case AST_CALL: {
        AstCall* data = ast_getData(code, id);
        data->argumentsCount = 0;
        data->argumentsMutability = 0;
        data->type = None();
        break;
    }
    case AST_IF_ELSE_EXPRESSION: {
        todo();
        break;
    }
    case AST_NOT: {
        todo();
        break;
    }
    case AST_OR: {
        todo();
        break;
    }
    case AST_AND: {
        todo();
        break;
    }
    case AST_EQUAL_EQUAL: {
        todo();
        break;
    }
    case AST_NOT_EQUAL: {
        todo();
        break;
    }
    case AST_LESS: {
        todo();
        break;
    }
    case AST_LESS_EQUAL: {
        todo();
        break;
    }
    case AST_GREATER: {
        todo();
        break;
    }
    case AST_GREATER_EQUAL: {
        todo();
        break;
    }
    case AST_ADD: {
        AstAdd* data = ast_getData(code, id);
        data->type = None();
        break;
    }
    case AST_SUBTRACT: {
        todo();
        break;
    }
    case AST_MUL: {
        break;
    }
    case AST_DIV: {
        todo();
        break;
    }
    case AST_MOD: {
        todo();
        break;
    }
    case AST_NEGATION: {
        break;
    }
    case AST_DEREFERENCE: {
        todo();
        break;
    }
    case AST_ADDRESS_OF: {
        todo();
        break;
    }
    case AST_FIELD_ACCESS: {
        todo();
        break;
    }
    case AST_INDEX_ACCESS: {
        todo();
        break;
    }
    case AST_FLOAT: {
        AstFloat* data = ast_getData(code, id);
        data->literal = None();
        data->type = None();
        break;
    }
    case AST_INTEGER: {
        AstInteger* data = ast_getData(code, id);
        data->literal = None();
        data->type = None();
        break;
    }
    case AST_IDENTIFIER: {
        AstIdentifier* data = ast_getData(code, id);
        data->literal = None();
        data->type = None();
        break;
    }
    case AST_BOOLEAN: {
        AstBoolean* data = ast_getData(code, id);
        data->value = false;
        data->type = None();
        break;
    }
    case AST_STRING: {
        AstString* data = ast_getData(code, id);
        data->literal = None();
        break;
    }
    case AST_ARRAY: {
        todo();
        break;
    }
    case AST_STRUCT_LITERAL: {
        todo();
        break;
    }
    }
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
    case AST_IF_ELSE_EXPRESSION: result = sizeof(AstIfElseExpression); break;
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

void* ast_getData(Code* code, uint32_t instruction) {
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

uint32_t ast_addTypeVariable(Types* types, uint32_t typeVariable) {
    uint32_t result = list_size(types->types);
    Type newType = {TYPE_VARIABLE, .variable = (TypeVariable){typeVariable}};
    list_append(types->types, newType);
    return result;
}

uint32_t ast_addTypeWithParams(
    Types* types, uint32_t literal, uint32_t parameterCount
) {
    uint32_t result = list_size(types->types);
    Type newType = {
        TYPE_WITH_PARAMS,
        .withParams = (TypeWithParams
        ){literal, parameterCount, list_size(types->parameters)}
    };
    list_append(types->types, newType);
    list_ensureExtraCapacity(types->parameters, parameterCount);
    list_setSize(
        types->parameters,
        list_size(types->parameters) + parameterCount
    );
    uint32_t* parameters = ast_getParameters(*types, result);
    for (uint32_t i = 0; i < parameterCount; i++) {
        parameters[i] = None();
    }
    return result;
}

uint32_t ast_addFunctionType(
    Types* types, uint32_t returnType, uint32_t parameterCount
) {
    uint32_t result = list_size(types->types);
    Type newType = {
        FUNCTION_TYPE,
        .functionType = (FunctionType
        ){returnType, parameterCount, list_size(types->parameters)}
    };
    list_append(types->types, newType);
    list_ensureExtraCapacity(types->parameters, parameterCount);
    list_setSize(
        types->parameters,
        list_size(types->parameters) + parameterCount
    );
    uint32_t* parameters = ast_getParameters(*types, result);
    for (uint32_t i = 0; i < parameterCount; i++) {
        parameters[i] = None();
    }
    return result;
}

uint32_t ast_getType(Ast ast, uint32_t instruction) {
    AstInstructionKind kind = *list_get(ast.code.instructions, instruction);
    switch (kind) {
    case AST_DECLARATION: unreachable();
    case AST_ASSIGNMENT: unreachable();
    case AST_RETURN: unreachable();
    case AST_RETURN_EXPRESSION: unreachable();
    case AST_BREAK: unreachable();
    case AST_CONTINUE: unreachable();
    case AST_IF_ELSE: unreachable();
    case AST_WHILE: unreachable();
    case AST_CALL: return ((AstCall*)ast_getData(&ast.code, instruction))->type;
    case AST_IF_ELSE_EXPRESSION: todo();
    case AST_NOT: todo();
    case AST_OR: todo();
    case AST_AND: todo();
    case AST_EQUAL_EQUAL: todo();
    case AST_NOT_EQUAL: todo();
    case AST_LESS: todo();
    case AST_LESS_EQUAL: todo();
    case AST_GREATER: todo();
    case AST_GREATER_EQUAL: todo();
    case AST_ADD: return ((AstAdd*)ast_getData(&ast.code, instruction))->type;
    case AST_SUBTRACT: todo();
    case AST_MUL: todo();
    case AST_DIV: todo();
    case AST_MOD: todo();
    case AST_NEGATION: todo();
    case AST_DEREFERENCE: todo();
    case AST_ADDRESS_OF: todo();
    case AST_FIELD_ACCESS: todo();
    case AST_INDEX_ACCESS: todo();
    case AST_FLOAT:
        return ((AstFloat*)ast_getData(&ast.code, instruction))->type;
    case AST_INTEGER:
        return ((AstInteger*)ast_getData(&ast.code, instruction))->type;
    case AST_IDENTIFIER:
        return ((AstIdentifier*)ast_getData(&ast.code, instruction))->type;
    case AST_BOOLEAN:
        return ((AstBoolean*)ast_getData(&ast.code, instruction))->type;
    case AST_STRING: todo();
    case AST_ARRAY: todo();
    case AST_STRUCT_LITERAL: todo();
    }
}

uint32_t* ast_getParameters(Types types, uint32_t typeId) {
    TypeKind kind = list_get(types.types, typeId)->kind;
    switch (kind) {
    case TYPE_VARIABLE: unreachable();
    case TYPE_WITH_PARAMS: {
        TypeWithParams type = *ast_getTypeWithParams(types, typeId);
        if (type.parameterCount == 0) return NULL;
        return list_get(types.parameters, type.firstParameter);
    }
    case FUNCTION_TYPE: {
        FunctionType type = *ast_getFunctionType(types, typeId);
        if (type.parameterCount == 0) return NULL;
        return list_get(types.parameters, type.firstParameter);
    }
    }
}

static inline uint32_t numberOfSlotsNeededForLiteral(uint32_t length) {
    return 1 + length / sizeof(uint32_t) + (length % sizeof(uint32_t) != 0);
}

uint32_t ast_getLiteral(Ast* ast, char* literal) {
    uint32_t length = strlen(literal) + 1;

    // Check if literal already exist
    uint32_t literalId = None();
    for (uint32_t i = 0; i < list_size(ast->literals);) {
        uint32_t* otherLiteral = list_get(ast->literals, i);
        bool equalLength = *otherLiteral == length;
        if (equalLength && memcmp(otherLiteral + 1, literal, length) == 0) {
            literalId = i;
            break;
        }
        i += numberOfSlotsNeededForLiteral(*list_get(ast->literals, i));
    }

    // Add literal to literals
    if (literalId == None()) {
        literalId = list_size(ast->literals);

        uint32_t extraCapacityNeeded = numberOfSlotsNeededForLiteral(length);
        list_ensureExtraCapacity((ast->literals), extraCapacityNeeded);

        list_size(ast->literals) += extraCapacityNeeded;
        *list_get(ast->literals, literalId) = length;
        strncpy((char*)list_get(ast->literals, literalId + 1), literal, length);
    }

    return literalId;
}

char* ast_literalAsString(Ast ast, uint32_t literal) {
    return (char*)list_get((ast).literals, literal + 1);
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

static void ast_printCode(Ast ast, Code code, uint32_t indentationLevel) {
    arena_newLifetime();
    Uint32Stack indentation = Stack();
    Uint32Stack separations = Stack();

    for (uint32_t i = 0; i < list_size(code.instructions); i += 1) {
        ast_printIndentation(indentationLevel + stack_size(indentation));
        AstInstructionKind kind = *list_get(code.instructions, i);
        switch (kind) {
        case AST_DECLARATION: {
            AstDeclaration* data = ast_getData(&code, i);
            if (data->type != None()) {
                printf(
                    "declaration(%s, expectedType: ",
                    ast_literalAsString(ast, data->identifier)
                );
                ast_printType(ast, data->type);
                printf(", mutable: %s)\n", data->mutable ? "true" : "false");
            } else {
                printf(
                    "declaration(%s, mutable: %s)\n",
                    ast_literalAsString(ast, data->identifier),
                    data->mutable ? "true" : "false"
                );
            }
            break;
        }
        case AST_ASSIGNMENT: printf("assignment\n"); break;
        case AST_RETURN: printf("return\n"); break;
        case AST_RETURN_EXPRESSION: printf("returnExpression\n"); break;
        case AST_BREAK: printf("break\n"); break;
        case AST_CONTINUE: printf("continue\n"); break;
        case AST_IF_ELSE: {
            AstIfElse* data = ast_getData(&code, i);
            printf("ifElse\n");
            if (data->elseBlockEnd != None()) {
                stack_push(separations, data->ifBlockEnd);
                stack_push(indentation, data->elseBlockEnd);
            } else {
                stack_push(indentation, data->ifBlockEnd);
            }
            break;
        }
        case AST_WHILE: {
            AstWhile* data = ast_getData(&code, i);
            printf("while\n");
            stack_push(indentation, data->whileEnd);
            break;
        }
        case AST_CALL: {
            AstCall* data = ast_getData(&code, i);
            printf("call(");
            for (uint32_t arg = 0; arg < data->argumentsCount; arg++) {
                printf("_");
                if (arg + 1 != data->argumentsCount) {
                    printf(", ");
                }
            }
            printf(")\n");
            break;
        }
        case AST_IF_ELSE_EXPRESSION: printf("ifElseExpression\n"); break;
        case AST_NOT: printf("not\n"); break;
        case AST_OR: printf("%s\n", getBinaryOp(kind)); break;
        case AST_AND: printf("%s\n", getBinaryOp(kind)); break;
        case AST_EQUAL_EQUAL: printf("%s\n", getBinaryOp(kind)); break;
        case AST_NOT_EQUAL: printf("%s\n", getBinaryOp(kind)); break;
        case AST_LESS: printf("%s\n", getBinaryOp(kind)); break;
        case AST_LESS_EQUAL: printf("%s\n", getBinaryOp(kind)); break;
        case AST_GREATER: printf("%s\n", getBinaryOp(kind)); break;
        case AST_GREATER_EQUAL: printf("%s\n", getBinaryOp(kind)); break;
        case AST_ADD: {
            AstAdd* data = ast_getData(&code, i);
            printf("%s", getBinaryOp(kind));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type);
            }
            printf("\n");
            break;
        }
        case AST_SUBTRACT: printf("%s\n", getBinaryOp(kind)); break;
        case AST_MUL: printf("%s\n", getBinaryOp(kind)); break;
        case AST_DIV: printf("%s\n", getBinaryOp(kind)); break;
        case AST_MOD: printf("%s\n", getBinaryOp(kind)); break;
        case AST_NEGATION: printf("-\n"); break;
        case AST_DEREFERENCE: printf("dereference\n"); break;
        case AST_ADDRESS_OF: printf("addressOf\n"); break;
        case AST_FIELD_ACCESS: printf("fieldAccess\n"); break;
        case AST_INDEX_ACCESS: printf("indexAccess\n"); break;
        case AST_FLOAT: {
            AstFloat* data = ast_getData(&code, i);
            printf("%s", ast_literalAsString(ast, data->literal));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type);
            }
            printf("\n");
            break;
        }
        case AST_INTEGER: {
            AstInteger* data = ast_getData(&code, i);
            printf("integer(%s)\n", ast_literalAsString(ast, data->literal));
            break;
        }
        case AST_IDENTIFIER: {
            AstIdentifier* data = ast_getData(&code, i);
            printf("%s", ast_literalAsString(ast, data->literal));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type);
            }
            printf("\n");
            break;
        }
        case AST_BOOLEAN: {
            AstBoolean* data = ast_getData(&code, i);
            printf("boolean(%s)\n", data->value ? "true" : "false");
            break;
        }
        case AST_STRING: {
            AstString* data = ast_getData(&code, i);
            printf("string(\"%s\")\n", ast_literalAsString(ast, data->literal));
            break;
        }
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
    printf("type %s\n", ast_literalAsString(ast, typeDefinition.identifier));
    assert(
        list_size(typeDefinition.fields) == list_size(typeDefinition.fieldTypes)
    );
    for (uint32_t i = 0; i < list_size(typeDefinition.fields); i++) {
        ast_printIndentation(indentationLevel + 1);
        printf(
            "%s: ",
            ast_literalAsString(ast, *list_get(typeDefinition.fields, i))
        );
        ast_printType(ast, *list_get(typeDefinition.fieldTypes, i));
        printf("\n");
    }
}

static void ast_printFunction(
    Ast ast, Function function, uint32_t indentationLevel
) {
    ast_printIndentation(indentationLevel);
    printf("function %s", ast_literalAsString(ast, function.identifier));
    printf("(");
    for (uint32_t i = 0; i < list_size(function.arguments); i++) {
        FunctionArgument argument = *list_get(function.arguments, i);
        if (argument.mutable) {
            printf("mut ");
        }
        printf("%s", ast_literalAsString(ast, argument.identifier));
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
            "import %s\n",
            ast_literalAsString(ast, list_get(ast.imports, i)->path)
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

void ast_printType(Ast ast, uint32_t typeId) {
    TypeKind kind = ((Type*)list_get(ast.types.types, typeId))->kind;
    switch (kind) {
    case TYPE_VARIABLE: {
        TypeVariable* variable = ast_getTypeVariable(ast.types, typeId);
        printf("%d", variable->id);
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams* type = ast_getTypeWithParams(ast.types, typeId);
        printf("%s", ast_literalAsString(ast, type->literal));
        if (type->parameterCount != 0) {
            uint32_t* parameters = ast_getParameters(ast.types, typeId);
            printf("[");
            for (uint32_t i = 0; i < type->parameterCount; i++) {
                ast_printType(ast, parameters[i]);
                if (i + 1 != type->parameterCount) {
                    printf(", ");
                }
            }
            printf("]");
        }
        break;
    }
    case FUNCTION_TYPE: {
        todo();
        break;
    }
    default: unreachable();
    }
}