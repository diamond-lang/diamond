#include <iostream>
#include <unordered_map>
#include <assert.h>

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

#include "codegen.hpp"


struct Codegen {
	llvm::LLVMContext* context;
	llvm::Module* module;
	llvm::IRBuilder<>* builder;
	std::unordered_map<std::string, llvm::Value*> scope;

	Codegen() {
		this->context = new llvm::LLVMContext();
		this->module = new llvm::Module("My cool jit", *(this->context));
		this->builder = new llvm::IRBuilder(*(this->context));
	}

	void codegen(std::shared_ptr<Ast::Program> node);
	void codegen(std::shared_ptr<Ast::Assignment> node);
	llvm::Value* codegen_expression(std::shared_ptr<Ast::Node> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Call> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Number> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Identifier> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Boolean> node);
};

void generate_executable(std::shared_ptr<Ast::Program> program, std::string executable_name) {
	Codegen llvm_ir;
	llvm_ir.codegen(program);

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

	llvm_ir.module->setDataLayout(TargetMachine->createDataLayout());
	llvm_ir.module->setTargetTriple(TargetTriple);

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

	pass.run(*(llvm_ir.module));
	dest.flush();

	std::string command = "clang-12 -o ";
	command += executable_name;
	command += " ";
	command += object_file_name;
	command += " && rm ";
	command += object_file_name;
	system(command.c_str());
}

void Codegen::codegen(std::shared_ptr<Ast::Program> node) {
	// Declare printf
	std::vector<llvm::Type*> args;
	args.push_back(llvm::Type::getInt8PtrTy(*(this->context)));
	llvm::FunctionType *printfType = llvm::FunctionType::get(this->builder->getInt32Ty(), args, true); // `true` specifies the function as variadic
	llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", this->module);

	// Crate main function
	llvm::FunctionType* mainType = llvm::FunctionType::get(this->builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", this->module);
	llvm::BasicBlock* entry = llvm::BasicBlock::Create(*(this->context), "entry", main);
	this->builder->SetInsertPoint(entry);

	// Create global string for printing doubles
	llvm::Value* format_float = this->builder->CreateGlobalStringPtr("%g\n");
	llvm::Value* format_integer = this->builder->CreateGlobalStringPtr("%d\n");
	llvm::Value* format_true = this->builder->CreateGlobalStringPtr("true\n");
	llvm::Value* format_false = this->builder->CreateGlobalStringPtr("false\n");
	llvm::Function* print_function = this->module->getFunction("printf");

	if (!print_function) {
		std::cout << "No print funciont :(" << '\n';
	}

	for (size_t i = 0; i < node->statements.size(); i++) {
		if (node->statements[i]->is_expression()) {
			llvm::Value* value = this->codegen_expression(node->statements[i]);

			if (value != nullptr) {
				if (value->getType()->isDoubleTy()) {
					std::vector<llvm::Value*> printArgs;
					printArgs.push_back(format_float);
					printArgs.push_back(value);
					this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
				}
				else if (value->getType()->isIntegerTy(1)) {
					std::vector<llvm::Value*> printArgs;

					llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
					llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
					llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
					llvm::BasicBlock *merge = llvm::BasicBlock::Create(*(this->context), "ifcont");
					this->builder->CreateCondBr(value, then_block, else_block);

					// Create then branch
					this->builder->SetInsertPoint(then_block);
					printArgs.push_back(format_true);
					this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
					this->builder->CreateBr(merge);
					then_block = this->builder->GetInsertBlock();

					printArgs.clear();

					// Create else branch
					current_function->getBasicBlockList().push_back(else_block);
					this->builder->SetInsertPoint(else_block);
					printArgs.push_back(format_false);
					this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
					this->builder->CreateBr(merge);
					else_block = this->builder->GetInsertBlock();

					// Merge  block
					current_function->getBasicBlockList().push_back(merge);
					this->builder->SetInsertPoint(merge);
				}
			}
			else {
				std::cout << "Value is null :(" << '\n';
			}
		}
		else if (std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i]));
		}
	}

	// Create return statement
	this->builder->CreateRet(llvm::ConstantInt::get(*(this->context), llvm::APInt(32, 0)));
}

void Codegen::codegen(std::shared_ptr<Ast::Assignment> node) {
	// Generate value of expression
	llvm::Value* expr = this->codegen_expression(node->expression);

	// Add it to the scope
	this->scope[node->identifier->value] = expr;
}

llvm::Value* Codegen::codegen_expression(std::shared_ptr<Ast::Node> node) {
	if      (std::dynamic_pointer_cast<Ast::Call>(node))       return this->codegen(std::dynamic_pointer_cast<Ast::Call>(node));
	else if (std::dynamic_pointer_cast<Ast::Number>(node))     return this->codegen(std::dynamic_pointer_cast<Ast::Number>(node));
	else if (std::dynamic_pointer_cast<Ast::Identifier>(node)) return this->codegen(std::dynamic_pointer_cast<Ast::Identifier>(node));
	else if (std::dynamic_pointer_cast<Ast::Boolean>(node))    return this->codegen(std::dynamic_pointer_cast<Ast::Boolean>(node));

	assert(false);
	return nullptr;
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Call> node) {
	llvm::Value* left = this->codegen_expression(node->args[0]);
	llvm::Value* right = this->codegen_expression(node->args[1]);
	if (node->identifier->value == "+") return this->builder->CreateFAdd(left, right, "addtmp");
	if (node->identifier->value == "-") return this->builder->CreateFSub(left, right, "subtmp");
	if (node->identifier->value == "*") return this->builder->CreateFMul(left, right, "multmp");
	if (node->identifier->value == "/") return this->builder->CreateFDiv(left, right, "divtmp");
	if (node->identifier->value == "<") return this->builder->CreateFCmpULT(left, right, "cmptmp");
	if (node->identifier->value == "<=") return this->builder->CreateFCmpULE(left, right, "cmptmp");
	if (node->identifier->value == ">") return this->builder->CreateFCmpUGT(left, right, "cmptmp");
	if (node->identifier->value == ">=") return this->builder->CreateFCmpUGE(left, right, "cmptmp");
	if (node->identifier->value == "==") {
		if (left->getType()->isIntegerTy(1) && right->getType()->isIntegerTy(1)) {
			return this->builder->CreateICmpEQ(left, right, "eqtmp");
		}
		else if (left->getType()->isDoubleTy() && right->getType()->isDoubleTy()) {
			return this->builder->CreateFCmpUEQ(left, right, "eqtmp");
		}
	}

	assert(false);
	return nullptr;
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Number> node) {
	return llvm::ConstantFP::get(*(this->context), llvm::APFloat(node->value));
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Identifier> node) {
	return this->scope[node->value];
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Boolean> node) {
	return llvm::ConstantInt::getBool(*(this->context), node->value);
}
