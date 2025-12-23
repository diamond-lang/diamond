#include "codegen.h"

#include <inttypes.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "arena.h"
#include "ast.h"
#include "common.h"
#include "lld-c.h"
#include "program.h"
#include "scopes.h"
#include "types.h"
#include "utilities.h"

defineScopesWith(LLVMValueRef);

typedef struct {
    uint32_t literal;
    LLVMValueRef ref;
} GlobalString;

typedef ListType(GlobalString) GlobalStringList;

typedef StackType(LLVMValueRef) LLVMValueStack;

typedef struct {
    Ast* module;
    uint32_t moduleId;
    Program* program;
    Scopes scopes;
    LLVMValueStack stack;
    uint32_t lastTypeVariable;
    Arena arena;
    LLVMContextRef llvmContext;
    LLVMModuleRef llvmModule;
    LLVMBuilderRef llvmBuilder;
    LLVMBasicBlockRef currentBlockEntry;
    LLVMBasicBlockRef lastAfterWhileBlock;  // Needed for break
    LLVMBasicBlockRef lastWhileBlock;       // Needed for continue
    GlobalStringList globalStrings;
    TypeReferenceHashmap mappings;  // Needed in instantiated functions
} Context;

static void init_context(
    Context* context,
    Program* program,
    Ast* ast,
    uint32_t astId,
    String path,
    Arena arena
) {
    context->module = ast;
    context->moduleId = astId;
    context->program = program;
    context->arena = arena;
    context->llvmContext = LLVMContextCreate();
    context->llvmModule =
        LLVMModuleCreateWithNameInContext(path.buffer, context->llvmContext);
    context->llvmBuilder = LLVMCreateBuilderInContext(context->llvmContext);
}

static LLVMTypeRef getTypReferenceAsLLVMType(
    Context* context, TypeReference typeRef, Arena scratch
) {
    Ast* module = program_getAst(context->program, typeRef.moduleId);
    TypeDefinition* typeDef = list_get(module->typeDefinitions, typeRef.id);
    if (typeDef->identifier == ast_getLiteral(module, "Bool")) {
        return LLVMInt1TypeInContext(context->llvmContext);
    } else if (typeDef->identifier == ast_getLiteral(module, "Float64")) {
        return LLVMDoubleTypeInContext(context->llvmContext);
    } else if (typeDef->identifier == ast_getLiteral(module, "Int64")) {
        return LLVMInt64TypeInContext(context->llvmContext);
    } else if (typeDef->identifier == ast_getLiteral(module, "None")) {
        return LLVMVoidTypeInContext(context->llvmContext);
    } else {
        todo();
    }
}

static LLVMTypeRef getTypeAsLLVMType(
    Context* context, uint32_t typeId, Arena scratch
) {
    assert(typeId != None());
    Type* type =
        ast_findTypeWithMappings(context->module, typeId, context->mappings);
    switch (type->kind) {
    case TYPE_VARIABLE: unreachable();
    case TYPE_WITH_PARAMS: {
        if (ast_isTypeVariable(*context->module, &type->withParams)) {
            TypeReference* typeRef =
                array_hashmap_get(context->mappings, type->withParams.literal);
            if (typeRef == NULL) {
                unreachable();
            }
            return getTypReferenceAsLLVMType(context, *typeRef, scratch);
        }

        TypeWithParams* data = &type->withParams;
        if (data->literal == ast_getLiteral(context->module, "->")) {
            assert(data->parameterCount >= 1);
            uint32_t argsCount = data->parameterCount - 1;
            LLVMTypeRef* args = arena_alloc(&scratch, LLVMTypeRef, argsCount);
            for (uint32_t i = 0; i < argsCount; i++) {
                args[i] =
                    getTypeAsLLVMType(context, data->parameters[i], scratch);
            }
            LLVMTypeRef returnType = getTypeAsLLVMType(
                context,
                data->parameters[argsCount],
                scratch
            );
            return LLVMFunctionType(returnType, args, argsCount, false);
        } else {
            TypeBinding* binding =
                getTypeBinding(&context->scopes, data->literal);
            TypeReference typeRef = typeBindingAsTypeReference(binding);
            return getTypReferenceAsLLVMType(context, typeRef, scratch);
        }
    }
    }
    unreachable();
}

