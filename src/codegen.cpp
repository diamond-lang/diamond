#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <iostream>

#include "codegen.hpp"

struct CodegenVisitor : Ast::Visitor {
	llvm::LLVMContext* context;
	llvm::Module* module;
	llvm::IRBuilder<>* builder;

	virtual void visit(Ast::Program* node) override;
	virtual void visit(Ast::Number* node) override;

	CodegenVisitor(llvm::LLVMContext* context, llvm::Module* module, llvm::IRBuilder<>* builder) : context(context), module(module), builder(builder) {}
};

// Prototypes
// ==========
CodegenVisitor generate_llvm_ir(Ast::Program &program);


// Implementantions
// ================
void generate_executable(Ast::Program &program, std::string executable_name) {
	CodegenVisitor visitor = generate_llvm_ir(program);

	// Generate object file
	auto TargetTriple = llvm::sys::getDefaultTargetTriple();
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	std::string Error;
	auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

	// Print an error and exit if we couldn't find the requested target.
	// This generally occurs if we've forgotten to initialise the
	// TargetRegistry or we have a bogus target triple.
	if (!Target) {
		llvm::errs() << Error;
	}

	auto CPU = "generic";
	auto Features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

	visitor.module->setDataLayout(TargetMachine->createDataLayout());
	visitor.module->setTargetTriple(TargetTriple);

	std::string object_file_name = executable_name + ".o";

	std::error_code EC;
	llvm::raw_fd_ostream dest(object_file_name, EC, llvm::sys::fs::OF_None);

	if (EC) {
		llvm::errs() << "Could not open file: " << EC.message();
	}

	llvm::legacy::PassManager pass;
	auto FileType = llvm::CGFT_ObjectFile;

	if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
		llvm::errs() << "TargetMachine can't emit a file of this type";
	}

	pass.run(*(visitor.module));
	dest.flush();

	std::string command = "clang-12 -o ";
	command += executable_name;
	command += " ";
	command += object_file_name;
	command += " && rm ";
	command += object_file_name;
	system(command.c_str());
}

CodegenVisitor generate_llvm_ir(Ast::Program &program) {
	auto context = new llvm::LLVMContext();
	auto module = new llvm::Module("My cool jit", *context);
	auto builder = new llvm::IRBuilder(*context);
	CodegenVisitor visitor = CodegenVisitor(context, module, builder);

	// Declare printf
	std::vector<llvm::Type*> args;
	args.push_back(llvm::Type::getInt8PtrTy(*context));
	llvm::FunctionType *printfType = llvm::FunctionType::get(builder->getInt32Ty(), args, true); // `true` specifies the function as variadic
	llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", module);

	// Generate program
	program.accept(&visitor);

	return visitor;
}

void CodegenVisitor::visit(Ast::Program* node) {
	// Crate main function
	llvm::FunctionType* mainType = llvm::FunctionType::get(builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", this->module);
	llvm::BasicBlock* entry = llvm::BasicBlock::Create(*(this->context), "entry", main);
	this->builder->SetInsertPoint(entry);

	for (size_t i = 0; i < node->expressions.size(); i++) {
		node->expressions[i]->accept(this);
	}

	// Create return statement
	builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
}

void CodegenVisitor::visit(Ast::Number* node) {
	llvm::Value* value = llvm::ConstantFP::get(*(this->context), llvm::APFloat(node->value));
	llvm::Function* print_function = this->module->getFunction("printf");

	if (!print_function) {
		std::cout << "No print funciont :(" << '\n';
	}

	// Format string
	std::vector<llvm::Value*> printArgs;
	llvm::Value* format_str = this->builder->CreateGlobalStringPtr("%g\n");
	printArgs.push_back(format_str);
	printArgs.push_back(value);
	this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
}
