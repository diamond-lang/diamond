#include <iostream>
#include <assert.h>

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
	if (current(source) == '\n') {
		source.line += 1;
		source.col = 1;
	}
	else {
		source.col++;
	}
	source.it++;
	return source;
}

Source operator+(Source source, size_t offset) {
	if (offset == 0) return source;
	else             return addOne(source) + (offset - 1);
}

// Ast
// ---

// Type
bool Type::operator==(const Type &t) const {
	if (this->name == t.name && this->parameters.size() == t.parameters.size()) {
		for (size_t i = 0; i < this->parameters.size(); i++) {
			if (this->parameters[i] != t.parameters[i]) {
				return false;
			}
		}
		return true;
	}
	else {
		return false;
	}
}

bool Type::operator!=(const Type &t) const {
	return !(t == *this);
}

std::string Type::to_str(std::string output) const {
	if (this->parameters.size() == 0) {
		output += this->name;
	}
	else {
		output += "(";
		output += this->name;
		output += " of ";
		output += this->parameters[0].to_str(output);
		for (size_t i = 1; i < this->parameters.size(); i++) {
			output += ", ";
			output += this->parameters[0].to_str(output);
		}
		output += ")";
	}
	return output;
}

// For printing
void put_indent_level(size_t indent_level) {
	for (size_t i = 0; i < indent_level; i++) std::cout << "    ";
}

// Program
void Ast::Program::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << "program" << '\n';
	for (size_t i = 0; i < this->statements.size(); i++) {
		this->statements[i]->print(indent_level + 1);
	}
	for (size_t i = 0; i < this->functions.size(); i++) {
		this->functions[i]->print(indent_level + 1);
	}
}

std::shared_ptr<Ast::Node> Ast::Program::clone() {
	std::vector<std::shared_ptr<Ast::Node>> statements;
	for (size_t i = 0; i < this->statements.size(); i++) {
		statements.push_back(this->statements[i]->clone());
	}
	std::vector<std::shared_ptr<Ast::Function>> functions;
	for (size_t i = 0; i < this->functions.size(); i++) {
		functions.push_back(std::dynamic_pointer_cast<Ast::Function>(this->functions[i]->clone()));
	}
	return std::make_shared<Ast::Program>(statements, functions, this->line, this->col, this->file);
}

// Block
void Ast::Block::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << "block" << '\n';
	for (size_t i = 0; i < this->statements.size(); i++) {
		this->statements[i]->print(indent_level + 1);
	}
	for (size_t i = 0; i < this->functions.size(); i++) {
		this->functions[i]->print(indent_level + 1);
	}
}

std::shared_ptr<Ast::Node> Ast::Block::clone() {
	std::vector<std::shared_ptr<Ast::Node>> statements;
	for (size_t i = 0; i < this->statements.size(); i++) {
		statements.push_back(this->statements[i]->clone());
	}
	std::vector<std::shared_ptr<Ast::Function>> functions;
	for (size_t i = 0; i < this->functions.size(); i++) {
		functions.push_back(std::dynamic_pointer_cast<Ast::Function>(this->functions[i]->clone()));
	}
	return std::make_shared<Ast::Block>(statements, functions, this->line, this->col, this->file);
}

// Function
void Ast::Function::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->identifier->value << '(';
	for (size_t i = 0; i < this->args.size(); i++) {
		std::cout << this->args[i]->value;
		if (i != this->args.size() - 1) std::cout << ", ";
	}
	std::cout << ") is\n";
	this->body->print(indent_level + 1);
}

std::shared_ptr<Ast::Node> Ast::Function::clone() {
	std::vector<std::shared_ptr<Ast::Identifier>> args;
	for (size_t i = 0; i < this->args.size(); i++) {
		args.push_back(std::dynamic_pointer_cast<Identifier>(this->args[i]->clone()));
	}
	return std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), args, this->body->clone(), this->line, this->col, this->file);
}

// Assignment
void Ast::Assignment::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->identifier->value << '\n';
	put_indent_level(indent_level + 1);
	std::cout << "be" << '\n';
	this->expression->print(indent_level + 1);
}

std::shared_ptr<Ast::Node> Ast::Assignment::clone() {
	return std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), std::dynamic_pointer_cast<Expression>(this->expression->clone()), this->line, this->col, this->file);
}

// Call
void Ast::Call::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->identifier->value << '\n';
	for (size_t i = 0; i < this->args.size(); i++) {
		this->args[i]->print(indent_level + 1);
	}
}

std::shared_ptr<Ast::Node> Ast::Call::clone() {
	std::vector<std::shared_ptr<Ast::Expression>> args;
	for (size_t i = 0; i < this->args.size(); i++) {
		args.push_back(std::dynamic_pointer_cast<Expression>(this->args[i]->clone()));
	}
	return std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), args, this->line, this->col, this->file);
}

// Number
void Ast::Number::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->value << '\n';
}

std::shared_ptr<Ast::Node> Ast::Number::clone() {
	return std::make_shared<Ast::Number>(this->value, this->line, this->col, this->file);
}

// Integer
void Ast::Integer::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->value << '\n';
}

std::shared_ptr<Ast::Node> Ast::Integer::clone() {
	return std::make_shared<Ast::Integer>(this->value, this->line, this->col, this->file);
}

// Identifier
void Ast::Identifier::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << this->value << '\n';
}

std::shared_ptr<Ast::Node> Ast::Identifier::clone() {
	return std::make_shared<Ast::Identifier>(this->value, this->line, this->col, this->file);
}

// Boolean
void Ast::Boolean::print(size_t indent_level) {
	put_indent_level(indent_level);
	std::cout << (this->value ? "true" : "false") << '\n';
}

std::shared_ptr<Ast::Node> Ast::Boolean::clone() {
	return std::make_shared<Ast::Boolean>(this->value, this->line, this->col, this->file);
}