static String getMangledFunctionNameAndMappings(
    Arena* arena,
    Context* context,
    Function* function,
    uint32_t instantiation,
    TypeReferenceHashmap* mappings
) {
    // Construct mangled name
    String mangledName = numberAsString(arena, context->moduleId);
    string_concat(arena, &mangledName, cStringAsView("_"));
    string_concat(
        arena,
        &mangledName,
        ast_literalAsView(*context->module, function->identifier)
    );

    if (list_size(function->parameters) != 0) {
        Instantiation* inst = list_get(function->instantiations, instantiation);
        string_concat(arena, &mangledName, cStringAsView("["));
        for (uint32_t j = 0; j < list_size(*inst); j++) {
            TypeReference t = *list_get(*inst, j);
            Ast* module = program_getAst(context->program, t.moduleId);
            String type = ast_typeReferenceAsString(arena, *module, t);
            string_concat(arena, &mangledName, cStringAsView(type.buffer));
        }
        string_concat(arena, &mangledName, cStringAsView("]"));

        // Create mappings
        for (uint32_t j = 0; j < list_size(*inst); j++) {
            TypeReference t = *list_get(*inst, j);
            uint32_t parameterId = *list_get(function->parameters, j);
            Type* parameter = ast_findType(context->module, parameterId);
            assert(parameter->kind == TYPE_WITH_PARAMS);
            array_hashmap_set(
                arena,
                *mappings,
                parameter->withParams.literal,
                t
            );
        }
    } else if (function->isImplementation) {
        string_concat(arena, &mangledName, cStringAsView("["));
        Ast* module = program_getAst(
            context->program,
            function->implementationFor.moduleId
        );
        String type = ast_typeReferenceAsString(
            arena,
            *module,
            function->implementationFor
        );
        string_concat(arena, &mangledName, cStringAsView(type.buffer));
        string_concat(arena, &mangledName, cStringAsView("]"));
    }
    return mangledName;
}

static LLVMValueRef getFunction(
    Context* context,
    uint32_t moduleId,
    Ast* module,
    uint32_t identifier,
    uint32_t functionTypeId,
    uint32_t actualTypeId,
    Arena scratch
) {
    // Construct mangled name
    String mangledName = numberAsString(&scratch, moduleId);
    string_concat(&scratch, &mangledName, cStringAsView("_"));
    string_concat(
        &scratch,
        &mangledName,
        ast_literalAsView(*module, identifier)
    );
    Type* functionType = ast_getType(*module, functionTypeId);
    Type* actualType = ast_getType(*context->module, actualTypeId);
    assert(
        (functionType->kind == TYPE_WITH_PARAMS) &&
        (actualType->kind == TYPE_WITH_PARAMS) &&
        (functionType->withParams.parameterCount ==
         actualType->withParams.parameterCount)
    );

    // Find instantiated parameters
    Uint32Hashmap mappings = {0};
    for (uint32_t i = 0; i < functionType->withParams.parameterCount; i++) {
        Type* parameter =
            ast_findType(module, functionType->withParams.parameters[i]);
        Type* actualParameter =
            ast_findType(context->module, actualType->withParams.parameters[i]);
        assert(parameter->kind == TYPE_WITH_PARAMS);
        assert(actualParameter->kind == TYPE_WITH_PARAMS);
        if (ast_isTypeVariable(*module, &parameter->withParams)) {
            if (array_hashmap_get(mappings, parameter->withParams.literal) ==
                NULL) {
                array_hashmap_set(
                    &scratch,
                    mappings,
                    parameter->withParams.literal,
                    actualType->withParams.parameters[i]
                );
            }
        }
    }

    // Add instantiated parameters to mangled name
    if (array_hashmap_size(mappings) != 0) {
        string_concat(&scratch, &mangledName, cStringAsView("["));
        for (uint32_t i = 0; i < array_hashmap_size(mappings); i++) {
            String parameter = ast_typeAsString(
                &scratch,
                *context->module,
                *list_get(mappings.values, i)
            );
            string_concat(&scratch, &mangledName, string_asView(parameter));
        }
        string_concat(&scratch, &mangledName, cStringAsView("]"));
    }

    // Check if function already added
    LLVMValueRef result =
        LLVMGetNamedFunction(context->llvmModule, mangledName.buffer);
    if (result == NULL) {
        LLVMTypeRef llvmFunctionType =
            getTypeAsLLVMType(context, actualTypeId, scratch);
        result = LLVMAddFunction(
            context->llvmModule,
            mangledName.buffer,
            llvmFunctionType
        );
    }
    return result;
}

