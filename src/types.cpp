#include <iostream>

#include "types.hpp"

// Source
char current(Source source) {
	return *(source.it);
}

bool at_end(Source source) {
	if (source.it >= source.end) return true;
	else                         return false;
}

bool match(Source source, std::string to_match) {
	for (size_t i = 0; i < to_match.size(); i++) {
		if (current(source) != to_match[i]) return false;
		source = source + 1;
	}
	return true;
}

Source addOne(Source source) {
	source.it++;
	if (current(source) == '\n') {
		source.line += 1;
		source.col = 1;
	}
	else {
		source.col++;
	}
	return source;
}

Source operator+(Source source, size_t offset) {
	if (offset == 0) return source;
	else             return addOne(source) + (offset - 1);
}

// Ast
// ---
void put_indent_level(size_t indent_level) {
	for (size_t i = 0; i < indent_level; i++) std::cout << "    ";
}

// Program
void Ast::Program::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << "program" << '\n';
	for (size_t i = 0; i < this->expressions.size(); i++) {
		this->expressions[i]->print(indent_level + 1);
	}
}

Ast::Program::~Program() {
	for (size_t i = 0; i < this->expressions.size(); i++) {
		delete this->expressions[i];
	}
}

// Float
void Ast::Float::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->value << '\n';
}

Ast::Float::~Float() {}
