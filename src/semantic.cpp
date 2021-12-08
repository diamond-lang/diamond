#include <iostream>
#include <unordered_map>
#include <assert.h>

#include "errors.hpp"
#include "semantic.hpp"

enum BindingType {
	AssignmentBinding,
	FunctionBinding,
	FunctionArgumentBinding
};

struct Binding {
	std::string identifier;
	BindingType binding_type;
	std::shared_ptr<Ast::Assignment> assignment;
	std::shared_ptr<Ast::Expression> function_argument;
	std::vector<std::shared_ptr<Ast::Function>> methods;

	Binding() {}
	Binding(std::string identifier, std::shared_ptr<Ast::Assignment> assignment) : identifier(identifier), binding_type(AssignmentBinding), assignment(assignment) {}
	Binding(std::string identifier, std::shared_ptr<Ast::Expression> function_argument) : identifier(identifier), binding_type(FunctionArgumentBinding), function_argument(function_argument) {}
	Binding(std::string identifier, std::vector<std::shared_ptr<Ast::Function>> methods) : identifier(identifier), binding_type(FunctionBinding), methods(methods) {}

	Type get_type() {
		if (this->binding_type == AssignmentBinding) return this->assignment->expression->type;
		if (this->binding_type == FunctionArgumentBinding) return this->function_argument->type;
		if (this->binding_type == FunctionBinding) return Type("function");
		else                                       assert(false);
	}

	bool is_assignment() {return this->binding_type == AssignmentBinding;}
	bool is_function() {return this->binding_type == FunctionBinding;}
	bool is_argument() {return this->binding_type == FunctionArgumentBinding;}
};

struct Context {
	std::string file;
	std::vector<std::unordered_map<std::string, Binding>> scopes;

	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Block> block);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Assignment> assignment);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Return> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::IfElseStmt> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Expression> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Call> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::IfElseExpr> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Number> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Integer> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Identifier> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Boolean> node);

	void add_scope() {this->scopes.push_back(std::unordered_map<std::string, Binding>());}
	std::unordered_map<std::string, Binding>& current_scope() {return this->scopes[this->scopes.size() - 1];}
	Binding* get_binding(std::string identifier);
	Result<Ok, Errors> get_type_of_intrinsic(std::shared_ptr<Ast::Call> node);
	Result<Ok, Errors> get_type_of_user_defined_function(std::shared_ptr<Ast::Call> node);
	std::vector<Type> get_args_types(std::shared_ptr<Ast::Call> node);
	std::vector<std::unordered_map<std::string, Binding>> get_definitions();
};