static void declaration(
    Context* context,
    Code* code,
    uint32_t id,
    AstDeclaration* data,
    Arena scratch
) {
    addVariableBinding(&context->arena, &context->scopes, 0, NULL);
    todo();
}

static void assignemnt(
    Context* context,
    Code* code,
    uint32_t id,
    AstAssignment* data,
    Arena scratch
) {
    todo();
}

static void returnStmt(
    Context* context, Code* code, uint32_t id, AstReturn* data, Arena scratch
) {
    todo();
}

static void returnExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstReturnLastExpression* data,
    Arena scratch
) {
    todo();
}

static void breakStmt(
    Context* context, Code* code, uint32_t id, AstBreak* data, Arena scratch
) {
    todo();
}

static void continueStmt(
    Context* context, Code* code, uint32_t id, AstContinue* data, Arena scratch
) {
    todo();
}

static void ifElse(
    Context* context, Code* code, uint32_t id, AstIfElse* data, Arena scratch
) {
    todo();
}

static void whileStmt(
    Context* context, Code* code, uint32_t id, AstWhile* data, Arena scratch
) {
    todo();
}

static void call(
    Context* context, Code* code, uint32_t id, AstCall* data, Arena scratch
) {
    LLVMValueRef function = *stack_get(
        context->stack,
        stack_size(context->stack) - data->argumentsCount - 1
    );
    LLVMTypeRef functionType;
    if (LLVMIsAGlobalValue(function)) {
        functionType = LLVMGlobalGetValueType(function);
    } else {
        functionType = LLVMGetElementType(LLVMTypeOf(function));
    }
    LLVMValueRef* args =
        arena_alloc(&scratch, LLVMValueRef, data->argumentsCount);
    for (uint32_t i = 0; i < data->argumentsCount; i++) {
        args[i] = *stack_get(
            context->stack,
            stack_size(context->stack) - data->argumentsCount + i
        );
    }
    LLVMValueRef result = LLVMBuildCall2(
        context->llvmBuilder,
        functionType,
        function,
        args,
        data->argumentsCount,
        ""
    );
    for (uint32_t i = 0; i < data->argumentsCount + 1; i++) {
        stack_pop(context->stack);
    }
    stack_push(&context->arena, context->stack, result);
}

static void ifElseExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstIfElseExpression* data,
    Arena scratch
) {
    todo();
}

static void addressOf(
    Context* context, Code* code, uint32_t id, AstAddressOf* data, Arena scratch
) {
    todo();
}

static void dereference(
    Context* context,
    Code* code,
    uint32_t id,
    AstDereference* data,
    Arena scratch
) {
    todo();
}

static void fieldAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstFieldAccess* data,
    Arena scratch
) {
    todo();
}

static void indexAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstIndexAccess* data,
    Arena scratch
) {
    todo();
}

static void floatLiteral(
    Context* context, Code* code, uint32_t id, AstFloat* data, Arena scratch
) {
    double value;
    sscanf(ast_literalAsString(*context->module, data->literal), "%lf", &value);
    LLVMValueRef llvmValue =
        LLVMConstReal(LLVMDoubleTypeInContext(context->llvmContext), value);
    stack_push(&context->arena, context->stack, llvmValue);
}

