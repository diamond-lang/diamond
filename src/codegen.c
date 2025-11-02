#include "codegen.h"

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
#include "types.h"
#include "utilities.h"

typedef enum {
    FUNCTION_BINDING,
    INTERFACE_BINDING,
    VARIABLE_BINDING,
    ARGUMENT_BINDING
} BindingKind;

typedef struct {
    uint32_t id;
    uint32_t module;
} FunctionBinding;

typedef struct {
    uint32_t id;
    uint32_t module;
} InterfaceBinding;

typedef struct {
    LLVMValueRef value;
} VariableBinding;

typedef struct {
    LLVMValueRef value;
} ArgumentBinding;

typedef struct {
    BindingKind kind;
    uint32_t literal;
    union {
        FunctionBinding asFunction;
        InterfaceBinding asInterface;
        VariableBinding asVariable;
        ArgumentBinding asArgument;
    };
} Binding;

typedef StackType(Binding) BindingStack;

typedef struct {
    uint32_t literal;
    uint32_t id;
    uint32_t module;
} TypeBinding;

typedef ListType(TypeBinding) TypeBindingList;

typedef struct {
    BindingStack bindings;
    Uint32Stack scopeStart;
    TypeBindingList types;
} Scopes;

typedef struct {
    uint32_t literal;
    LLVMValueRef ref;
} GlobalString;

typedef ListType(GlobalString) GlobalStringList;

typedef StackType(LLVMValueRef) LLVMValueStack;

typedef struct {
    Ast* ast;
    uint32_t astId;
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
} Context;

static void addScope(Context* context) {
    stack_push(
        &context->arena,
        context->scopes.scopeStart,
        stack_size(context->scopes.bindings)
    );
}

static void removeScope(Context* context) {
    context->scopes.bindings.count = *stack_top(context->scopes.scopeStart);
    stack_pop(context->scopes.scopeStart);
}

