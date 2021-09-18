#include <iostream>
#include <unordered_map>
#include <assert.h>

#include "errors.hpp"
#include "semantic.hpp"

struct Binding {
	std::string identifier;
	std::shared_ptr<Ast::Assignment> assignment;
	Type type;
};

struct Context {
	std::string file;
	std::unordered_map<std::string, Binding> scope;

	Result<Ok, Error> analyze(std::shared_ptr<Ast::Assignment> assignment);
	Result<Ok, Error> analyze_expression(std::shared_ptr<Ast::Node> node);
	Result<Ok, Error> analyze(std::shared_ptr<Ast::Call> node);
	Result<Ok, Error> analyze(std::shared_ptr<Ast::Number> node);
	Result<Ok, Error> analyze(std::shared_ptr<Ast::Identifier> node);
	Result<Ok, Error> analyze(std::shared_ptr<Ast::Boolean> node);

	Binding* get_binding(std::string identifier);
	Result<Ok, Error> get_type_of_intrinsic(std::shared_ptr<Ast::Call> node);
	std::vector<Type> get_args_types(std::shared_ptr<Ast::Call> node);
};


// Implementations
// ---------------
Result<Ok, std::vector<Error>> semantic::analyze(std::shared_ptr<Ast::Program> program) {
	Context context;
	std::vector<Error> errors;

	for (size_t i = 0; i  < program->statements.size(); i++) {
		std::shared_ptr<Ast::Node> node = program->statements[i];
		Result<Ok, Error> result;

		if      (std::dynamic_pointer_cast<Ast::Assignment>(node)) result = context.analyze(std::dynamic_pointer_cast<Ast::Assignment>(node));
		else if (node->is_expression())                            result = context.analyze_expression(node);

		if (result.is_error()) {
			errors.push_back(result.get_error());
		}
	}

	if (errors.size() != 0) return Result<Ok, std::vector<Error>>(errors);
	else                    return Result<Ok, std::vector<Error>>(Ok());
}

Result<Ok, Error> Context::analyze(std::shared_ptr<Ast::Assignment> assignment) {
	Binding binding;
	binding.identifier = assignment->identifier->value;
	binding.assignment = assignment;

	// Get expression type
	auto result = this->analyze_expression(assignment->expression);
	if (result.is_error()) return result;
	assignment->type = assignment->expression->type;
	binding.type = assignment->type;

	// Save it context
	if (this->get_binding(assignment->identifier->value)) {
		return Result<Ok, Error>(Error(errors::reassigning_immutable_variable(assignment->identifier, this->get_binding(assignment->identifier->value)->assignment)));
	}
	else {
		this->scope[assignment->identifier->value] = binding;
		return Result<Ok, Error>(Ok());
	}
}

Result<Ok, Error> Context::analyze_expression(std::shared_ptr<Ast::Node> node) {
	if      (std::dynamic_pointer_cast<Ast::Call>(node))       return this->analyze(std::dynamic_pointer_cast<Ast::Call>(node));
	else if (std::dynamic_pointer_cast<Ast::Number>(node))     return this->analyze(std::dynamic_pointer_cast<Ast::Number>(node));
	else if (std::dynamic_pointer_cast<Ast::Identifier>(node)) return this->analyze(std::dynamic_pointer_cast<Ast::Identifier>(node));
	else if (std::dynamic_pointer_cast<Ast::Boolean>(node))    return this->analyze(std::dynamic_pointer_cast<Ast::Boolean>(node));
	else assert(false);
	return Result<Ok, Error>(Error("Error: This shouldn't happen"));
}

Result<Ok, Error> Context::analyze(std::shared_ptr<Ast::Call> node) {
	// Get types of arguments
	for (size_t i = 0; i < node->args.size(); i++) {
		auto result = this->analyze_expression(node->args[i]);
		if (result.is_error()) return result;
	}

	auto result = this->get_type_of_intrinsic(node);
	if (result.is_error()) assert(false);

	return Result<Ok, Error>(Ok());
}

Result<Ok, Error> Context::analyze(std::shared_ptr<Ast::Number> node) {
	node->type = Type("float64");
	return Result<Ok, Error>(Ok());
}

Result<Ok, Error> Context::analyze(std::shared_ptr<Ast::Identifier> node) {
	Binding* binding = this->get_binding(node->value);
	if (!binding) {
		return Result<Ok, Error>(Error(errors::undefined_variable(node)));
	}
	else {
		node->type = binding->type;
		return Result<Ok, Error>(Ok());
	}
}

Result<Ok, Error> Context::analyze(std::shared_ptr<Ast::Boolean> node) {
	node->type = Type("bool");
	return Result<Ok, Error>(Ok());
}

Binding* Context::get_binding(std::string identifier) {
	if (this->scope.find(identifier) != this->scope.end()) {
		return &(this->scope[identifier]);
	}
	else {
		return nullptr;
	}
}

Result<Ok, Error> Context::get_type_of_intrinsic(std::shared_ptr<Ast::Call> node) {
	static std::unordered_map<std::string, std::vector<std::pair<std::vector<Type>, Type>>> intrinsics = {
		{"+", {
			{{Type("float64"), Type("float64")}, Type("float64")}
		}},
		{"*", {
			{{Type("float64"), Type("float64")}, Type("float64")}
		}},
		{"/", {
			{{Type("float64"), Type("float64")}, Type("float64")}
		}},
		{"-", {
			{{Type("float64"), Type("float64")}, Type("float64")}
		}},
		{"<", {
			{{Type("float64"), Type("float64")}, Type("bool")}
		}},
		{"<=", {
			{{Type("float64"), Type("float64")}, Type("bool")}
		}},
		{">", {
			{{Type("float64"), Type("float64")}, Type("bool")}
		}},
		{">=", {
			{{Type("float64"), Type("float64")}, Type("bool")}
		}},
		{"==", {
			{{Type("bool"), Type("bool")}, Type("bool")},
			{{Type("float64"), Type("float64")}, Type("bool")}
		}},
		{"!=", {
			{{Type("bool"), Type("bool")}, Type("bool")},
			{{Type("float64"), Type("float64")}, Type("bool")}
		}}
	};

	if (intrinsics.find(node->identifier->value) != intrinsics.end()) {
		auto args = this->get_args_types(node);
		auto prototypes = intrinsics[node->identifier->value];
		for (size_t i = 0; i < prototypes.size(); i++) {
			if (args == prototypes[i].first) {
				node->type = prototypes[i].second;
				return Result<Ok, Error>(Ok());
			}
		}

	}
	return Result<Ok, Error>(errors::operation_not_defined_for(node, node->args[0]->type.to_str(), node->args[1]->type.to_str()));
}

std::vector<Type> Context::get_args_types(std::shared_ptr<Ast::Call> node) {
	std::vector<Type> types;
	for (size_t i = 0; i < node->args.size(); i++) {
		types.push_back(node->args[i]->type);
	}
	return types;
}
