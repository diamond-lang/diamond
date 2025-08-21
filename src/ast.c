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
#include "utilities.h"

static uint32_t ast_numberOfSlotsUsedInData(AstInstructionKind kind);

void ast_clear(Ast* ast) {
    ast->importedAsts.count = 0;
    ast->imports.count = 0;
    ast->functions.count = 0;
    ast->typeDefinitions.count = 0;
    ast->code.instructions.count = 0;
    ast->code.dataOrIndex.count = 0;
    ast->code.data.count = 0;
    ast->types.types.count = 0;
    ast->types.parameters.count = 0;
    ast->errors.count = 0;
}

uint32_t ast_addInstruction(Arena* arena, Code* code, AstInstructionKind kind) {
    list_append(arena, code->instructions, kind);
    list_append(arena, code->dataOrIndex, None());
    uint32_t id = list_size(code->instructions);
    ast_addData(arena, code, id);
    return id;
}

AstInstructionKind ast_getInstruction(Code code, uint32_t id) {
    assert(1 <= id && id <= list_size(code.instructions));
    return *list_get(code.instructions, id - 1);
}

uint32_t* ast_getDataOrIndex(Code code, uint32_t id) {
    assert(1 <= id && id <= list_size(code.dataOrIndex));
    return list_get(code.dataOrIndex, id - 1);
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

void ast_addData(Arena* arena, Code* code, uint32_t instruction) {
    uint32_t numberOfSlotsUsed =
        ast_numberOfSlotsUsedInData(ast_getInstruction(*code, instruction));
    if (numberOfSlotsUsed == 0) return;
    if (numberOfSlotsUsed == 1) return;
    else {
        assert(*ast_getDataOrIndex(*code, instruction) == None());
        *ast_getDataOrIndex(*code, instruction) = list_size(code->data);
        list_ensureExtraCapacity(arena, code->data, numberOfSlotsUsed);
        list_setSize(code->data, list_size(code->data) + numberOfSlotsUsed);
    }
}

void* ast_getData(Code* code, uint32_t instruction) {
    uint32_t numberOfSlotsUsed =
        ast_numberOfSlotsUsedInData(ast_getInstruction(*code, instruction));
    if (numberOfSlotsUsed == 0) return NULL;
    if (numberOfSlotsUsed == 1) {
        return ast_getDataOrIndex(*code, instruction);
    } else {
        return list_get(code->data, *ast_getDataOrIndex(*code, instruction));
    }
}

uint32_t ast_addTypeVariable(
    Arena* arena, Types* types, uint32_t typeVariable
) {
    Type newType = {TYPE_VARIABLE, .variable = (TypeVariable){typeVariable}};
    list_append(arena, types->types, newType);
    return list_size(types->types);
}

uint32_t ast_addTypeWithParams(
    Arena* arena, Types* types, uint32_t literal, uint32_t parameterCount
) {
    Type newType = {
        TYPE_WITH_PARAMS,
        .withParams = (TypeWithParams
        ){literal, parameterCount, list_size(types->parameters)}
    };
    list_append(arena, types->types, newType);
    list_ensureExtraCapacity(arena, types->parameters, parameterCount);
    list_setSize(
        types->parameters,
        list_size(types->parameters) + parameterCount
    );
    uint32_t result = list_size(types->types);
    uint32_t* parameters = ast_getParameters(*types, result);
    for (uint32_t i = 0; i < parameterCount; i++) {
        parameters[i] = None();
    }
    return result;
}

uint32_t ast_addFunctionType(
    Arena* arena, Types* types, uint32_t returnType, uint32_t parameterCount
) {
    Type newType = {
        FUNCTION_TYPE,
        .functionType = (FunctionType
        ){returnType, parameterCount, list_size(types->parameters)}
    };
    list_append(arena, types->types, newType);
    list_ensureExtraCapacity(arena, types->parameters, parameterCount);
    list_setSize(
        types->parameters,
        list_size(types->parameters) + parameterCount
    );
    uint32_t result = list_size(types->types);
    uint32_t* parameters = ast_getParameters(*types, result);
    for (uint32_t i = 0; i < parameterCount; i++) {
        parameters[i] = None();
    }
    return result;
}

TypeKind ast_getTypeKind(Types types, uint32_t type) {
    return list_get(types.types, type - 1)->kind;
}

Type* ast_getType(Types types, uint32_t type) {
    return list_get(types.types, type - 1);
}

uint32_t ast_getTypeOfInstruction(Ast ast, uint32_t instruction) {
    AstInstructionKind kind = ast_getInstruction(ast.code, instruction);
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

uint32_t ast_getParametersCount(Types types, uint32_t typeId) {
    TypeKind kind = list_get(types.types, typeId - 1)->kind;
    switch (kind) {
    case TYPE_VARIABLE: unreachable();
    case TYPE_WITH_PARAMS: {
        TypeWithParams type = *ast_getTypeWithParams(types, typeId);
        return type.parameterCount;
    }
    case FUNCTION_TYPE: {
        FunctionType type = *ast_getFunctionType(types, typeId);
        return type.parameterCount;
    }
    }
}

uint32_t* ast_getParameters(Types types, uint32_t typeId) {
    TypeKind kind = list_get(types.types, typeId - 1)->kind;
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

bool ast_typeHasParams(TypeKind kind) {
    switch (kind) {
    case TYPE_VARIABLE: return false;
    case TYPE_WITH_PARAMS: return true;
    case FUNCTION_TYPE: return true;
    }
}

String ast_typeAsString(Arena* arena, Ast ast, Types types, uint32_t typeId) {
    String result = {0};
    TypeKind kind = ast_getTypeKind(types, typeId);
    switch (kind) {
    case TYPE_VARIABLE: {
        TypeVariable* type = ast_getTypeVariable(types, typeId);
        int digitsCount = numberOfDigits(type->id);
        string_ensureExtraCapacity(arena, &result, digitsCount + 1);
        snprintf(result.buffer, digitsCount + 1, "%d", type->id);
        result.count = digitsCount;
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams* type = ast_getTypeWithParams(types, typeId);
        string_concat(arena, &result, ast_literalAsView(ast, type->literal));
        if (type->parameterCount > 0) {
            string_append(arena, &result, '[');
            uint32_t* params = ast_getParameters(types, typeId);
            for (uint32_t i = 0; i < type->parameterCount; i++) {
                String param =
                    ast_typeAsString(arena, ast, ast.types, params[i]);
                string_concat(arena, &result, string_asView(param));
                if (i + 1 != type->parameterCount)
                    string_append(arena, &result, ',');
            }
            string_append(arena, &result, ']');
        }
        break;
    }
    case FUNCTION_TYPE: {
        FunctionType* type = ast_getFunctionType(types, typeId);
        assert(type->parameterCount > 0);
        string_append(arena, &result, '(');
        uint32_t* params = ast_getParameters(types, typeId);
        for (uint32_t i = 0; i < type->parameterCount; i++) {
            String param = ast_typeAsString(arena, ast, ast.types, params[i]);
            string_concat(arena, &result, string_asView(param));
            if (i + 1 != type->parameterCount)
                string_append(arena, &result, ',');
        }
        string_append(arena, &result, ')');
        string_concat(arena, &result, cStringAsView("->"));
        String returnType =
            ast_typeAsString(arena, ast, ast.types, type->returnType);
        string_concat(arena, &result, string_asView(returnType));
        break;
    }
    }
    return result;
}

static uint32_t ast_hashString(const char* key, uint32_t length) {
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

uint32_t ast_getLiteral(Ast* ast, char* literal) {
    return ast_getLiteralWithLength(ast, literal, strlen(literal));
}

uint32_t ast_getLiteralWithLength(Ast* ast, char* literal, uint32_t length) {
    // Hash string
    uint32_t hash = ast_hashString(literal, length);

    // Check if literal already exist
    uint32_t literalId = None();
    for (uint32_t i = 0; i < list_size(ast->literals); i++) {
        Literal* otherLiteral = list_get(ast->literals, i);
        bool equalHash = otherLiteral->hash == hash;
        bool equalLength = otherLiteral->length == length;
        if (equalHash && equalLength &&
            memcmp(
                ast->arena.buffer + otherLiteral->arenaOffset,
                literal,
                length
            ) == 0) {
            literalId = i + 1;
            break;
        }
    }

    // Add literal to literals
    if (literalId == None()) {
        char* buffer = alloc(&ast->arena, char, length + 1);
        strncpy(buffer, literal, length);
        buffer[length] = '\0';
        Literal newLiteral = {
            hash,
            length,
            arena_getOffset(ast->arena, buffer)
        };
        list_append(&ast->arena, ast->literals, newLiteral);
        literalId = list_size(ast->literals);
    }

    return literalId;
}

char* ast_literalAsString(Ast ast, uint32_t literal) {
    return (char*)(ast.arena.buffer +
                   list_get((ast).literals, literal - 1)->arenaOffset);
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

static void ast_printCode(
    Ast ast, Code code, uint32_t indentationLevel, Arena scratch
) {
    Uint32Stack indentation = {0};
    Uint32Stack separations = {0};

    for (uint32_t i = 1; i <= list_size(code.instructions); i += 1) {
        ast_printIndentation(indentationLevel + stack_size(indentation));
        AstInstructionKind kind = ast_getInstruction(ast.code, i);
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
                stack_push(&scratch, separations, data->ifBlockEnd);
                stack_push(&scratch, indentation, data->elseBlockEnd);
            } else {
                stack_push(&scratch, indentation, data->ifBlockEnd);
            }
            break;
        }
        case AST_WHILE: {
            AstWhile* data = ast_getData(&code, i);
            printf("while\n");
            stack_push(&scratch, indentation, data->whileEnd);
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
    Ast ast, Function function, uint32_t indentationLevel, Arena scratch
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
    ast_printCode(ast, function.code, indentationLevel + 1, scratch);
}

void ast_print(Ast ast, String path, Arena scratch) {
    printf("%s\n", string_asCString(path));
    for (uint32_t i = 0; i < list_size(ast.imports); i++) {
        ast_printIndentation(1);
        printf(
            "import %s\n",
            ast_literalAsString(ast, list_get(ast.imports, i)->path)
        );
    }
    ast_printCode(ast, ast.code, 1, scratch);
    for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
        ast_printTypeDefinition(ast, *list_get(ast.typeDefinitions, i), 1);
    }
    for (uint32_t i = 0; i < list_size(ast.functions); i++) {
        ast_printFunction(ast, *list_get(ast.functions, i), 1, scratch);
    }
}

void ast_printType(Ast ast, uint32_t typeId) {
    TypeKind kind = ((Type*)list_get(ast.types.types, typeId - 1))->kind;
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
        FunctionType* type = ast_getFunctionType(ast.types, typeId);
        assert(type->parameterCount > 0);
        uint32_t* parameters = ast_getParameters(ast.types, typeId);
        printf("(");
        for (uint32_t i = 0; i < type->parameterCount; i++) {
            ast_printType(ast, parameters[i]);
            if (i + 1 != type->parameterCount) {
                printf(", ");
            }
        }
        printf(")->");
        ast_printType(ast, type->returnType);
        break;
    }
    default: unreachable();
    }
}