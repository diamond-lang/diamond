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

void ast_clear(Ast* ast) {
    ast->importedAsts.count = 0;
    ast->imports.count = 0;
    ast->functions.count = 0;
    ast->interfaces.count = 0;
    ast->typeDefinitions.count = 0;
    ast->code.instructions.count = 0;
    ast->code.dataOrIndex.count = 0;
    ast->errors.count = 0;
}

uint32_t ast_addInstruction(Ast* ast, Code* code, AstInstructionKind kind) {
    list_append(&ast->arena, code->instructions, kind);
    list_append(&ast->arena, code->dataOrIndex, None());
    uint32_t id = list_size(code->instructions);
    ast_addData(ast, code, id);
    return id;
}

uint32_t ast_insertInst(
    Ast* ast, Code* code, AstInstructionKind kind, uint32_t offset
) {
    // Make space for insertation
    list_ensureExtraCapacity(&ast->arena, code->instructions, 1);
    list_ensureExtraCapacity(&ast->arena, code->dataOrIndex, 1);
    code->instructions.count += 1;
    code->dataOrIndex.count += 1;
    for (size_t i = list_size(code->instructions) - 2;
         offset <= i && i <= list_size(code->instructions) - 2;
         i--) {
        *list_get(code->instructions, i + 1) = *list_get(code->instructions, i);
        *list_get(code->dataOrIndex, i + 1) = *list_get(code->dataOrIndex, i);
    }

    // Remember sizes
    size_t actualSize = code->instructions.count;

    // Set size
    code->instructions.count = offset;
    code->dataOrIndex.count = offset;

    // Add instruction
    uint32_t inst = ast_addInstruction(ast, code, kind);

    // Restore size;
    code->instructions.count = actualSize;
    code->dataOrIndex.count = actualSize;

    // Returm
    return inst;
}

AstInstructionKind ast_getInstruction(Code code, uint32_t id) {
    assert(1 <= id && id <= list_size(code.instructions));
    return *list_get(code.instructions, id - 1);
}

uint32_t* ast_getDataOrIndex(Code code, uint32_t id) {
    assert(1 <= id && id <= list_size(code.dataOrIndex));
    return list_get(code.dataOrIndex, id - 1);
}

