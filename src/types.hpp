#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cassert>

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

struct Ok {
	Ok() {}
	~Ok() {}
};

struct Error {
	std::string error_message;

	Error() {}
	Error(std::string error_message) : error_message(error_message) {}
	~Error() {}
};

// Parser result
template <class T>
struct ParserResult {
	T value;
	Source source;
	std::string error_message;

	ParserResult<T>(T value, Source source, std::string error_message = "") : value(value), source(source), error_message(error_message) {}
	ParserResult<T>(Source source, std::string error_message) : source(source), error_message(error_message) {}

	bool is_ok()        {return this->error_message.size() == 0;}
	bool is_error()     {return !this->is_ok();}
	T get_value()       {return this->value;}
	Source get_source() {return this->source;}
	Error get_error()   {return Error(this->error_message);}
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
	T2 get_error()  {
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
};

namespace Ast {
	struct Node {
		size_t line;
		size_t col;
		std::string file;

		Node(size_t line, size_t col, std::string file): line(line), col(col), file(file) {}
		virtual ~Node() {}
		virtual void print(size_t indent_level = 0) = 0;
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct Identifier;
	struct Function;

	struct Program : Node {
		std::vector<std::shared_ptr<Ast::Node>> statements;
		std::vector<std::shared_ptr<Ast::Function>> functions;

		Program(std::vector<std::shared_ptr<Ast::Node>> statements, std::vector<std::shared_ptr<Ast::Function>> functions, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements), functions(functions) {}
		virtual ~Program() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization;

	struct Function : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Identifier>> args;
		std::shared_ptr<Ast::Node> body;

		bool generic = false;
		std::vector<Type> args_types;
		Type return_type;
		std::vector<std::shared_ptr<Ast::FunctionSpecialization>> specializations;

		Function(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Identifier>> args, std::shared_ptr<Ast::Node> body, size_t line, size_t col, std::string file) :  Node(line, col, file), identifier(identifier), args(args), body(body) {}
		virtual ~Function() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization {
		bool valid = false;
		std::vector<Type> args_types;
		Type return_type;
		std::shared_ptr<Ast::Node> body;
	};

	struct Expression;

	struct Assignment : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::shared_ptr<Ast::Expression> expression;

		Assignment(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Expression> expression, size_t line, size_t col, std::string file) : Node(line, col, file), identifier(identifier), expression(expression) {}
		virtual ~Assignment() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct Expression : Node {
		Type type;

		Expression(size_t line, size_t col, std::string file) : Node(line, col, file) {}
		virtual ~Expression() {}
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct Call : Expression {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Expression>> args;

		Call(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Expression>> args, size_t line, size_t col, std::string file) : Expression(line, col, file), identifier(identifier), args(args) {}
		virtual ~Call() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct Number : Expression {
		double value;

		Number(double value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Number() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct Identifier : Expression {
		std::string value;

		Identifier(std::string value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Identifier() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};

	struct Boolean : Expression {
		bool value;

		Boolean(bool value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Boolean() {}
		virtual void print(size_t indent_level = 0);
		virtual std::shared_ptr<Node> clone();
	};
}

#endif