static Binding* getBinding(Context* context, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(context->scopes.bindings);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(context->scopes.bindings, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

static Binding* getBindingInCurrentScope(Context* context, uint32_t literalId) {
    uint32_t start = *stack_top(context->scopes.scopeStart);
    uint32_t bindingsCount = stack_size(context->scopes.bindings);
    assert(bindingsCount >= start);
    for (uint32_t i = bindingsCount - 1; start <= i && i < bindingsCount; i--) {
        Binding* binding = stack_get(context->scopes.bindings, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

// static void addArgumentBinding(
//     Context* context, uint32_t literal, LLVMValueRef value
// ) {
//     Binding binding = {
//         .kind = ARGUMENT_BINDING,
//         .literal = literal,
//         .asArgument = (ArgumentBinding){.value = value}
//     };
//     stack_push(&context->arena, context->scopes.bindings, binding);
// }

// static void scopes_addVariableBinding(
//     Arena* arena, Scopes* scopes, uint32_t literalId, uint32_t type
// ) {
//     Binding binding = {
//         .kind = VARIABLE_BINDING,
//         .identifier = literalId,
//         .type = type
//     };
//     stack_push(arena, scopes->bindings, binding);
// }

static void addFunctionBinding(
    Context* context, uint32_t literal, uint32_t id, uint32_t module
) {
    Binding binding = {
        .kind = FUNCTION_BINDING,
        .literal = literal,
        .asFunction = (FunctionBinding){.id = id, .module = module}
    };
    stack_push(&context->arena, context->scopes.bindings, binding);
}

static void addInterfaceBinding(
    Context* context, uint32_t literalId, uint32_t id, uint32_t module
) {
    Binding binding = {
        .kind = INTERFACE_BINDING,
        .literal = literalId,
        .asInterface = (InterfaceBinding){.id = id, .module = module}
    };
    stack_push(&context->arena, context->scopes.bindings, binding);
}

static void addTypeBinding(
    Context* context, uint32_t literal, uint32_t id, uint32_t module
) {
    TypeBinding binding = {.literal = literal, .id = id, .module = module};
    stack_push(&context->arena, context->scopes.types, binding);
}

static TypeBinding* getTypeBinding(Context* context, uint32_t literalId) {
    uint32_t bindingsCount = stack_size(context->scopes.types);
    for (uint32_t i = bindingsCount - 1; 0 <= i && i < bindingsCount; i--) {
        TypeBinding* binding = stack_get(context->scopes.types, i);
        if (binding->literal == literalId) return binding;
    }
    return NULL;
}

// static LLVMValueRef getBindingLLVMValueRef(Context* context, Binding binding) {
//     todo();
// }

static bool addTopLevelBindings(Context* context, uint32_t astId) {
    // Add builtin type bindings
    Ast* builtin = &context->program->builtin;
    for (uint32_t i = 0; i < list_size(builtin->typeDefinitions); i++) {
        TypeDefinition* typeDef = list_get(builtin->typeDefinitions, i);
        char* literal = ast_literalAsString(*builtin, typeDef->identifier);
        uint32_t identifier = ast_getLiteral(context->ast, literal);
        TypeBinding* binding = getTypeBinding(context, identifier);
        if (binding != NULL) {
            todo();
        }
        addTypeBinding(context, identifier, i, 0);
    }

    // Add builtin interface bindings
    for (uint32_t i = 0; i < list_size(builtin->interfaces); i++) {
        Interface* interface = list_get(builtin->interfaces, i);
        char* literal = ast_literalAsString(*builtin, interface->identifier);
        uint32_t identifier = ast_getLiteral(context->ast, literal);
        Binding* binding = getBindingInCurrentScope(context, identifier);
        if (binding != NULL) {
            todo();
        }
        addInterfaceBinding(context, identifier, i, 0);
    }

    // Add builtin function bindings
    for (uint32_t i = 0; i < list_size(builtin->functions); i++) {
        Function* function = list_get(builtin->functions, i);
        char* literal = ast_literalAsString(*builtin, function->identifier);
        uint32_t identifier = ast_getLiteral(context->ast, literal);
        if (function->isImplementation) continue;
        Binding* binding = getBindingInCurrentScope(context, identifier);
        if (binding != NULL) {
            todo();
        }
        addFunctionBinding(context, identifier, i, 0);
    }

    // Add types bindings
    for (uint32_t i = 0; i < list_size(context->ast->typeDefinitions); i++) {
        TypeDefinition typeDef = *list_get(context->ast->typeDefinitions, i);
        TypeBinding* binding = getTypeBinding(context, typeDef.identifier);
        assert(binding == NULL);
        addTypeBinding(context, typeDef.identifier, i, astId);
    }

    // Add interface bindings
    for (uint32_t i = 0; i < list_size(context->ast->interfaces); i++) {
        Interface* interface = list_get(context->ast->interfaces, i);
        Binding* binding =
            getBindingInCurrentScope(context, interface->identifier);
        assert(binding == NULL);
        addInterfaceBinding(context, interface->identifier, i, astId);
    }

    // Add function bindings
    for (uint32_t i = 0; i < list_size(context->ast->functions); i++) {
        Function* function = list_get(context->ast->functions, i);
        if (function->isImplementation) continue;
        Binding* binding =
            getBindingInCurrentScope(context, function->identifier);
        assert(binding == NULL);
        addFunctionBinding(context, function->identifier, i, astId);
    }

    return true;
}

static void init_context(
    Context* context, Program* program, Ast* ast, uint32_t astId, Arena arena
) {
    context->ast = ast;
    context->astId = astId;
    context->program = program;
    context->arena = arena;
    context->llvmContext = LLVMContextCreate();
    context->llvmModule = LLVMModuleCreateWithNameInContext(
        list_get(program->paths, 0)->buffer,
        context->llvmContext
    );
    context->llvmBuilder = LLVMCreateBuilderInContext(context->llvmContext);
}

static LLVMTypeRef getTypeAsLLVMType(
    Arena* arena, Context* context, uint32_t typeId
) {
    assert(typeId != None());
    Type* type = ast_findType(context->ast, typeId);
    switch (type->kind) {
    case TYPE_VARIABLE: unreachable();
    case TYPE_WITH_PARAMS: {
        if (ast_isTypeVariable(*context->ast, &type->withParams)) {
            unreachable();
        }

        TypeWithParams* data = &type->withParams;
        if (data->literal == ast_getLiteral(context->ast, "Bool")) {
            return LLVMInt1TypeInContext(context->llvmContext);
        } else if (data->literal == ast_getLiteral(context->ast, "Float64")) {
            return LLVMDoubleTypeInContext(context->llvmContext);
        } else if (data->literal == ast_getLiteral(context->ast, "None")) {
            return LLVMVoidTypeInContext(context->llvmContext);
        } else if (data->literal == ast_getLiteral(context->ast, "->")) {
            assert(data->parameterCount >= 1);
            uint32_t argsCount = data->parameterCount - 1;
            LLVMTypeRef* args = arena_alloc(arena, LLVMTypeRef, argsCount);
            for (uint32_t i = 0; i < argsCount; i++) {
                args[i] =
                    getTypeAsLLVMType(arena, context, data->parameters[i]);
            }
            LLVMTypeRef returnType =
                getTypeAsLLVMType(arena, context, data->parameters[argsCount]);
            return LLVMFunctionType(returnType, args, argsCount, false);
        } else {
            todo();
        }
    }
    }
    unreachable();
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
    Type* actualType = ast_getType(*context->ast, actualTypeId);
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
            ast_findType(context->ast, actualType->withParams.parameters[i]);
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
                *context->ast,
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
            getTypeAsLLVMType(&scratch, context, actualTypeId);
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
    sscanf(ast_literalAsString(*context->ast, data->literal), "%lf", &value);
    LLVMValueRef llvmValue =
        LLVMConstReal(LLVMDoubleTypeInContext(context->llvmContext), value);
    stack_push(&context->arena, context->stack, llvmValue);
}

static void integerLiteral(
    Context* context, Code* code, uint32_t id, AstInteger* data, Arena scratch
) {
    todo();
}

static void identifier(
    Context* context,
    Code* code,
    uint32_t id,
    AstIdentifier* data,
    Arena scratch
) {
    Binding* binding = getBinding(context, data->literal);
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
    todo();
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
    void* data = ast_getData(context->ast, code, id);
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

static void codengenFunctionPrototype(
    Context* context, Function* function, Arena scratch
) {
    if (function->builtin || function->parameters.count != 0) return;
    LLVMTypeRef functionType =
        getTypeAsLLVMType(&scratch, context, function->type);
    char* identifier = ast_literalAsString(*context->ast, function->identifier);
    LLVMAddFunction(context->llvmModule, identifier, functionType);
}

static void codengenFunction(
    Context* context, Function* function, Arena scratch
) {
    if (function->builtin || function->parameters.count != 0) return;
    char* identifier = ast_literalAsString(*context->ast, function->identifier);
    LLVMValueRef llvmFunction =
        LLVMGetNamedFunction(context->llvmModule, identifier);
    LLVMBasicBlockRef body =
        LLVMAppendBasicBlockInContext(context->llvmContext, llvmFunction, "");
    LLVMPositionBuilderAtEnd(context->llvmBuilder, body);
    context->currentBlockEntry = LLVMGetEntryBasicBlock(llvmFunction);
    todo();
}

static void codegenCode(Context* context, Code* code, Arena scratch) {
    for (uint32_t id = 1; id <= list_size(code->instructions); id++) {
        codegenInstruction(context, code, id, scratch);
    }
}

static void codegen(
    Context* context, Ast* ast, uint32_t astId, bool isEntry, Arena scratch
) {
    addScope(context);
    addTopLevelBindings(context, astId);

    LLVMTypeRef int32Type = LLVMInt32TypeInContext(context->llvmContext);
    // LLVMTypeRef int8Type = LLVMInt8TypeInContext(context->llvmContext);
    // LLVMTypeRef charPointerType = LLVMPointerType(int8Type, 0);

    // // Create printf declaration
    // LLVMTypeRef printfParams[] = {charPointerType};
    // LLVMTypeRef printfType = LLVMFunctionType(int32Type, printfParams, 1, 1);
    // LLVMValueRef printfFunction =
    //     LLVMAddFunction(context->llvmModule, "printf", printfType);

    if (isEntry) {
        // Generate function prototypes
        for (uint32_t i = 0; i < list_size(ast->functions); i++) {
            Function* function = list_get(ast->functions, i);
            codengenFunctionPrototype(context, function, scratch);
        }

        // Generate function bodies
        for (uint32_t i = 0; i < list_size(ast->functions); i++) {
            Function* function = list_get(ast->functions, i);
            codengenFunction(context, function, scratch);
        }

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
        codegenCode(context, &context->ast->code, scratch);

        // // Create hello world constant
        // char* helloWorld = "Hello world!\n";
        // LLVMValueRef helloConstant =
        //     LLVMBuildGlobalString(context->llvmBuilder, helloWorld, "constant");
        // LLVMValueRef printfArgs[] = {helloConstant};
        // (void)LLVMBuildCall2(
        //     context->llvmBuilder,
        //     printfType,
        //     printfFunction,
        //     printfArgs,
        //     1,
        //     "call"
        // );

        // Create return 0 statement
        LLVMValueRef returnValue = LLVMConstInt(int32Type, 0, 0);
        LLVMBuildRet(context->llvmBuilder, returnValue);
    }

    removeScope(context);
}

void generateObjectCode(
    Program* program,
    uint32_t astId,
    String objectFileName,
    bool isEntry,
    Arena scratch
) {
    Ast* ast = NULL;
    if (astId == 0) {
        ast = &program->builtin;
    } else {
        ast = program_getAst(program, astId);
    }
    char* error = NULL;

    // Initialize context
    Context context = {0};
    init_context(&context, program, ast, astId, arena_new());

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

void printLLVMIR(Program* program, uint32_t astId, Arena scratch) {
    Ast* ast = program_getAst(program, astId);
    Context context = {0};
    init_context(&context, program, ast, astId, arena_new());
    codegen(&context, ast, astId, true, scratch);
    LLVMDumpModule(context.llvmModule);
    arena_free(&context.arena);
}