#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>
#include <vector>
#include <memory>

struct SourceFile {
	size_t line;
	size_t col;
	std::string file;
	std::string::iterator it;
	std::string::iterator end;

	SourceFile(std::string file, std::string::iterator it, std::string::iterator end) : line(1), col(1), file(file), it(it), end(end) {}
};
char current(SourceFile source);
bool at_end(SourceFile source);
bool match(SourceFile source, std::string to_match);
SourceFile operator+(SourceFile source, size_t offset);

template <class T>
struct ParserResult {
	std::unique_ptr<T> value;
	SourceFile source;
	std::string error_message;

	bool error() {
		if (this->error_message == "" || this->value != nullptr) return false;
		else                                                     return true;
	}

	ParserResult<T>(std::unique_ptr<T> value, SourceFile source, std::string error_message = "") : value(std::move(value)), source(source) {}
	ParserResult<T>(SourceFile source, std::string error_message) : value(nullptr), source(source), error_message(error_message) {}
};

namespace Ast {
	struct Node {
		size_t line;
		size_t col;
		std::string file;
		Node(size_t line, size_t col, std::string file): line(line), col(col), file(file) {}
		virtual ~Node() {}
	};

	struct Program : Node {
		std::unique_ptr<Ast::Node> expressions;
		Program(std::unique_ptr<Ast::Node> expressions) : Node(line, col, file), expressions(std::move(expressions)) {}
	};

	struct Float : Node {
		double value;
		Float(double value, size_t line, size_t col, std::string file) : Node(line, col, file), value(value) {}
	};

	struct Call : Node {
		std::string name;
		std::vector<std::unique_ptr<Node>> args;
		Call(std::string name, std::vector<std::unique_ptr<Node>> args) : Node(line, col, file), name(name), args(std::move(args)) {}
	};
}

#endif
