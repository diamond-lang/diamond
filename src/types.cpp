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

bool Type::is_type_variable() const {
	std::string str = this->to_str();
	if (str.size() > 0 && str[0] == '$') return true;
	else                                 return false;
}

// For printing
std::vector<bool> append(std::vector<bool> vec, bool val) {
	vec.push_back(val);
	return vec;
}

void put_indent_level(size_t indent_level, std::vector<bool> last) {
	if (indent_level == 0) return;
	for (int i = 0; i < indent_level - 1; i++) {
		if (last[i]) {
			std::cout << "   ";
		} else {
			std::cout << "│  ";
		}
	}
	if (last[indent_level - 1]) {
		std::cout << "└──"; 
	} else {
		std::cout << "├──";
	}
}

// Program
void Ast::Program::print(size_t indent_level, std::vector<bool> last) {
	std::cout << "program" << '\n';
	for (size_t i = 0; i < this->statements.size(); i++) {
		this->statements[i]->print(indent_level + 1, append(last, (this->functions.size() == 0 && i == this->statements.size() - 1)));
	}
	for (size_t i = 0; i < this->functions.size(); i++) {
		this->functions[i]->print(indent_level + 1, append(last, i == this->functions.size() - 1));
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
void Ast::Block::print(size_t indent_level, std::vector<bool> last) {
	for (size_t i = 0; i < this->statements.size(); i++) {
		this->statements[i]->print(indent_level + 1, append(last, (this->functions.size() == 0 && i == this->statements.size() - 1)));
	}
	for (size_t i = 0; i < this->functions.size(); i++) {
		this->functions[i]->print(indent_level + 1, append(last, i == this->functions.size() - 1));
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
void Ast::Function::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << "function " << this->identifier->value << '(';
	for (size_t i = 0; i < this->args.size(); i++) {
		std::cout << this->args[i]->value;
		
		if (this->args[i]->type != Type("")) {
			std::cout << ": " << this->args[i]->type.to_str();
		}

		if (i != this->args.size() - 1) std::cout << ", ";
	}
	std::cout << ")";
	if (this->return_type != Type("")) {
		std::cout << ": " << this->return_type.to_str();
	}
	std::cout << "\n";
	if (std::dynamic_pointer_cast<Ast::Expression>(body)) {
		this->body->print(indent_level + 1, append(last, true));
	}
	else {
		this->body->print(indent_level, last);
	}
}

std::shared_ptr<Ast::Node> Ast::Function::clone() {
	std::vector<std::shared_ptr<Ast::Identifier>> args;
	for (size_t i = 0; i < this->args.size(); i++) {
		args.push_back(std::dynamic_pointer_cast<Identifier>(this->args[i]->clone()));
	}
	auto function = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), args, this->body->clone(), this->line, this->col, this->file);
	function->return_type = this->return_type;
	return function;
}

void Ast::FunctionSpecialization::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << "function " << this->identifier->value << '(';
	for (size_t i = 0; i < this->args.size(); i++) {
		std::cout << this->args[i]->value;
		
		if (this->args[i]->type != Type("")) {
			std::cout << ": " << this->args[i]->type.to_str();
		}

		if (i != this->args.size() - 1) std::cout << ", ";
	}
	std::cout << ")";
	if (this->return_type != Type("")) {
		std::cout << ": " << this->return_type.to_str();
	}
	std::cout << "\n";
	if (std::dynamic_pointer_cast<Ast::Expression>(body)) {
		this->body->print(indent_level + 1, append(last, true));
	}
	else {
		this->body->print(indent_level, last);
	}
}

std::shared_ptr<Ast::Node> Ast::FunctionSpecialization::clone() {
	assert(false);
}

// Assignment
void Ast::Assignment::print(size_t indent_level, std::vector<bool> last) {
	this->identifier->print(indent_level, last);
	put_indent_level(indent_level + 1, append(last, false));
	std::cout << "be" << '\n';
	
	this->expression->print(indent_level + 1, append(last, true));
}

std::shared_ptr<Ast::Node> Ast::Assignment::clone() {
	return std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), std::dynamic_pointer_cast<Expression>(this->expression->clone()), this->line, this->col, this->file);
}

// Return
void Ast::Return::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << "return" << '\n';
	if (this->expression) {
		this->expression->print(indent_level + 1, append(last, true));
	}
}

std::shared_ptr<Ast::Node> Ast::Return::clone() {
	if (this->expression) return std::make_shared<Ast::Return>(std::dynamic_pointer_cast<Expression>(this->expression->clone()), this->line, this->col, this->file);
	else                  return std::make_shared<Ast::Return>(nullptr, this->line, this->col, this->file);
}

