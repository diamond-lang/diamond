#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>

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
	};

	struct Program : Node {
		std::vector<Ast::Node*> statements;

		Program(std::vector<Ast::Node*> statements, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements) {}
		virtual ~Program();
		virtual void print(size_t indent_level = 0);
	};

	struct Call : Node {
		std::string identifier;
		std::vector<Ast::Node*> args;

		Call(std::string identifier, std::vector<Ast::Node*> args, size_t line, size_t col, std::string file) : Node(line, col, file), identifier(identifier), args(args) {}
		virtual ~Call();
		virtual void print(size_t indent_level = 0);
	};

	struct Assignment : Node {
		std::string identifier;
		Ast::Node* expression;

		Assignment(std::string identifier, Ast::Node* expression, size_t line, size_t col, std::string file) : Node(line, col, file), identifier(identifier), expression(expression) {}
		virtual ~Assignment();
		virtual void print(size_t indent_level = 0);
	};

	struct Number : Node {
		double value;

		Number(double value, size_t line, size_t col, std::string file) : Node(line, col, file), value(value) {}
		virtual ~Number();
		virtual void print(size_t indent_level = 0);
	};

	struct Identifier : Node {
		std::string value;

		Identifier(std::string value, size_t line, size_t col, std::string file) : Node(line, col, file), value(value) {}
		virtual ~Identifier();
		virtual void print(size_t indent_level = 0);
	};
}

#endif
