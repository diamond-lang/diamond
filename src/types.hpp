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
	std::unique_ptr<T> value;
	Source source;
	std::string error_message;

	bool error() {
		if (this->error_message == "" || this->value != nullptr) return false;
		else                                                     return true;
	}

	ParserResult<T>(std::unique_ptr<T> value, Source source, std::string error_message = "") : value(std::move(value)), source(source) {}
	ParserResult<T>(Source source, std::string error_message) : value(nullptr), source(source), error_message(error_message) {}
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
		std::vector<std::unique_ptr<Ast::Node>> expressions;
		Program(std::vector<std::unique_ptr<Ast::Node>> expressions, size_t line, size_t col, std::string file) : Node(line, col, file), expressions(std::move(expressions)) {}
		virtual void print(size_t indent_level = 0);
	};

	struct Float : Node {
		double value;
		Float(double value, size_t line, size_t col, std::string file) : Node(line, col, file), value(value) {}
		virtual void print(size_t indent_level = 0);
	};
}

#endif