// IfElseStmt
void Ast::IfElseStmt::print(size_t indent_level, std::vector<bool> last) {
	bool is_last = last[last.size() - 1];
	last.pop_back();

	bool has_else_block = this->else_block ? true : false;
	put_indent_level(indent_level, append(last, is_last && !has_else_block));
	std::cout << "if" << '\n';
	this->condition->print(indent_level + 1, append(append(last, is_last && !has_else_block), false));
	this->block->print(indent_level, append(last, is_last && !has_else_block));

	if (this->else_block) {
		put_indent_level(indent_level, append(last, is_last));
		std::cout << "else" << "\n";
		this->else_block->print(indent_level, append(last, is_last));
	}
}

std::shared_ptr<Ast::Node> Ast::IfElseStmt::clone() {
	if (this->else_block) return std::make_shared<Ast::IfElseStmt>(std::dynamic_pointer_cast<Ast::Expression>(this->condition->clone()), std::dynamic_pointer_cast<Ast::Block>(this->block->clone()), std::dynamic_pointer_cast<Ast::Block>(this->else_block->clone()), this->line, this->col, this->file);
	else                  return std::make_shared<Ast::IfElseStmt>(std::dynamic_pointer_cast<Ast::Expression>(this->condition->clone()), std::dynamic_pointer_cast<Ast::Block>(this->block->clone()), this->line, this->col, this->file);
}

// IfElseExpr
void Ast::IfElseExpr::print(size_t indent_level, std::vector<bool> last) {
	bool is_last = true;
	if (last.size() > 0) {
		is_last = last[last.size() - 1];
		last.pop_back();
	}

	put_indent_level(indent_level, append(last, false));
	std::cout << "if";
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
	this->condition->print(indent_level + 1, append(append(last, false), false));
	this->expression->print(indent_level + 1, append(append(last, false), true));

	assert(this->else_expression);
	put_indent_level(indent_level, append(last, is_last));
	std::cout << "else" << "\n";
	this->else_expression->print(indent_level + 1, append(append(last, is_last), true));
}

std::shared_ptr<Ast::Node> Ast::IfElseExpr::clone() {
	assert(this->else_expression);
	auto ifElse = std::make_shared<Ast::IfElseExpr>(std::dynamic_pointer_cast<Ast::Expression>(this->condition->clone()), std::dynamic_pointer_cast<Ast::Expression>(this->expression->clone()), std::dynamic_pointer_cast<Ast::Expression>(this->else_expression->clone()), this->line, this->col, this->file);
	ifElse->type = this->type;
	return ifElse;
}

// Call
void Ast::Call::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << this->identifier->value;
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
	for (size_t i = 0; i < this->args.size(); i++) {
		this->args[i]->print(indent_level + 1, append(last, i == this->args.size() - 1));
	}
}

std::shared_ptr<Ast::Node> Ast::Call::clone() {
	std::vector<std::shared_ptr<Ast::Expression>> args;
	for (size_t i = 0; i < this->args.size(); i++) {
		args.push_back(std::dynamic_pointer_cast<Expression>(this->args[i]->clone()));
	}
	auto call = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(this->identifier->clone()), args, this->line, this->col, this->file);
	call->type = this->type;
	return call;
}

// Number
void Ast::Number::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << this->value;
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
}

std::shared_ptr<Ast::Node> Ast::Number::clone() {
	auto number = std::make_shared<Ast::Number>(this->value, this->line, this->col, this->file);
	number->type = this->type;
	return number;
}

// Integer
void Ast::Integer::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << this->value;
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
}

std::shared_ptr<Ast::Node> Ast::Integer::clone() {
	auto integer = std::make_shared<Ast::Integer>(this->value, this->line, this->col, this->file);
	integer->type = this->type;
	return integer;
}

// Identifier
void Ast::Identifier::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << this->value;
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
}

std::shared_ptr<Ast::Node> Ast::Identifier::clone() {
	auto identifier = std::make_shared<Ast::Identifier>(this->value, this->line, this->col, this->file);
	identifier->type = this->type;
	return identifier;
}

// Boolean
void Ast::Boolean::print(size_t indent_level, std::vector<bool> last) {
	put_indent_level(indent_level, last);
	std::cout << (this->value ? "true" : "false");
	if (this->type != Type("")) std::cout << ": " << this->type.to_str();
	std::cout << "\n";
}

std::shared_ptr<Ast::Node> Ast::Boolean::clone() {
	auto boolean = std::make_shared<Ast::Boolean>(this->value, this->line, this->col, this->file);
	boolean->type = this->type;
	return boolean;
}

// Utilities
std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Identifier>> args) {
	std::vector<Type> types;
	for (size_t i = 0; i < args.size(); i++) {
		types.push_back(args[i]->type);
	}
	return types;
}

std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Expression>> args) {
	std::vector<Type> types;
	for (size_t i = 0; i < args.size(); i++) {
		types.push_back(args[i]->type);
	}
	return types;
}