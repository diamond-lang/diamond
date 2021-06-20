#include <iostream>
#include <unordered_map>
#include <assert.h>

#include "errors.hpp"
#include "semantic.hpp"

struct Binding {
	std::string identifier;
	Ast::Assignment* assignment;
	Type type;
};

struct Context {
	std::string file;
	std::unordered_map<std::string, Binding> scope;

	std::string analyze(Ast::Assignment* assignment);
	std::string analyze_expression(Ast::Node* node);
	std::string analyze(Ast::Call* node);
	std::string analyze(Ast::Number* node);
	std::string analyze(Ast::Identifier* node);
	std::string analyze(Ast::Boolean* node);

	Binding* get_binding(std::string identifier);
};


// Implementations
// ---------------
void analyze(Ast::Program* program) {
	Context context;

	for (size_t i = 0; i  < program->statements.size(); i++) {
		Ast::Node* node = program->statements[i];
		std::string error;

		if      (dynamic_cast<Ast::Assignment*>(node)) error = context.analyze(dynamic_cast<Ast::Assignment*>(node));
		else if (node->is_expression()) error = context.analyze_expression(node);

		if (error.size() != 0) std::cout << error << '\n';
	}
}

std::string Context::analyze(Ast::Assignment* assignment) {
	Binding binding;
	binding.identifier = assignment->identifier->value;
	binding.assignment = assignment;

	// Get expression type
	std::string error = this->analyze_expression(assignment->expression);
	if (error.size() != 0) return error;
	assignment->type = assignment->expression->type;
	binding.type = assignment->type;

	// Save it context
	if (this->get_binding(assignment->identifier->value)) {
		return errors::reassigning_immutable_variable(assignment->identifier, this->get_binding(assignment->identifier->value)->assignment);
	}
	else {
		this->scope[assignment->identifier->value] = binding;
		return "";
	}
}

std::string Context::analyze_expression(Ast::Node* node) {
	if      (dynamic_cast<Ast::Call*>(node))       return this->analyze(dynamic_cast<Ast::Call*>(node));
	else if (dynamic_cast<Ast::Number*>(node))     return this->analyze(dynamic_cast<Ast::Number*>(node));
	else if (dynamic_cast<Ast::Identifier*>(node)) return this->analyze(dynamic_cast<Ast::Identifier*>(node));
	else if (dynamic_cast<Ast::Boolean*>(node))    return this->analyze(dynamic_cast<Ast::Boolean*>(node));
	else assert(false);
	return "Error: This shouldn't happen";
}

std::string Context::analyze(Ast::Call* node) {
	// Get types of arguments
	for (size_t i = 0; i < node->args.size(); i++) {
		std::string error = this->analyze_expression(node->args[i]);
		if (error.size() != 0) return error;
	}

	if (node->identifier->value == "+" || node->identifier->value == "-" || node->identifier->value == "*" || node->identifier->value == "/") {
		if (node->args[0]->type == Type("float64") && node->args[1]->type == Type("float64")) {
			node->type = Type("float64");
		}
		else {
			return errors::operation_not_defined_for(node, node->args[0]->type.to_str(), node->args[1]->type.to_str());
		}
	}
	else if (node->identifier->value == "<" || node->identifier->value == "<=" || node->identifier->value == ">" || node->identifier->value == ">=") {
		if (node->args[0]->type == Type("float64") && node->args[1]->type == Type("float64")) {
			node->type = Type("bool");
		}
		else {
			return errors::operation_not_defined_for(node, node->args[0]->type.to_str(), node->args[1]->type.to_str());
		}
	}
	else if (node->identifier->value == "==") {
		if (node->args[0]->type == node->args[1]->type) {
			node->type = Type("bool");
		}
		else {
			return errors::operation_not_defined_for(node, node->args[0]->type.to_str(), node->args[1]->type.to_str());
		}
	}
	else {
		assert(false);
	}

	return "";
}

std::string Context::analyze(Ast::Number* node) {
	node->type = Type("float64");
	return "";
}

std::string Context::analyze(Ast::Identifier* node) {
	Binding* binding = this->get_binding(node->value);
	if (!binding) {
		return errors::undefined_variable(node);
	}
	else {
		node->type = binding->type;
		return "";
	}
}

std::string Context::analyze(Ast::Boolean* node) {
	node->type = Type("bool");
	return "";
}

Binding* Context::get_binding(std::string identifier) {
	if (this->scope.find(identifier) != this->scope.end()) {
		return &(this->scope[identifier]);
	}
	else {
		return nullptr;
	}
}