static uint32_t sizeOfData(AstInstructionKind kind) {
    uint32_t result;
    switch (kind) {
    case AST_DECLARATION: result = sizeof(AstDeclaration); break;
    case AST_ASSIGNMENT: result = sizeof(AstAssignment); break;
    case AST_RETURN: result = sizeof(AstReturn); break;
    case AST_RETURN_LAST_EXPRESSION:
        result = sizeof(AstReturnLastExpression);
        break;
    case AST_BREAK: result = sizeof(AstBreak); break;
    case AST_CONTINUE: result = sizeof(AstContinue); break;
    case AST_IF_ELSE: result = sizeof(AstIfElse); break;
    case AST_WHILE: result = sizeof(AstWhile); break;
    case AST_CALL: result = sizeof(AstCall); break;
    case AST_IF_ELSE_EXPRESSION: result = sizeof(AstIfElseExpression); break;
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
    return result;
}

void ast_addData(Ast* ast, Code* code, uint32_t instruction) {
    AstInstructionKind inst = ast_getInstruction(*code, instruction);
    uint32_t size = sizeOfData(inst);
    uint32_t alignment = sizeof(uint32_t);
    uint32_t numberOfSlotsUsed = size / sizeof(uint32_t);
    if (numberOfSlotsUsed == 0) return;
    else if (numberOfSlotsUsed == 1) return;
    else {
        assert(*ast_getDataOrIndex(*code, instruction) == None());
        uint8_t* data =
            arena_allocWithAlignment(&ast->arena, size, alignment, 1);
        *ast_getDataOrIndex(*code, instruction) = arena_getId(ast->arena, data);
    }
}

void* ast_getData(Ast* ast, Code* code, uint32_t instruction) {
    uint32_t numberOfSlotsUsed =
        sizeOfData(ast_getInstruction(*code, instruction));
    if (numberOfSlotsUsed == 0) return NULL;
    if (numberOfSlotsUsed == 1) {
        return ast_getDataOrIndex(*code, instruction);
    } else {
        uint32_t arenaId = *ast_getDataOrIndex(*code, instruction);
        return arena_getPointer(ast->arena, arenaId);
    }
}

uint32_t ast_addTypeVariable(Ast* ast, uint32_t typeVariable) {
    Type* type = arena_allocWithAlignment(&ast->arena, sizeof(Type), 4, 1);
    *type = (Type){TYPE_VARIABLE, .variable = (TypeVariable){typeVariable}};
    uint32_t id = arena_getId(ast->arena, type);
    return id;
}

uint32_t ast_addTypeWithParams(
    Ast* ast, uint32_t literal, Uint32List parameters
) {
    Type* type = arena_allocWithAlignment(
        &ast->arena,
        sizeof(Type) + sizeof(uint32_t) * list_size(parameters),
        4,
        1
    );
    *type =
        (Type){TYPE_WITH_PARAMS,
               .withParams = (TypeWithParams
               ){.literal = literal, .parameterCount = list_size(parameters)}};
    for (uint32_t i = 0; i < list_size(parameters); i++) {
        type->withParams.parameters[i] = *list_get(parameters, i);
    }
    uint32_t id = arena_getId(ast->arena, type);
    return id;
}

uint32_t ast_addFunctionType(
    Ast* ast, Uint32List arguments, uint32_t returnType
) {
    Type* type = arena_allocWithAlignment(
        &ast->arena,
        sizeof(Type) + sizeof(uint32_t) * (list_size(arguments) + 1),
        4,
        1
    );
    uint32_t literal = ast_getLiteral(ast, "->");
    *type = (Type
    ){TYPE_WITH_PARAMS,
      .withParams = (TypeWithParams
      ){.literal = literal, .parameterCount = list_size(arguments) + 1}};
    for (uint32_t i = 0; i < list_size(arguments); i++) {
        type->withParams.parameters[i] = *list_get(arguments, i);
    }
    type->withParams.parameters[list_size(arguments)] = returnType;
    uint32_t id = arena_getId(ast->arena, type);
    return id;
}

Type* ast_findType(Ast* ast, uint32_t type) {
    Type* result = ast_getType(*ast, type);
    while (result->kind == TYPE_VARIABLE) {
        if (result->variable.forwarded == None()) break;
        result = ast_getType(*ast, result->variable.forwarded);
    }
    return result;
}

Type* ast_findTypeWithMappings(
    Ast* ast, uint32_t type, TypeReferenceHashmap mappings
) {
    Type* result = ast_findType(ast, type);

    return result;
}

void ast_makeEqual(TypeVariable* typeVariable, uint32_t other) {
    typeVariable->forwarded = other;
}

Type* ast_getType(Ast ast, uint32_t type) {
    return arena_getPointerWithType(ast.arena, Type, type);
}

uint32_t ast_getTypeOfInstruction(Ast* ast, Code* code, uint32_t instruction) {
    AstInstructionKind kind = ast_getInstruction(*code, instruction);
    switch (kind) {
    case AST_DECLARATION: unreachable();
    case AST_ASSIGNMENT: unreachable();
    case AST_RETURN: unreachable();
    case AST_RETURN_LAST_EXPRESSION: unreachable();
    case AST_BREAK: unreachable();
    case AST_CONTINUE: unreachable();
    case AST_IF_ELSE: unreachable();
    case AST_WHILE: unreachable();
    case AST_CALL: return ((AstCall*)ast_getData(ast, code, instruction))->type;
    case AST_IF_ELSE_EXPRESSION: todo();
    case AST_DEREFERENCE: todo();
    case AST_ADDRESS_OF: todo();
    case AST_FIELD_ACCESS: todo();
    case AST_INDEX_ACCESS: todo();
    case AST_FLOAT:
        return ((AstFloat*)ast_getData(ast, code, instruction))->type;
    case AST_INTEGER:
        return ((AstInteger*)ast_getData(ast, code, instruction))->type;
    case AST_IDENTIFIER:
        return ((AstIdentifier*)ast_getData(ast, code, instruction))->type;
    case AST_BOOLEAN:
        return ((AstBoolean*)ast_getData(ast, code, instruction))->type;
    case AST_STRING: todo();
    case AST_ARRAY: todo();
    case AST_STRUCT_LITERAL: todo();
    }
}

String ast_typeAsString(Arena* arena, Ast ast, uint32_t typeId) {
    Type* type = ast_findType(&ast, typeId);
    String result = {0};
    switch (type->kind) {
    case TYPE_VARIABLE: {
        TypeVariable* data = &type->variable;
        result = numberAsString(arena, data->id);
        break;
    }
    case TYPE_WITH_PARAMS: {
        TypeWithParams* data = &type->withParams;
        if (data->literal == ast_getLiteral(&ast, "->")) {
            assert(data->parameterCount >= 2);
            string_append(arena, &result, '(');
            for (uint32_t i = 0; i < data->parameterCount - 1; i++) {
                String param =
                    ast_typeAsString(arena, ast, data->parameters[i]);
                string_concat(arena, &result, string_asView(param));
                if (i + 1 != data->parameterCount - 1) {
                    string_append(arena, &result, ',');
                    string_append(arena, &result, ' ');
                }
            }
            string_append(arena, &result, ')');
            string_concat(arena, &result, cStringAsView("->"));
            String returnTypeStr = ast_typeAsString(
                arena,
                ast,
                data->parameters[data->parameterCount - 1]
            );
            string_concat(arena, &result, string_asView(returnTypeStr));
        } else {
            string_concat(
                arena,
                &result,
                ast_literalAsView(ast, data->literal)
            );
            if (data->parameterCount > 0) {
                string_append(arena, &result, '[');
                for (uint32_t i = 0; i < data->parameterCount; i++) {
                    String paramAsString =
                        ast_typeAsString(arena, ast, data->parameters[i]);
                    string_concat(arena, &result, string_asView(paramAsString));
                    if (i + 1 != data->parameterCount) {
                        string_append(arena, &result, ',');
                        string_append(arena, &result, ' ');
                    }
                }
                string_append(arena, &result, ']');
            }
        }
        break;
    }
    }
    return result;
}

String ast_typeReferenceAsString(Arena* arena, Ast ast, TypeReference typeRef) {
    TypeDefinition* type = list_get(ast.typeDefinitions, typeRef.id);
    String result = {0};
    string_concat(arena, &result, ast_literalAsView(ast, type->identifier));
    if (typeRef.parameterCount != 0) {
        todo();
    }
    return result;
}

bool ast_isTypeVariable(Ast ast, TypeWithParams* type) {
    StringView view = ast_literalAsView(ast, type->literal);
    bool result = string_isLowerCase(view);
    assert(!result || type->parameterCount == 0);
    return result;
}

bool ast_areTypesEqual(Ast* ast, uint32_t aId, uint32_t bId) {
    Type* a = ast_findType(ast, aId);
    Type* b = ast_findType(ast, bId);
    if (a->kind == b->kind) {
        TypeKind kind = a->kind;
        switch (kind) {
        case TYPE_VARIABLE: {
            return a->variable.id = b->variable.id;
            break;
        }
        case TYPE_WITH_PARAMS: {
            if (a->withParams.literal != b->withParams.literal) {
                return false;
            }
            assert(
                a->withParams.parameterCount == b->withParams.parameterCount
            );
            for (uint32_t i = 0; i < a->withParams.parameterCount; i++) {
                bool result = ast_areTypesEqual(
                    ast,
                    a->withParams.parameters[i],
                    b->withParams.parameters[i]
                );
                if (result == false) return result;
            }
            return true;
        }
        }
    }
    return false;
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
                arena_getPointer(ast->arena, otherLiteral->arenaId),
                literal,
                length
            ) == 0) {
            literalId = i + 1;
            break;
        }
    }

    // Add literal to literals
    if (literalId == None()) {
        char* buffer = arena_alloc(&ast->arena, char, length + 1);
        strncpy(buffer, literal, length);
        buffer[length] = '\0';
        Literal newLiteral = {hash, length, arena_getId(ast->arena, buffer)};
        list_append(&ast->arena, ast->literals, newLiteral);
        literalId = list_size(ast->literals);
    }

    return literalId;
}

