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
#include "scopes.h"
#include "types.h"
#include "utilities.h"

typedef struct {
    uint32_t literal;
    LLVMValueRef ref;
} GlobalString;

typedef ListType(GlobalString) GlobalStringList;

typedef struct {
    Ast* ast;
    Scopes bindings;
    Uint32Stack stack;
    uint32_t lastTypeVariable;
    Arena arena;
    LLVMContextRef llvmContext;
    LLVMModuleRef llvmModule;
    LLVMBuilderRef llvmBuilder;
    LLVMBasicBlockRef currentEntryBlock;
    LLVMBasicBlockRef lastAfterWhileBlock;  // Needed for break
    LLVMBasicBlockRef lastWhileBlock;       // Needed for continue
    GlobalStringList globalStrings;
} Context;

static void init_context(
    Context* context, Program program, Ast* ast, Arena arena
) {
    context->ast = ast;
    context->arena = arena;
    scopes_addScope(&context->arena, &context->bindings);
    context->llvmContext = LLVMContextCreate();
    context->llvmModule = LLVMModuleCreateWithNameInContext(
        list_get(program.paths, 0)->buffer,
        context->llvmContext
    );
    context->llvmBuilder = LLVMCreateBuilderInContext(context->llvmContext);
}

static bool declaration(
    Context* ctx, Code* code, uint32_t id, AstDeclaration* data, Arena scratch
) {
    todo();
}

static bool assignemnt(
    Context* context,
    Code* code,
    uint32_t id,
    AstAssignment* data,
    Arena scratch
) {
    todo();
}

static bool returnStmt(
    Context* context, Code* code, uint32_t id, AstReturn* data, Arena scratch
) {
    todo();
}

static bool returnExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstReturnLastExpression* data,
    Arena scratch
) {
    todo();
}

static bool breakStmt(
    Context* context, Code* code, uint32_t id, AstBreak* data, Arena scratch
) {
    todo();
}

static bool continueStmt(
    Context* context, Code* code, uint32_t id, AstContinue* data, Arena scratch
) {
    todo();
}

static bool ifElse(
    Context* context, Code* code, uint32_t id, AstIfElse* data, Arena scratch
) {
    todo();
}

static bool whileStmt(
    Context* context, Code* code, uint32_t id, AstWhile* data, Arena scratch
) {
    todo();
}

static bool call(
    Context* context, Code* code, uint32_t id, AstCall* data, Arena scratch
) {
    todo();
}

static bool ifElseExpression(
    Context* context,
    Code* code,
    uint32_t id,
    AstIfElseExpression* data,
    Arena scratch
) {
    todo();
}

static bool addressOf(
    Context* context, Code* code, uint32_t id, AstAddressOf* data, Arena scratch
) {
    todo();
}

static bool dereference(
    Context* context,
    Code* code,
    uint32_t id,
    AstDereference* data,
    Arena scratch
) {
    todo();
}

static bool fieldAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstFieldAccess* data,
    Arena scratch
) {
    todo();
}

static bool indexAccess(
    Context* context,
    Code* code,
    uint32_t id,
    AstIndexAccess* data,
    Arena scratch
) {
    todo();
}

static bool floatLiteral(
    Context* context, Code* code, uint32_t id, AstFloat* data, Arena scratch
) {
    todo();
}

static bool integerLiteral(
    Context* context, Code* code, uint32_t id, AstInteger* data, Arena scratch
) {
    todo();
}

static bool identifier(
    Context* context,
    Code* code,
    uint32_t id,
    AstIdentifier* data,
    Arena scratch
) {
    todo();
}

static bool booleanLiteral(
    Context* context, Code* code, uint32_t id, AstBoolean* data, Arena scratch
) {
    todo();
}

static bool stringLiteral(
    Context* context, Code* code, uint32_t id, AstString* data, Arena scratch
) {
    todo();
}

static bool arrayLiteral(
    Context* context, Code* code, uint32_t id, AstArray* data, Arena scratch
) {
    todo();
}

static bool structLiteral(
    Context* context,
    Code* code,
    uint32_t id,
    AstStructLiteral* data,
    Arena scratch
) {
    todo();
}

static bool codegenInstruction(
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

// static bool codegenTypeDefinition(
//     Context* context, TypeDefinition* typeDefinition, Arena scratch
// ) {
//     todo();
// }

// static bool codengenFunction(
//     Context* context, Function* function, Arena scratch
// ) {
//     todo();
// }

static bool codegenCode(Context* context, Code* code, Arena scratch) {
    return false;
    for (uint32_t id = 1; id <= list_size(code->instructions); id++) {
        bool result = codegenInstruction(context, code, id, scratch);
        if (!result) return false;
    }
    return true;
}

static void codegen(Context* context, Ast* ast, bool isEntry, Arena scratch) {
    LLVMTypeRef int32Type = LLVMInt32TypeInContext(context->llvmContext);
    LLVMTypeRef int8Type = LLVMInt8TypeInContext(context->llvmContext);
    LLVMTypeRef charPointerType = LLVMPointerType(int8Type, 0);

    // Create printf declaration
    LLVMTypeRef printfParams[] = {charPointerType};
    LLVMTypeRef printfType = LLVMFunctionType(int32Type, printfParams, 1, 1);
    LLVMValueRef printfFunction =
        LLVMAddFunction(context->llvmModule, "printf", printfType);

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
        context->currentEntryBlock = entryBlock;

        // Codegen statements
        codegenCode(context, &context->ast->code, scratch);

        // Create hello world constant
        char* helloWorld = "Hello world!\n";
        LLVMValueRef helloConstant =
            LLVMBuildGlobalString(context->llvmBuilder, helloWorld, "constant");
        LLVMValueRef printfArgs[] = {helloConstant};
        (void)LLVMBuildCall2(
            context->llvmBuilder,
            printfType,
            printfFunction,
            printfArgs,
            1,
            "call"
        );

        // Create return 0 statement
        LLVMValueRef returnValue = LLVMConstInt(int32Type, 0, 0);
        LLVMBuildRet(context->llvmBuilder, returnValue);
    }
}

void generateObjectCode(
    Program program,
    Ast* ast,
    String objectFileName,
    bool isEntry,
    Arena scratch
) {
    char* error = NULL;

    // Initialize context
    Context context = {0};
    init_context(&context, program, ast, arena_new());

    // Codegen
    codegen(&context, ast, isEntry, scratch);

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

void printLLVMIR(Program program, uint32_t astId, Arena scratch) {
    Ast* ast = list_get(program.asts, astId);
    Context context = {0};
    init_context(&context, program, ast, arena_new());
    codegen(&context, ast, true, scratch);
    LLVMDumpModule(context.llvmModule);
    arena_free(&context.arena);
}