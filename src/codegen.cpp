#include "codegen.hpp"

Codegen codegen(Ast::Program &program) {
	Codegen codegen;
	codegen.context = new llvm::LLVMContext();
	codegen.module = new llvm::Module("My cool jit", *(codegen.context));
	codegen.builder = new llvm::IRBuilder(*(codegen.context));

	/*Declare that printf exists and has signature int (i8*, ...)**/
	/* from: https://laratelli.com/posts/2020/06/generating-calls-to-printf-from-llvm-ir/ */
	std::vector<llvm::Type *> args;
	args.push_back(llvm::Type::getInt8PtrTy(*(codegen.context)));
	llvm::FunctionType *printfType = llvm::FunctionType::get(codegen.builder->getInt32Ty(), args, true); // `true` specifies the function as variadic
	llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", codegen.module);

	// Crate main function
	llvm::FunctionType* mainType = llvm::FunctionType::get(codegen.builder->getInt32Ty(), false);
	llvm::Function* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", codegen.module);
	llvm::BasicBlock* entry = llvm::BasicBlock::Create(*(codegen.context), "entry", main);
	codegen.builder->SetInsertPoint(entry);

	/* Create global variables */
	codegen.format_str = codegen.builder->CreateGlobalStringPtr("%g\n");

	program.codegen(codegen);

	/*return value for `main`*/
	codegen.builder->CreateRet(llvm::ConstantInt::get(*(codegen.context), llvm::APInt(32, 0)));

	return codegen;
}