// Implementations
// ---------------
Result<Ok, Errors> semantic::analyze(std::shared_ptr<Ast::Program> program) {
	Context context;
	context.add_scope();
	auto block = std::make_shared<Ast::Block>(program->statements, program->functions, program->line, program->col, program->file);
	return context.analyze(block);
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Block> block) {
	block->type = Type("");;
	Errors errors;

	// Add functions to the current scope
	for (size_t i = 0; i < block->functions.size(); i++) {
		auto function = block->functions[i];

		auto& scope = this->current_scope();
		if (scope.find(function->identifier->value) == scope.end()) {
			auto binding = Binding(function->identifier->value, {function});
			scope[function->identifier->value] = binding;
		}
		else {
			scope[function->identifier->value].methods.push_back(function);
		}
	}

	for (size_t i = 0; i  < block->statements.size(); i++) {
		std::shared_ptr<Ast::Node> stmt = block->statements[i];
		Result<Ok, Errors> result;
		
		if (std::dynamic_pointer_cast<Ast::Assignment>(stmt)) {
			result = this->analyze(std::dynamic_pointer_cast<Ast::Assignment>(stmt));
		}
		else if (std::dynamic_pointer_cast<Ast::Call>(stmt)) {
			auto node = std::dynamic_pointer_cast<Ast::Call>(stmt);
			result = this->analyze(node);
			if (result.is_ok() && node->type != Type("void")) {
				result = Result<Ok, Errors>(Errors{errors::unhandled_return_value(node)}); // tested in test/errors/unhandled_return_value.dm
			}
		}
		else if (std::dynamic_pointer_cast<Ast::Return>(stmt)) {
			auto node = std::dynamic_pointer_cast<Ast::Return>(stmt);
			result = this->analyze(node);
			if (result.is_ok()) {
				auto return_type = node->expression ? node->expression->type : Type("void");
				if (block->type == Type("")) {
					block->type = return_type;
				}
				else if (block->type != return_type) {
					result = Result<Ok, Errors>(Errors{std::string("Error: Incompatible return types")});
				}	
			}
		}
		else if (std::dynamic_pointer_cast<Ast::IfElseStmt>(stmt)) {
			auto node = std::dynamic_pointer_cast<Ast::IfElseStmt>(stmt);
			result = this->analyze(node);
			if (result.is_ok()) {
				if (node->block->type != Type("")) {
					if (block->type == Type("")) {
						block->type = node->block->type;
					}
					else if (block->type != node->block->type) {
						result = Result<Ok, Errors>(Errors{std::string("Error: Incompatible return types")});
					}
				}

				if (node->else_block && node->else_block->type != Type("")) {
					if (block->type == Type("")) {
						block->type = node->else_block->type;
					}
					else if (block->type != node->else_block->type) {
						result = Result<Ok, Errors>(Errors{std::string("Error: Incompatible return types")});
					}
				}
			}
		}
		else  {
			assert(false);
		}

		if (result.is_error()) {
			auto result_errors = result.get_errors();
			errors.insert(errors.begin(), result_errors.begin(), result_errors.end());
		}
	}

	if (errors.size() != 0) return Result<Ok, std::vector<Error>>(errors);
	else                    return Result<Ok, std::vector<Error>>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Assignment> node) {
	Binding binding = Binding(node->identifier->value, node);

	// Get expression type
	auto result = this->analyze(node->expression);
	if (result.is_error()) return result;

	// Save it context
	if (this->get_binding(node->identifier->value)) {
		return Result<Ok, Errors>(Errors{errors::reassigning_immutable_variable(node->identifier, this->get_binding(node->identifier->value)->assignment)}); // tested in test/errors/reassigning_immutable_variable.dm
	}
	else {
		this->current_scope()[node->identifier->value] = binding;
		return Result<Ok, Errors>(Ok());
	}
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Return> node) {
	// Get expression type
	if (node->expression) {
		auto result = this->analyze(node->expression);
		if (result.is_error()) return result;
	}

	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::IfElseStmt> node) {
	auto codition = this->analyze(node->condition);
	if (codition.is_error()) return codition;
	if (node->condition->type != Type("bool")) return Result<Ok, Errors>(Errors{std::string("The condition in a if must be boolean")});

	auto block = this->analyze(node->block);
	if (block.is_error()) return block;

	if (node->else_block) {
		auto else_block = this->analyze(node->else_block);
		if (else_block.is_error()) return else_block;
	}

	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Expression> node) {
	if      (std::dynamic_pointer_cast<Ast::Call>(node))       return this->analyze(std::dynamic_pointer_cast<Ast::Call>(node));
	if      (std::dynamic_pointer_cast<Ast::IfElseExpr>(node)) return this->analyze(std::dynamic_pointer_cast<Ast::IfElseExpr>(node));
	else if (std::dynamic_pointer_cast<Ast::Number>(node))     return this->analyze(std::dynamic_pointer_cast<Ast::Number>(node));
	else if (std::dynamic_pointer_cast<Ast::Integer>(node))    return this->analyze(std::dynamic_pointer_cast<Ast::Integer>(node));
	else if (std::dynamic_pointer_cast<Ast::Identifier>(node)) return this->analyze(std::dynamic_pointer_cast<Ast::Identifier>(node));
	else if (std::dynamic_pointer_cast<Ast::Boolean>(node))    return this->analyze(std::dynamic_pointer_cast<Ast::Boolean>(node));
	else assert(false);
	return Result<Ok, Errors>(Errors{std::string("Error: This shouldn't happen")});
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Call> node) {
	// Get types of arguments
	for (size_t i = 0; i < node->args.size(); i++) {
		auto result = this->analyze(node->args[i]);
		if (result.is_error()) return result;
	}

	auto result = this->get_type_of_intrinsic(node);
	if (result.is_error()) {
		result = this->get_type_of_user_defined_function(node);
		if (result.is_error()) return Result<Ok, Errors>(Errors{errors::undefined_function(node)}); // tested in test/errors/undefined_function.dm
	}

	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::IfElseExpr> node) {
	auto codition = this->analyze(node->condition);
	if (codition.is_error()) return codition;
	if (node->condition->type != Type("bool")) return Result<Ok, Errors>(Errors{std::string("The condition in a if must be boolean")});

	auto expression = this->analyze(node->expression);
	if (expression.is_error()) return expression;

	assert(node->else_expression);
	auto else_expression = this->analyze(node->else_expression);
	if (else_expression.is_error()) return else_expression;

	if (node->expression->type != node->else_expression->type) {
		return Result<Ok, Errors>(Errors{std::string("Then and else branch type are not the same")});
	}

	node->type = node->expression->type;

	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Number> node) {
	node->type = Type("float64");
	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Integer> node) {
	node->type = Type("int64");
	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Identifier> node) {
	Binding* binding = this->get_binding(node->value);
	if (!binding) {
		return Result<Ok, Errors>(Errors{errors::undefined_variable(node)}); // tested in test/errors/undefined_variable.dm
	}
	else {
		node->type = binding->get_type();
		return Result<Ok, Errors>(Ok());
	}
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Boolean> node) {
	node->type = Type("bool");
	return Result<Ok, Errors>(Ok());
}