static void integerLiteral(
    Context* context, Code* code, uint32_t id, AstInteger* data, Arena scratch
) {
    int64_t value;
    sscanf(
        ast_literalAsString(*context->module, data->literal),
        "%" SCNd64,
        &value
    );
    LLVMValueRef llvmValue =
        LLVMConstInt(LLVMInt64TypeInContext(context->llvmContext), value, true);
    stack_push(&context->arena, context->stack, llvmValue);
}

static void identifier(
    Context* context,
    Code* code,
    uint32_t id,
    AstIdentifier* data,
    Arena scratch
) {
    Binding* binding = getBinding(&context->scopes, data->literal);
    assert(binding);
    switch (binding->kind) {
    case FUNCTION_BINDING: {
        Ast* module =
            program_getAst(context->program, binding->asFunction.module);
        Function* function =
            list_get(module->functions, binding->asFunction.id);
        LLVMValueRef value = getFunction(
            context,
            binding->asFunction.module,
            module,
            function->identifier,
            function->type,
            data->type,
            scratch
        );
        stack_push(&context->arena, context->stack, value);
        break;
    }
    case INTERFACE_BINDING: {
        Ast* module =
            program_getAst(context->program, binding->asFunction.module);
        Interface* interface =
            list_get(module->interfaces, binding->asFunction.id);
        LLVMValueRef value = getFunction(
            context,
            binding->asInterface.module,
            module,
            interface->identifier,
            interface->type,
            data->type,
            scratch
        );
        stack_push(&context->arena, context->stack, value);

        break;
    }
    case ARGUMENT_BINDING: {
        todo();
        break;
    }
    case VARIABLE_BINDING: {
        todo();
        break;
    }
    }
}

static void booleanLiteral(
    Context* context, Code* code, uint32_t id, AstBoolean* data, Arena scratch
) {
    LLVMValueRef value = LLVMConstInt(
        LLVMInt1TypeInContext(context->llvmContext),
        data->value,
        0
    );
    stack_push(&context->arena, context->stack, value);
}

static void stringLiteral(
    Context* context, Code* code, uint32_t id, AstString* data, Arena scratch
) {
    todo();
}

static void arrayLiteral(
    Context* context, Code* code, uint32_t id, AstArray* data, Arena scratch
) {
    todo();
}

static void structLiteral(
    Context* context,
    Code* code,
    uint32_t id,
    AstStructLiteral* data,
    Arena scratch
) {
    todo();
}

static void codegenInstruction(
    Context* context, Code* code, uint32_t id, Arena scratch
) {
    AstInstructionKind kind = ast_getInstruction(*code, id);
    void* data = ast_getData(context->module, code, id);
    switch (kind) {
    case AST_DECLARATION: return declaration(context, code, id, data, scratch);
    case AST_ASSIGNMENT: return assignemnt(context, code, id, data, scratch);
    case AST_RETURN: return returnStmt(context, code, id, data, scratch);
    case AST_RETURN_LAST_EXPRESSION:
        return returnExpression(context, code, id, data, scratch);
    case AST_BREAK: return breakStmt(context, code, id, data, scratch);
    case AST_CONTINUE: return continueStmt(context, code, id, data, scratch);
    case AST_IF_ELSE: return ifElse(context, code, id, data, scratch);
    case AST_WHILE: return whileStmt(context, code, id, data, scratch);
    case AST_CALL: return call(context, code, id, data, scratch);
    case AST_IF_ELSE_EXPRESSION:
        return ifElseExpression(context, code, id, data, scratch);
    case AST_ADDRESS_OF: return addressOf(context, code, id, data, scratch);
    case AST_DEREFERENCE: return dereference(context, code, id, data, scratch);
    case AST_FIELD_ACCESS: return fieldAccess(context, code, id, data, scratch);
    case AST_INDEX_ACCESS: return indexAccess(context, code, id, data, scratch);
    case AST_FLOAT: return floatLiteral(context, code, id, data, scratch);
    case AST_INTEGER: return integerLiteral(context, code, id, data, scratch);
    case AST_IDENTIFIER: return identifier(context, code, id, data, scratch);
    case AST_BOOLEAN: return booleanLiteral(context, code, id, data, scratch);
    case AST_STRING: return stringLiteral(context, code, id, data, scratch);
    case AST_ARRAY: return arrayLiteral(context, code, id, data, scratch);
    case AST_STRUCT_LITERAL:
        return structLiteral(context, code, id, data, scratch);
    default: unreachable();
    }
}

