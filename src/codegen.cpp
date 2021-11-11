#include <iostream>
#include <cstdio>
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

#include "lld/Common/Driver.h"

#include "codegen.hpp"
#include "utilities.hpp"

struct Codegen {
	llvm::LLVMContext* context;
	llvm::Module* module;
	llvm::IRBuilder<>* builder;
	std::vector<std::unordered_map<std::string, llvm::Value*>> scopes;
	struct {
		llvm::Value* format_float;
		llvm::Value* format_integer;
		llvm::Value* format_true;
		llvm::Value* format_false;
	} format_strings;

	Codegen() {
		this->context = new llvm::LLVMContext();
		this->module = new llvm::Module("My cool jit", *(this->context));
		this->builder = new llvm::IRBuilder(*(this->context));
	}

	void codegen(std::shared_ptr<Ast::Program> node);
	void codegen(std::shared_ptr<Ast::Block> node);
	void codegen(std::vector<std::shared_ptr<Ast::Function>> functions);
	void codegen(std::shared_ptr<Ast::Assignment> node);
	void codegen(std::shared_ptr<Ast::Return> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Expression> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Call> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Number> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Integer> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Identifier> node);
	llvm::Value* codegen(std::shared_ptr<Ast::Boolean> node);

	llvm::Type* as_llvm_type(Type type);
	std::vector<llvm::Type*> as_llvm_types(std::vector<Type> types);

	void add_scope();
	std::unordered_map<std::string, llvm::Value*>& current_scope();
	void remove_scope();
};


// Linking
// -------
static std::string get_object_file_name(std::string executable_name);
static void link(std::string executable_name, std::string object_file_name);

void generate_executable(std::shared_ptr<Ast::Program> program, std::string program_name) {
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

	// Generate object code
	std::string object_file_name = get_object_file_name(program_name);

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

	// Link
	link(utilities::get_executable_name(program_name), object_file_name);

	// Remove generated object file
	remove(object_file_name.c_str());
}

#ifdef __linux__
	static std::string get_object_file_name(std::string executable_name) {
		return executable_name + ".o";
	}

	static void link(std::string executable_name, std::string object_file_name) {
		std::string name = "-o" + executable_name;
		std::vector<const char*> args = {
			"lld",
			object_file_name.c_str(),
			name.c_str(),
			"-dynamic-linker",
			"/lib64/ld-linux-x86-64.so.2",
			"-L/usr/lib/x86_64-linux-gnu",
			"-lc",
			"/usr/lib/x86_64-linux-gnu/crt1.o",
			"/usr/lib/x86_64-linux-gnu/crti.o",
			"/usr/lib/x86_64-linux-gnu/crtn.o"
		};

		std::string output = "";
		std::string errors = "";
		llvm::raw_string_ostream output_stream(output);
		llvm::raw_string_ostream errors_stream(errors);

		bool result = lld::elf::link(args, false, output_stream, errors_stream);

		remove(object_file_name.c_str());
	}
#elif _WIN32
	static std::string get_object_file_name(std::string executable_name) {
		return executable_name + ".obj";
	}

	static void link(std::string executable_name, std::string object_file_name) {
		std::string name = "-out:" + executable_name;
		std::vector<const char*> args = {
			"lld",
			object_file_name.c_str(),
			"-defaultlib:libcmt",
			"-defaultlib:oldnames",
			"-nologo",
			name.c_str()
		};

		std::string output = "";
		std::string errors = "";
		llvm::raw_string_ostream output_stream(output);
		llvm::raw_string_ostream errors_stream(errors);

		bool result = lld::coff::link(args, false, output_stream, errors_stream);
	}
#endif


// Codegeneration
// --------------
void Codegen::codegen(std::shared_ptr<Ast::Program> node) {
	// Declare printf
	std::vector<llvm::Type*> args;
	args.push_back(llvm::Type::getInt8PtrTy(*(this->context)));
	llvm::FunctionType *printfType = llvm::FunctionType::get(this->builder->getInt32Ty(), args, true); // `true` specifies the function as variadic
	llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", this->module);

	// Codegen functions
	this->codegen(node->functions);

	// Crate main function
	llvm::FunctionType* mainType = llvm::FunctionType::get(this->builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", this->module);
	llvm::BasicBlock* entry = llvm::BasicBlock::Create(*(this->context), "entry", main);
	this->builder->SetInsertPoint(entry);

	// Add new scope
	this->add_scope();

	// Create global string for print
	this->format_strings.format_float = this->builder->CreateGlobalStringPtr("%g\n");
	this->format_strings.format_integer = this->builder->CreateGlobalStringPtr("%d\n");
	this->format_strings.format_true = this->builder->CreateGlobalStringPtr("true\n");
	this->format_strings.format_false = this->builder->CreateGlobalStringPtr("false\n");

	// Check if we have printf
	if (!this->module->getFunction("printf")) {
		std::cout << "No print funciont :(" << '\n';
	}

	// Codegen statements
	for (size_t i = 0; i < node->statements.size(); i++) {
		if (std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i]));
		}
		else if (std::dynamic_pointer_cast<Ast::Call>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Call>(node->statements[i]));
		}
		else {
			assert(false);
		}
	}

	// Create return statement
	this->builder->CreateRet(llvm::ConstantInt::get(*(this->context), llvm::APInt(32, 0)));
}

