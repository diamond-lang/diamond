#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>
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

struct Codegen {
	llvm::LLVMContext* context;
	llvm::Module* module;
	llvm::IRBuilder<>* builder;
	llvm::Value* format_str;
};

// Source file
struct Source {
	size_t line;
	size_t col;
	std::string file;
	std::string::iterator it;
	std::string::iterator end;

	Source() {}
	Source(std::string file, std::string::iterator it, std::string::iterator end) : line(1), col(1), file(file), it(it), end(end) {}
};
char current(Source source);
bool at_end(Source source);
bool match(Source source, std::string to_match);
Source operator+(Source source, size_t offset);

// Parser result
template <class T>
struct ParserResult {
	T value;
	Source source;
	std::string error_message;

	bool error() {
		if (this->error_message == "") return false;
		else                           return true;
	}

	ParserResult<T>(T value, Source source, std::string error_message = "") : value(value), source(source), error_message(error_message) {}
	ParserResult<T>(Source source, std::string error_message) : source(source), error_message(error_message) {}
};

// Ast
namespace Ast {
	struct Node {
		size_t line;
		size_t col;
		std::string file;

		Node(size_t line, size_t col, std::string file): line(line), col(col), file(file) {}
		virtual ~Node() {}
		virtual void print(size_t indent_level = 0) = 0;
		virtual llvm::Value* codegen(Codegen codegen) = 0;

	};

	struct Program : Node {
		std::vector<Ast::Node*> expressions;

		Program(std::vector<Ast::Node*> expressions, size_t line, size_t col, std::string file) : Node(line, col, file), expressions(expressions) {}
		virtual ~Program();
		virtual void print(size_t indent_level = 0);
		llvm::Value* codegen(Codegen codegen) override;
	};

	struct Float : Node {
		double value;

		Float(double value, size_t line, size_t col, std::string file) : Node(line, col, file), value(value) {}
		virtual ~Float();
		virtual void print(size_t indent_level = 0);
		llvm::Value* codegen(Codegen codegen) override;
	};
}

#endif