// static void codegenTypeDefinition(
//     Context* context, TypeDefinition* typeDefinition, Arena scratch
// ) {
//     todo();
// }

static void codegenFunctionPrototype(
    Context* context, Function* function, Arena scratch
) {
    for (uint32_t i = 0; i < list_size(function->instantiations); i++) {
        String mangledName = getMangledFunctionNameAndMappings(
            &scratch,
            context,
            function,
            i,
            &context->mappings
        );

        // Add function prototype
        LLVMTypeRef functionType =
            getTypeAsLLVMType(context, function->type, scratch);
        LLVMAddFunction(context->llvmModule, mangledName.buffer, functionType);

        // Reset mappings
        context->mappings = (TypeReferenceHashmap){0};
    }
}

static void codegenFunction(
    Context* context, Function* function, Arena scratch
) {
    for (uint32_t i = 0; i < list_size(function->instantiations); i++) {
        String mangledName = getMangledFunctionNameAndMappings(
            &scratch,
            context,
            function,
            i,
            &context->mappings
        );

        // Get function
        LLVMValueRef llvmFunction =
            LLVMGetNamedFunction(context->llvmModule, mangledName.buffer);
        assert(llvmFunction);

        // Create body
        LLVMBasicBlockRef body = LLVMAppendBasicBlockInContext(
            context->llvmContext,
            llvmFunction,
            "entry"
        );

        // Set builder at body
        LLVMPositionBuilderAtEnd(context->llvmBuilder, body);

        if (function->builtin) {
            if (list_size(function->arguments) == 2) {
                // Left
                LLVMValueRef arg1 = LLVMGetParam(llvmFunction, 0);
                LLVMTypeRef arg1Type = LLVMTypeOf(arg1);
                LLVMValueRef arg1Allocation =
                    LLVMBuildAlloca(context->llvmBuilder, arg1Type, "");
                LLVMBuildStore(context->llvmBuilder, arg1, arg1Allocation);
                LLVMValueRef arg1Value = LLVMBuildLoad2(
                    context->llvmBuilder,
                    arg1Type,
                    arg1Allocation,
                    ""
                );

                // Right
                LLVMValueRef arg2 = LLVMGetParam(llvmFunction, 1);
                LLVMTypeRef arg2Type = LLVMTypeOf(arg2);
                LLVMValueRef arg2Allocation =
                    LLVMBuildAlloca(context->llvmBuilder, arg2Type, "");
                LLVMBuildStore(context->llvmBuilder, arg2, arg2Allocation);
                LLVMValueRef arg2Value = LLVMBuildLoad2(
                    context->llvmBuilder,
                    arg2Type,
                    arg2Allocation,
                    ""
                );

                if (function->identifier ==
                    ast_getLiteral(context->module, "==")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealUEQ,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntEQ,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, "!=")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealUNE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntNE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, "<")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealULT,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntULT,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, "<=")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealULE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntULE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, ">")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealUGT,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntUGT,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, ">=")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFCmp(
                            context->llvmBuilder,
                            LLVMRealUGE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildICmp(
                            context->llvmBuilder,
                            LLVMIntUGE,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }

                } else if (function->identifier ==
                           ast_getLiteral(context->module, "+")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFAdd(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildAdd(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }

                } else if (function->identifier ==
                           ast_getLiteral(context->module, "-")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFSub(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildSub(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }

                } else if (function->identifier ==
                           ast_getLiteral(context->module, "*")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFMul(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildMul(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }

                } else if (function->identifier ==
                           ast_getLiteral(context->module, "/")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value = LLVMBuildFDiv(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildSDiv(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, "%")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(arg1Type);
                    if (kind == LLVMIntegerTypeKind &&
                        LLVMGetIntTypeWidth(arg1Type) == 64) {
                        LLVMValueRef value = LLVMBuildSRem(
                            context->llvmBuilder,
                            arg1Value,
                            arg2Value,
                            ""
                        );
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                } else {
                    unreachable();
                }
            } else if (list_size(function->arguments) == 1) {
                LLVMValueRef arg = LLVMGetParam(llvmFunction, 0);
                LLVMTypeRef argType = LLVMTypeOf(arg);
                LLVMValueRef argAllocation =
                    LLVMBuildAlloca(context->llvmBuilder, argType, "");
                LLVMBuildStore(context->llvmBuilder, arg, argAllocation);
                LLVMValueRef argValue = LLVMBuildLoad2(
                    context->llvmBuilder,
                    argType,
                    argAllocation,
                    ""
                );

                if (function->identifier ==
                    ast_getLiteral(context->module, "print")) {
                    LLVMTypeRef int32Type =
                        LLVMInt32TypeInContext(context->llvmContext);
                    LLVMTypeRef int8Type =
                        LLVMInt8TypeInContext(context->llvmContext);
                    LLVMTypeRef charPointerType = LLVMPointerType(int8Type, 0);
                    LLVMTypeRef printfParams[] = {charPointerType};
                    LLVMTypeRef printfType =
                        LLVMFunctionType(int32Type, printfParams, 1, 1);
                    LLVMValueRef printfFunction =
                        LLVMGetNamedFunction(context->llvmModule, "printf");
                    if (printfFunction == NULL) {
                        printfFunction = LLVMAddFunction(
                            context->llvmModule,
                            "printf",
                            printfType
                        );
                    }

                    LLVMTypeKind kind = LLVMGetTypeKind(argType);
                    if (kind == LLVMDoubleTypeKind) {
                        // Create string format
                        char* stringFormat = "%g\n";
                        LLVMValueRef llvmStringFormat = LLVMBuildGlobalString(
                            context->llvmBuilder,
                            stringFormat,
                            ""
                        );

                        LLVMValueRef printfArgs[] = {
                            llvmStringFormat,
                            argValue
                        };
                        (void)LLVMBuildCall2(
                            context->llvmBuilder,
                            printfType,
                            printfFunction,
                            printfArgs,
                            2,
                            ""
                        );
                        LLVMBuildRetVoid(context->llvmBuilder);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(argType) == 64) {
                        // Create string format
                        char* stringFormat = "%d\n";
                        LLVMValueRef llvmStringFormat = LLVMBuildGlobalString(
                            context->llvmBuilder,
                            stringFormat,
                            ""
                        );

                        LLVMValueRef printfArgs[] = {
                            llvmStringFormat,
                            argValue
                        };
                        (void)LLVMBuildCall2(
                            context->llvmBuilder,
                            printfType,
                            printfFunction,
                            printfArgs,
                            2,
                            ""
                        );
                        LLVMBuildRetVoid(context->llvmBuilder);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(argType) == 1) {
                        LLVMBasicBlockRef then_bb =
                            LLVMAppendBasicBlockInContext(
                                context->llvmContext,
                                llvmFunction,
                                ""
                            );
                        LLVMBasicBlockRef else_bb =
                            LLVMAppendBasicBlockInContext(
                                context->llvmContext,
                                llvmFunction,
                                ""
                            );
                        LLVMBasicBlockRef merge_bb =
                            LLVMAppendBasicBlockInContext(
                                context->llvmContext,
                                llvmFunction,
                                ""
                            );

                        LLVMBuildCondBr(
                            context->llvmBuilder,
                            argValue,
                            then_bb,
                            else_bb
                        );

                        LLVMPositionBuilderAtEnd(context->llvmBuilder, then_bb);

                        // Then
                        char* stringFormat = "true\n";
                        LLVMValueRef llvmStringFormat = LLVMBuildGlobalString(
                            context->llvmBuilder,
                            stringFormat,
                            ""
                        );
                        LLVMValueRef printfArgs[] = {
                            llvmStringFormat,
                            argValue
                        };
                        (void)LLVMBuildCall2(
                            context->llvmBuilder,
                            printfType,
                            printfFunction,
                            printfArgs,
                            2,
                            ""
                        );
                        LLVMBuildBr(context->llvmBuilder, merge_bb);

                        // Else
                        LLVMPositionBuilderAtEnd(context->llvmBuilder, else_bb);
                        stringFormat = "false\n";
                        llvmStringFormat = LLVMBuildGlobalString(
                            context->llvmBuilder,
                            stringFormat,
                            ""
                        );
                        printfArgs[0] = llvmStringFormat;
                        printfArgs[1] = argValue;
                        (void)LLVMBuildCall2(
                            context->llvmBuilder,
                            printfType,
                            printfFunction,
                            printfArgs,
                            2,
                            ""
                        );
                        LLVMBuildBr(context->llvmBuilder, merge_bb);

                        // Merge
                        LLVMPositionBuilderAtEnd(
                            context->llvmBuilder,
                            merge_bb
                        );
                        LLVMBuildRetVoid(context->llvmBuilder);
                    } else {
                        todo();
                    }
                } else if (function->identifier ==
                           ast_getLiteral(context->module, "-'")) {
                    LLVMTypeKind kind = LLVMGetTypeKind(argType);
                    if (kind == LLVMDoubleTypeKind) {
                        LLVMValueRef value =
                            LLVMBuildFNeg(context->llvmBuilder, argValue, "");
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else if (kind == LLVMIntegerTypeKind &&
                               LLVMGetIntTypeWidth(argType) == 64) {
                        LLVMValueRef value =
                            LLVMBuildNeg(context->llvmBuilder, argValue, "");
                        LLVMBuildRet(context->llvmBuilder, value);
                    } else {
                        todo();
                    }
                }
            } else {
                unreachable();
            }
        } else {
            addArgumentBinding(&context->arena, &context->scopes, 0, NULL);
            todo();
        }
    }

    // Reset mappings
    context->mappings = (TypeReferenceHashmap){0};
}

static void codegenCode(Context* context, Code* code, Arena scratch) {
    for (uint32_t id = 1; id <= list_size(code->instructions); id++) {
        codegenInstruction(context, code, id, scratch);
    }
}

static void codegen(
    Context* context, Ast* ast, uint32_t astId, bool isEntry, Arena scratch
) {
    addScope(&context->arena, &context->scopes);
    addTopLevelBindings(
        &context->arena,
        &context->scopes,
        &context->program->builtin,
        context->module,
        context->moduleId
    );

    LLVMTypeRef int32Type = LLVMInt32TypeInContext(context->llvmContext);

    // Generate function prototypes
    for (uint32_t i = 0; i < list_size(ast->functions); i++) {
        Function* function = list_get(ast->functions, i);
        codegenFunctionPrototype(context, function, scratch);
    }

    // Generate function bodies
    for (uint32_t i = 0; i < list_size(ast->functions); i++) {
        Function* function = list_get(ast->functions, i);
        codegenFunction(context, function, scratch);
    }

    if (isEntry) {
        // Create main function
        LLVMTypeRef mainType = LLVMFunctionType(int32Type, NULL, 0, 0);

        LLVMValueRef mainFunc =
            LLVMAddFunction(context->llvmModule, "main", mainType);
        LLVMSetLinkage(mainFunc, LLVMExternalLinkage);

        // Create entry block
        LLVMBasicBlockRef entryBlock = LLVMAppendBasicBlockInContext(
            context->llvmContext,
            mainFunc,
            "entry"
        );
        LLVMPositionBuilderAtEnd(context->llvmBuilder, entryBlock);
        context->currentBlockEntry = entryBlock;

        // Codegen statements
        codegenCode(context, &context->module->code, scratch);

        // Create return 0 statement
        LLVMValueRef returnValue = LLVMConstInt(int32Type, 0, 0);
        LLVMBuildRet(context->llvmBuilder, returnValue);
    }

    removeScope(&context->scopes);
}

void generateObjectCode(
    Program* program,
    uint32_t astId,
    String objectFileName,
    String path,
    bool isEntry,
    Arena scratch
) {
    Ast* ast = program_getAst(program, astId);
    char* error = NULL;

    // Initialize context
    Context context = {0};
    init_context(&context, program, ast, astId, path, arena_new());

    // Codegen
    codegen(&context, ast, astId, isEntry, scratch);

    // Initialize code generation
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    // Get target triple and get target
    char* triple = "arm64-apple-macos11.0.0";
    LLVMTargetRef target;
    error = NULL;
    LLVMGetTargetFromTriple(triple, &target, &error);
    if (error != NULL) {
        printf("%s\n", error);
        LLVMDisposeMessage(error);
        unreachable();
    }

    // Select CPU features
    char* cpu = "generic";
    char* features = "";

    // Create target machine
    LLVMTargetMachineRef targetMachine = LLVMCreateTargetMachine(
        target,
        triple,
        cpu,
        features,
        LLVMCodeGenLevelNone,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    // Configure module
    LLVMTargetDataRef dataLayout = LLVMCreateTargetDataLayout(targetMachine);
    LLVMSetModuleDataLayout(context.llvmModule, dataLayout);
    LLVMSetTarget(context.llvmModule, triple);

    // Generate object code
    error = NULL;
    LLVMBool errorOcurred = LLVMTargetMachineEmitToFile(
        targetMachine,
        context.llvmModule,
        objectFileName.buffer,
        LLVMObjectFile,
        &error
    );
    if (errorOcurred) {
        printf("%s\n", error);
        LLVMDisposeMessage(error);
        unreachable();
    }

    // Free resources
    LLVMDisposeTargetMachine(targetMachine);
    LLVMDisposeBuilder(context.llvmBuilder);
    LLVMDisposeModule(context.llvmModule);
    LLVMContextDispose(context.llvmContext);

    // Free arena
    arena_free(&context.arena);
}

void linkObjectFiles(
    String executableName, StringList objectFiles, Arena scratch
) {
    switch (currentPlatform()) {
    case Windows: todo();
    case Linux: todo();
    case MacOS: {
        String macosVersion = {0};
        string_concat(&scratch, &macosVersion, cStringAsView("11.0.0"));

        // Create link args
        CStringList args = {0};
        list_append(&scratch, args, "lld");
        for (uint32_t i = 0; i < list_size(objectFiles); i++) {
            list_append(&scratch, args, list_get(objectFiles, i)->buffer);
        }
        list_append(&scratch, args, "-o");
        list_append(&scratch, args, executableName.buffer);
        list_append(&scratch, args, "-arch");
        list_append(&scratch, args, "arm64");
        list_append(&scratch, args, "-platform_version");
        list_append(&scratch, args, "macos");
        list_append(&scratch, args, macosVersion.buffer);
        list_append(&scratch, args, macosVersion.buffer);
        String workingDirectory = getWorkingDirectory(&scratch);
        string_concat(
            &scratch,
            &workingDirectory,
            cStringAsView("/libc/macOS/libSystem.tbd")
        );
        list_append(&scratch, args, workingDirectory.buffer);

        // Link
        lld_link(args);
        break;
    }
    }
}

void printLLVMIR(Program* program, uint32_t astId, String path, Arena scratch) {
    Ast* ast = program_getAst(program, astId);
    Context context = {0};
    init_context(&context, program, ast, astId, path, arena_new());
    codegen(&context, ast, astId, astId == 1, scratch);
    LLVMDumpModule(context.llvmModule);
    arena_free(&context.arena);
}