void Codegen::codegen(std::shared_ptr<Ast::Block> node) {
	for (size_t i = 0; i < node->statements.size(); i++) {
		if (std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Assignment>(node->statements[i]));
		}
		else if (std::dynamic_pointer_cast<Ast::Call>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Call>(node->statements[i]));
		}
		else if (std::dynamic_pointer_cast<Ast::Return>(node->statements[i])) {
			this->codegen(std::dynamic_pointer_cast<Ast::Return>(node->statements[i]));
		}
		else {
			assert(false);
		}
	}
}

void Codegen::codegen(std::vector<std::shared_ptr<Ast::Function>> functions) {
	for (size_t i = 0; i < functions.size(); i++) {
		auto& node = functions[i];

		// Generate prototypes
		for (size_t i = 0; i < node->specializations.size(); i++) {
			auto& specialization = node->specializations[i];
			assert(specialization->valid);

			// Make function type
			std::vector<llvm::Type*> args_types = this->as_llvm_types(specialization->args_types);
			auto return_type = this->as_llvm_type(specialization->return_type);
			llvm::FunctionType* function_type = llvm::FunctionType::get(return_type, args_types, false);

			// Create function
			std::string name = node->identifier->value + "_" + specialization->return_type.to_str();
			llvm::Function* f = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, name, this->module);

			// Set args for names
			size_t j = 0;
			for (auto &arg : f->args()) {
				arg.setName(node->args[j]->value);
				j++;
			}
		}

		// Generate functions
		for (size_t i = 0; i < node->specializations.size(); i++) {
			auto& specialization = node->specializations[i];
			assert(specialization->valid);

			std::string name = node->identifier->value + "_" + specialization->return_type.to_str();
			llvm::Function* f = this->module->getFunction(name);

			// Create the body of the function
			llvm::BasicBlock *body = llvm::BasicBlock::Create(*(this->context), "entry", f);
			this->builder->SetInsertPoint(body);

			// Add arguments to scope
			this->add_scope();
			size_t j = 0;
			for (auto &arg : f->args()) {
				this->current_scope()[node->args[j]->value] = &arg;
				j++;
			}

			// Codegen body
			if (std::dynamic_pointer_cast<Ast::Expression>(node->body)) {
				llvm::Value* result = this->codegen(std::dynamic_pointer_cast<Ast::Expression>(node->body));
				if (result) {
					this->builder->CreateRet(result);
				}
			}
			else if (std::dynamic_pointer_cast<Ast::Block>(node->body)) {
				this->codegen(std::dynamic_pointer_cast<Ast::Block>(node->body));
			}
			else {
				assert(false);
			}

			llvm::verifyFunction(*f);
			this->remove_scope();
		}
	}
}

void Codegen::codegen(std::shared_ptr<Ast::Assignment> node) {
	// Generate value of expression
	llvm::Value* expr = this->codegen(node->expression);

	// Add it to the scope
	this->current_scope()[node->identifier->value] = expr;
}

