#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cassert>
#include <cstdint>

// Source file
struct Source {
	size_t line;
	size_t col;
	size_t indentation_level = -1;
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

struct Ok {
	Ok() {}
	~Ok() {}
};

struct Error {
	std::string message;

	Error() {}
	Error(std::string message) : message(message) {}
	~Error() {}
};

using Errors = std::vector<Error>;

// Parser result
template <class T>
struct ParserResult {
	bool ok;
	T value;
	Source source;
	Errors errors;

	ParserResult<T>(T value, Source source) : ok(true), value(value), source(source) {}
	ParserResult<T>(Errors errors) : ok(false), errors(errors) {}

	bool is_ok() {return this->ok;}
	bool is_error()     {return !this->is_ok();}
	T get_value()       {
		assert(this->ok);
		return this->value;
	}
	Source get_source() {
		assert(this->ok);
		return this->source;
	}
	Errors get_errors() {
		assert(!(this->ok));
		return this->errors;
	}
};

template <class T1, class T2>
struct Result {
	std::variant<T1, T2> value;

	Result() {}
	Result(T1 value) : value(value) {}
	Result(T2 error) : value(error) {}
	~Result() {}

	bool is_ok()    {return std::holds_alternative<T1>(this->value);}
	bool is_error() {return !std::holds_alternative<T1>(this->value);}
	T1 get_value()  {
		assert(this->is_ok() == true);
		return std::get<T1>(this->value);
	}
	T2 get_error() {
		assert(this->is_error() == true);
		return std::get<T2>(this->value);
	}
	T2 get_errors()  {
		assert(this->is_error() == true);
		return std::get<T2>(this->value);
	}
};

// Ast
struct Type {
	std::string name;
	std::vector<Type> parameters;

	Type() : name("") {}
	Type(std::string name) : name(name) {}
	Type(std::string name, std::vector<Type> parameters) : name(name), parameters(parameters) {}
	bool operator==(const Type &t) const;
	bool operator!=(const Type &t) const;
	std::string to_str(std::string output = "") const;
	bool is_type_variable() const;
};

namespace Ast {
	struct Node {
		size_t line;
		size_t col;
		std::string file;

		Node(size_t line, size_t col, std::string file): line(line), col(col), file(file) {}
		virtual ~Node() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false) = 0;
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct Identifier;
	struct Function;

	struct Program : Node {
		std::vector<std::shared_ptr<Ast::Node>> statements;
		std::vector<std::shared_ptr<Ast::Function>> functions;

		Program(std::vector<std::shared_ptr<Ast::Node>> statements, std::vector<std::shared_ptr<Ast::Function>> functions, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements), functions(functions) {}
		virtual ~Program() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
		void print_with_concrete_types();
	};

	struct Block : Node {
		std::vector<std::shared_ptr<Ast::Node>> statements;
		std::vector<std::shared_ptr<Ast::Function>> functions;
		Type type = Type("");

		Block(std::vector<std::shared_ptr<Ast::Node>> statements, std::vector<std::shared_ptr<Ast::Function>> functions, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements), functions(functions) {}
		virtual ~Block() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization;

	struct Function : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Identifier>> args;
		std::shared_ptr<Ast::Node> body;

		bool generic = false;
		Type return_type = Type("");
		std::vector<std::shared_ptr<Ast::FunctionSpecialization>> specializations;

		Function(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Identifier>> args, std::shared_ptr<Ast::Node> body, size_t line, size_t col, std::string file) :  Node(line, col, file), identifier(identifier), args(args), body(body) {}
		virtual ~Function() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Identifier>> args;
		std::shared_ptr<Ast::Node> body;
		
		bool valid = false;
		Type return_type = Type("");

		FunctionSpecialization(size_t line, size_t col, std::string file) :  Node(line, col, file) {}
		virtual ~FunctionSpecialization() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Expression;

	struct Assignment : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::shared_ptr<Ast::Expression> expression;
		bool is_mutable = false;
		bool nonlocal = false;

		Assignment(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Expression> expression, bool is_mutable, bool nonlocal, size_t line, size_t col, std::string file) : Node(line, col, file), identifier(identifier), expression(expression), is_mutable(is_mutable), nonlocal(nonlocal) {}
		virtual ~Assignment() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Return : Node {
		std::shared_ptr<Ast::Expression> expression;

		Return(std::shared_ptr<Ast::Expression> expression, size_t line, size_t col, std::string file) : Node(line, col, file), expression(expression) {}
		virtual ~Return() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct IfElseStmt : Node {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Block> block;
		std::shared_ptr<Ast::Block> else_block;

		IfElseStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block), else_block(nullptr) {}
		IfElseStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, std::shared_ptr<Ast::Block> else_block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block), else_block(else_block) {}
		virtual ~IfElseStmt() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct WhileStmt : Node {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Block> block;

		WhileStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block) {}
		virtual ~WhileStmt() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Expression : Node {
		Type type = Type();

		Expression(size_t line, size_t col, std::string file) : Node(line, col, file) {}
		virtual ~Expression() {}
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct IfElseExpr : Expression {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Expression> expression;
		std::shared_ptr<Ast::Expression> else_expression;
		
		IfElseExpr(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Expression> expression, std::shared_ptr<Ast::Expression> else_expression, size_t line, size_t col, std::string file) : Expression(line, col, file), condition(condition), expression(expression), else_expression(else_expression) {}
		virtual ~IfElseExpr() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Call : Expression {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Expression>> args;

		Call(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Expression>> args, size_t line, size_t col, std::string file) : Expression(line, col, file), identifier(identifier), args(args) {}
		virtual ~Call() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Number : Expression {
		double value;

		Number(double value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Number() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Integer : Expression {
		int64_t value;

		Integer(int64_t value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Integer() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Identifier : Expression {
		std::string value;

		Identifier(std::string value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Identifier() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Boolean : Expression {
		bool value;

		Boolean(bool value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Boolean() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};
}

std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Identifier>> args);
std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Expression>> args);

#endif