char* ast_literalAsString(Ast ast, uint32_t literal) {
    return (char*)(arena_getPointer(
        ast.arena,
        list_get((ast).literals, literal - 1)->arenaId
    ));
}

Implementation* ast_getImplementation(
    Implementations* implementations, TypeReference typeRef
) {
    for (uint64_t i = 0; i < list_size(implementations->keys); i++) {
        TypeReference key = *list_get(implementations->keys, i);
        if (key.id == typeRef.id && key.moduleId == typeRef.moduleId) {
            return list_get(implementations->functions, i);
        }
    }
    return NULL;
}

void ast_setImplementation(
    Arena* arena,
    Implementations* implementations,
    TypeReference typeRef,
    Implementation implementation
) {
    bool founded = false;
    for (uint64_t i = 0; i < list_size(implementations->keys); i++) {
        TypeReference key = *list_get(implementations->keys, i);
        if (key.id == typeRef.id && key.moduleId == typeRef.moduleId) {
            *list_get(implementations->functions, i) = implementation;
            founded = true;
        }
    }
    if (!founded) {
        list_append(arena, implementations->keys, typeRef);
        list_append(arena, implementations->functions, implementation);
    }
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
        AstInstructionKind kind = ast_getInstruction(code, i);
        switch (kind) {
        case AST_DECLARATION: {
            AstDeclaration* data = ast_getData(&ast, &code, i);
            if (data->type != None()) {
                printf(
                    "declaration(%s, expectedType: ",
                    ast_literalAsString(ast, data->identifier)
                );
                ast_printType(ast, data->type, scratch);
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
        case AST_RETURN_LAST_EXPRESSION:
            printf("returnLastExpression\n");
            break;
        case AST_BREAK: printf("break\n"); break;
        case AST_CONTINUE: printf("continue\n"); break;
        case AST_IF_ELSE: {
            AstIfElse* data = ast_getData(&ast, &code, i);
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
            AstWhile* data = ast_getData(&ast, &code, i);
            printf("while\n");
            stack_push(&scratch, indentation, data->whileEnd);
            break;
        }
        case AST_CALL: {
            AstCall* data = ast_getData(&ast, &code, i);
            printf("call(");
            for (uint32_t arg = 0; arg < data->argumentsCount; arg++) {
                printf("_");
                if (arg + 1 != data->argumentsCount) {
                    printf(", ");
                }
            }
            printf(")");
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type, scratch);
            }
            printf("\n");
            break;
        }
        case AST_IF_ELSE_EXPRESSION: printf("ifElseExpression\n"); break;
        case AST_DEREFERENCE: printf("dereference\n"); break;
        case AST_ADDRESS_OF: printf("addressOf\n"); break;
        case AST_FIELD_ACCESS: printf("fieldAccess\n"); break;
        case AST_INDEX_ACCESS: printf("indexAccess\n"); break;
        case AST_FLOAT: {
            AstFloat* data = ast_getData(&ast, &code, i);
            printf("%s", ast_literalAsString(ast, data->literal));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type, scratch);
            }
            printf("\n");
            break;
        }
        case AST_INTEGER: {
            AstInteger* data = ast_getData(&ast, &code, i);
            printf("%s", ast_literalAsString(ast, data->literal));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type, scratch);
            }
            printf("\n");
            break;
        }
        case AST_IDENTIFIER: {
            AstIdentifier* data = ast_getData(&ast, &code, i);
            printf("%s", ast_literalAsString(ast, data->literal));
            if (data->type != None()) {
                printf(": ");
                ast_printType(ast, data->type, scratch);
            }
            printf("\n");
            break;
        }
        case AST_BOOLEAN: {
            AstBoolean* data = ast_getData(&ast, &code, i);
            printf("boolean(%s)\n", data->value ? "true" : "false");
            break;
        }
        case AST_STRING: {
            AstString* data = ast_getData(&ast, &code, i);
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
    Ast ast,
    TypeDefinition typeDefinition,
    uint32_t indentationLevel,
    Arena scratch
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
        ast_printType(ast, *list_get(typeDefinition.fieldTypes, i), scratch);
        printf("\n");
    }
}