void Codegen::codegen(std::shared_ptr<Ast::Return> node) {
	// Generate value of expression
	llvm::Value* expr = this->codegen(node->expression);

	// Create return value
	assert(expr);
	this->builder->CreateRet(expr);
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Expression> node) {
	if      (std::dynamic_pointer_cast<Ast::Call>(node))       return this->codegen(std::dynamic_pointer_cast<Ast::Call>(node));
	else if (std::dynamic_pointer_cast<Ast::Number>(node))     return this->codegen(std::dynamic_pointer_cast<Ast::Number>(node));
	else if (std::dynamic_pointer_cast<Ast::Integer>(node))    return this->codegen(std::dynamic_pointer_cast<Ast::Integer>(node));
	else if (std::dynamic_pointer_cast<Ast::Identifier>(node)) return this->codegen(std::dynamic_pointer_cast<Ast::Identifier>(node));
	else if (std::dynamic_pointer_cast<Ast::Boolean>(node))    return this->codegen(std::dynamic_pointer_cast<Ast::Boolean>(node));

	assert(false);
	return nullptr;
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Call> node) {
	// Codegen args
	std::vector<llvm::Value*> args;
	for (size_t i = 0; i < node->args.size(); i++) {
		args.push_back(this->codegen(node->args[i]));
	}

	if (node->args.size() == 2) {
		if (node->identifier->value == "+") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFAdd(args[0], args[1], "addtmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateAdd(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "-") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFSub(args[0], args[1], "subtmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateSub(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "*") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFMul(args[0], args[1], "multmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateMul(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "/") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFDiv(args[0], args[1], "divtmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateSDiv(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "<") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFCmpULT(args[0], args[1], "cmptmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateICmpULT(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "<=") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFCmpULE(args[0], args[1], "cmptmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateICmpULE(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == ">") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFCmpUGT(args[0], args[1], "cmptmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateICmpUGT(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == ">=") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFCmpUGE(args[0], args[1], "cmptmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateICmpUGE(args[0], args[1], "addtmp");
			}
		}
		if (node->identifier->value == "==") {
			if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
				return this->builder->CreateFCmpUEQ(args[0], args[1], "eqtmp");
			}
			if (args[0]->getType()->isIntegerTy(1) && args[1]->getType()->isIntegerTy(1)) {
				return this->builder->CreateICmpEQ(args[0], args[1], "eqtmp");
			}
			if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
				return this->builder->CreateICmpEQ(args[0], args[1], "addtmp");
			}
		}
	}
	if (node->args.size() == 1) {
		if (node->identifier->value == "-") {
			if (args[0]->getType()->isDoubleTy()) {
				return this->builder->CreateFNeg(args[0], "negation");
			}
			if (args[0]->getType()->isIntegerTy(64)) {
				return this->builder->CreateNeg(args[0], "negation");
			}
		}
		if (node->identifier->value == "not") {
			return this->builder->CreateNot(args[0], "not");
		}
	}
	if (node->identifier->value == "print") {
		if (args[0]->getType()->isDoubleTy()) {
			std::vector<llvm::Value*> printArgs;
			printArgs.push_back(this->format_strings.format_float);
			printArgs.push_back(args[0]);
			this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
			return nullptr;
		}
		if (args[0]->getType()->isIntegerTy(64)) {
			std::vector<llvm::Value*> printArgs;
			printArgs.push_back(this->format_strings.format_integer);
			printArgs.push_back(args[0]);
			this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
			return nullptr;
		}
		if (args[0]->getType()->isIntegerTy(1)) {
			std::vector<llvm::Value*> printArgs;
			llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
			llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
			llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
			llvm::BasicBlock *merge = llvm::BasicBlock::Create(*(this->context), "ifcont");
			this->builder->CreateCondBr(args[0], then_block, else_block);

			// Create then branch
			this->builder->SetInsertPoint(then_block);
			printArgs.push_back(this->format_strings.format_true);
			this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
			this->builder->CreateBr(merge);
			then_block = this->builder->GetInsertBlock();

			printArgs.clear();

			// Create else branch
			current_function->getBasicBlockList().push_back(else_block);
			this->builder->SetInsertPoint(else_block);
			printArgs.push_back(this->format_strings.format_false);
			this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
			this->builder->CreateBr(merge);
			else_block = this->builder->GetInsertBlock();

			// Merge  block
			current_function->getBasicBlockList().push_back(merge);
			this->builder->SetInsertPoint(merge);
			return nullptr;
		}
	}

	// Get function
	std::string name = node->identifier->value + "_" + node->type.to_str();
	llvm::Function* function = this->module->getFunction(name);
	assert(function);

	// Make call
	return this->builder->CreateCall(function, args, "calltmp");
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Number> node) {
	return llvm::ConstantFP::get(*(this->context), llvm::APFloat(node->value));
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Integer> node) {
	return llvm::ConstantInt::get(*(this->context), llvm::APInt(64, node->value, true));
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Identifier> node) {
	return this->current_scope()[node->value];
}

llvm::Value* Codegen::codegen(std::shared_ptr<Ast::Boolean> node) {
	return llvm::ConstantInt::getBool(*(this->context), node->value);
}

llvm::Type* Codegen::as_llvm_type(Type type) {
	if      (type == Type("float64")) return llvm::Type::getDoubleTy(*(this->context));
	else if (type == Type("int64"))   return llvm::Type::getInt64Ty(*(this->context));
	else if (type == Type("bool"))    return llvm::Type::getInt1Ty(*(this->context));
	else if (type == Type("void"))    return llvm::Type::getVoidTy(*(this->context));
	else                              assert(false);
}

std::vector<llvm::Type*> Codegen::as_llvm_types(std::vector<Type> types) {
	std::vector<llvm::Type*> llvm_types;
	for (size_t i = 0; i < types.size(); i++) {
		llvm_types.push_back(this->as_llvm_type(types[i]));
	}
	return llvm_types;
}

void Codegen::add_scope() {
	this->scopes.push_back(std::unordered_map<std::string, llvm::Value*>());
}

std::unordered_map<std::string, llvm::Value*>& Codegen::current_scope() {
	return this->scopes[this->scopes.size() - 1];
}

void Codegen::remove_scope() {
	this->scopes.pop_back();
}