Binding* Context::get_binding(std::string identifier) {
	for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
		if (scope->find(identifier) != scope->end()) {
			return &(*scope)[identifier];
		}
	}
	return nullptr;
}

std::vector<std::unordered_map<std::string, Binding>> Context::get_definitions() {
	std::vector<std::unordered_map<std::string, Binding>> scopes;
	for (size_t i = 0; i < this->scopes.size(); i++) {
		scopes.push_back(std::unordered_map<std::string, Binding>());
		for (auto it = this->scopes[i].begin(); it != this->scopes[i].end(); it++) {
			if (it->second.is_function()) {
				scopes[i][it->first] = it->second;
			}
		}
	}
	return scopes;
}

Result<Ok, Errors> Context::get_type_of_intrinsic(std::shared_ptr<Ast::Call> node) {
	static std::unordered_map<std::string, std::vector<std::pair<std::vector<Type>, Type>>> intrinsics = {
		{"+", {
			{{Type("float64"), Type("float64")}, Type("float64")},
			{{Type("int64"), Type("int64")}, Type("int64")}
		}},
		{"*", {
			{{Type("float64"), Type("float64")}, Type("float64")},
			{{Type("int64"), Type("int64")}, Type("int64")}
		}},
		{"/", {
			{{Type("float64"), Type("float64")}, Type("float64")},
			{{Type("int64"), Type("int64")}, Type("int64")}
		}},
		{"-", {
			{{Type("float64")}, Type("float64")},
			{{Type("int64")}, Type("int64")},
			{{Type("float64"), Type("float64")}, Type("float64")},
			{{Type("int64"), Type("int64")}, Type("int64")},
		}},
		{"<", {
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{"<=", {
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{">", {
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{">=", {
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{"==", {
			{{Type("bool"), Type("bool")}, Type("bool")},
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{"!=", {
			{{Type("bool"), Type("bool")}, Type("bool")},
			{{Type("float64"), Type("float64")}, Type("bool")},
			{{Type("int64"), Type("int64")}, Type("bool")}
		}},
		{"not", {
			{{Type("bool")}, Type("bool")}
		}},
		{"print", {
			{{Type("float64")}, Type("void")},
			{{Type("int64")}, Type("void")},
			{{Type("bool")}, Type("void")}
		}}
	};

	if (intrinsics.find(node->identifier->value) != intrinsics.end()) {
		auto args = this->get_args_types(node);
		auto prototypes = intrinsics[node->identifier->value];
		for (size_t i = 0; i < prototypes.size(); i++) {
			if (args == prototypes[i].first) {
				node->type = prototypes[i].second;
				return Result<Ok, Errors>(Ok());
			}
		}
	}

	std::string error_message = node->identifier->value + "(";
	for (size_t i = 0; i < node->args.size(); i++) {
		error_message += node->args[i]->type.to_str();
		if (i != node->args.size() - 1) {
			error_message += ", ";
		}
	}
	error_message += ") is not an intrinsic function";
	return Result<Ok, Errors>(Errors{error_message});
}

Result<Ok, Errors> Context::get_type_of_user_defined_function(std::shared_ptr<Ast::Call> node) {
	// If function binding exists
	auto binding = this->get_binding(node->identifier->value);
	if (binding && binding->is_function()) {

		// Find method with that match arguments types
		auto methods = binding->methods;
		for (auto method = methods.begin(); method != methods.end(); method++) {

			// If the method has the same argument size as the call
			if ((*method)->args.size() == node->args.size()) {
				assert((*method)->generic);

				// Check if there is a specialization that match arguments types
				auto args = this->get_args_types(node);
				std::shared_ptr<Ast::FunctionSpecialization>* specialization = nullptr;
				for (auto it = (*method)->specializations.begin(); it != (*method)->specializations.end(); it++) {
					if ((*it)->args_types == args) {
						specialization = &(*it);
						break;
					}
				}

				// If no specialization was founded
				if (!specialization) {
					// Add new specialization
					auto aux = std::make_shared<Ast::FunctionSpecialization>();
					specialization = &aux;
					(*specialization)->args_types = args;
					(*specialization)->body = (*method)->body->clone();

					// Create new context
					Context context;
					context.file = this->file;
					context.scopes = this->get_definitions();
					context.add_scope();

					// Add arguments to new scope
					for (size_t i = 0; i != node->args.size(); i++) {
						auto binding = Binding((*method)->args[i]->value, node->args[i]);
						context.current_scope()[binding.identifier] = binding;
					}

				
					if (std::dynamic_pointer_cast<Ast::Expression>((*specialization)->body)) {
						auto result = context.analyze(std::dynamic_pointer_cast<Ast::Expression>((*specialization)->body));
						if (result.is_ok()) {
							(*specialization)->valid = true;
							(*specialization)->return_type = std::dynamic_pointer_cast<Ast::Expression>((*specialization)->body)->type;
							(*method)->specializations.push_back(*specialization);
						}
					}
					else if (std::dynamic_pointer_cast<Ast::Block>((*specialization)->body)) {
						auto result = context.analyze(std::dynamic_pointer_cast<Ast::Block>((*specialization)->body));
						if (result.is_ok()) {
							(*specialization)->valid = true;
							(*specialization)->return_type = std::dynamic_pointer_cast<Ast::Block>((*specialization)->body)->type;
							if ((*specialization)->return_type == Type("")) {
								(*specialization)->return_type = Type("void");	
							}
							(*method)->specializations.push_back(*specialization);
						}
					}
					else assert(false);
				}

				// If specialization valid
				if ((*specialization)->valid) {
					node->type = (*specialization)->return_type;
					return Result<Ok, Errors>(Ok());
				}
			}
		}
	}

	std::string error_message = node->identifier->value + "(";
	for (size_t i = 0; i < node->args.size(); i++) {
		error_message += node->args[i]->type.to_str();
		if (i != node->args.size() - 1) {
			error_message += ", ";
		}
	}
	error_message += ") is not defined by the user\n";
	return Result<Ok, Errors>(Errors{error_message});
}

std::vector<Type> Context::get_args_types(std::shared_ptr<Ast::Call> node) {
	std::vector<Type> types;
	for (size_t i = 0; i < node->args.size(); i++) {
		types.push_back(node->args[i]->type);
	}
	return types;
}