static void ast_printInterface(
    Ast ast, Interface interface, uint32_t indentationLevel, Arena scratch
) {
    ast_printIndentation(indentationLevel);
    printf("interface %s[", ast_literalAsString(ast, interface.identifier));
    ast_printType(ast, interface.parameter, scratch);
    printf("](");
    for (uint32_t i = 0; i < list_size(interface.arguments); i++) {
        FunctionArgument argument = *list_get(interface.arguments, i);
        if (argument.mutable) {
            printf("mut ");
        }
        printf("%s", ast_literalAsString(ast, argument.identifier));
        if (argument.type != None()) {
            printf(": ");
            ast_printType(ast, argument.type, scratch);
        }
        if (i + 1 != list_size(interface.arguments)) {
            printf(", ");
        }
    }
    printf(")");
    if (interface.returnType != None()) {
        printf(": ");
        ast_printType(ast, interface.returnType, scratch);
    }
    printf("\n");
}

static void ast_printFunction(
    Ast ast, Function function, uint32_t indentationLevel, Arena scratch
) {
    ast_printIndentation(indentationLevel);
    printf("function %s", ast_literalAsString(ast, function.identifier));
    printf("(");
    for (uint32_t i = 0; i < list_size(function.arguments); i++) {
        FunctionArgument* arg = list_get(function.arguments, i);
        if (arg->mutable) {
            printf("mut ");
        }
        printf("%s", ast_literalAsString(ast, arg->identifier));
        if (arg->type != None()) {
            printf(": ");
            ast_printType(ast, arg->type, scratch);
        }
        if (i + 1 != list_size(function.arguments)) {
            printf(", ");
        }
    }
    printf(")");
    if (function.returnType != None()) {
        printf(": ");
        ast_printType(ast, function.returnType, scratch);
    }
    if (list_size(function.constraints) != 0) {
        printf(" with ");
    }
    for (uint32_t i = 0; i < list_size(function.constraints); i++) {
        Constraint constraint = *list_get(function.constraints, i);
        printf("%s[", ast_literalAsString(ast, constraint.identifier));
        ast_printType(ast, constraint.parameter, scratch);
        printf("]");
        if (i + 1 != list_size(function.constraints)) {
            printf(", ");
        }
    }
    printf("\n");
    ast_printCode(ast, function.code, indentationLevel + 1, scratch);
}

void ast_print(Ast ast, String path, Arena scratch1, Arena scratch2) {
    String workinDirectory = getWorkingDirectory(&scratch1);
    String relativePath =
        getRelativePath(&scratch1, workinDirectory, path, scratch2);
    printf("%s\n", string_asCString(relativePath));
    for (uint32_t i = 0; i < list_size(ast.imports); i++) {
        ast_printIndentation(1);
        printf(
            "import %s\n",
            ast_literalAsString(ast, list_get(ast.imports, i)->path)
        );
    }
    ast_printCode(ast, ast.code, 1, scratch1);
    for (uint32_t i = 0; i < list_size(ast.typeDefinitions); i++) {
        ast_printTypeDefinition(
            ast,
            *list_get(ast.typeDefinitions, i),
            1,
            scratch1
        );
    }
    for (uint32_t i = 0; i < list_size(ast.interfaces); i++) {
        ast_printInterface(ast, *list_get(ast.interfaces, i), 1, scratch1);
    }
    for (uint32_t i = 0; i < list_size(ast.functions); i++) {
        ast_printFunction(ast, *list_get(ast.functions, i), 1, scratch1);
    }
}

void ast_printType(Ast ast, uint32_t typeId, Arena scratch) {
    String string = ast_typeAsString(&scratch, ast, typeId);
    printf("%s", string.buffer);
}