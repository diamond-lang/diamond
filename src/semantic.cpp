#include <iostream>
#include <unordered_map>
#include <assert.h>

#include "errors.hpp"
#include "semantic.hpp"
#include "intrinsics.hpp"
#include "type_inference.hpp"

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

struct FunctionCall {
	std::string identifier;
	std::vector<Type> args;
	Type return_type = Type("");
};

struct Context {
	std::string file;
	std::vector<std::unordered_map<std::string, Binding>> scopes;
	std::unordered_map<std::string, Type> type_bindings;
	std::vector<FunctionCall> call_stack;
	bool inside_while = false;

	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Block> block);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Assignment> assignment);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Return> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::IfElseStmt> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::WhileStmt> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Expression> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Call> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::IfElseExpr> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Number> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Integer> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Identifier> node);
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Boolean> node);

	Result<Ok, Errors> get_type_of_intrinsic(std::shared_ptr<Ast::Call> node);
	Result<Ok, Errors> get_type_of_user_defined_function(std::shared_ptr<Ast::Call> node);
	std::shared_ptr<Ast::FunctionSpecialization> create_and_analyze_specialization(std::vector<Type> args, std::shared_ptr<Ast::Call> call, std::shared_ptr<Ast::Function> function);

	void add_scope() {this->scopes.push_back(std::unordered_map<std::string, Binding>());}
	void remove_scope() {this->scopes.pop_back();}
	std::unordered_map<std::string, Binding>& current_scope() {return this->scopes[this->scopes.size() - 1];}
	Binding* get_binding(std::string identifier);
	std::vector<std::unordered_map<std::string, Binding>> get_definitions();
	int is_recursive_call(std::shared_ptr<Ast::Call> call);
};

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

int Context::is_recursive_call(std::shared_ptr<Ast::Call> call) {
	for (int i = 0; i < this->call_stack.size(); i++) {
		if (call->identifier->value == this->call_stack[i].identifier
		&& get_args_types(call->args) == this->call_stack[i].args) {
			return i;
		}
	}
	return -1;
}

Type find_concrete_type_for_type_variable_on_expression(std::shared_ptr<Ast::Expression> expression, std::string type_variable) {
	auto if_else = std::dynamic_pointer_cast<Ast::IfElseExpr>(expression);
	auto call = std::dynamic_pointer_cast<Ast::Call>(expression);
	auto integer = std::dynamic_pointer_cast<Ast::Integer>(expression);
	auto number = std::dynamic_pointer_cast<Ast::Number>(expression);
	auto identifier = std::dynamic_pointer_cast<Ast::Identifier>(expression);
	
	if (if_else) {
		Type expr_type = find_concrete_type_for_type_variable_on_expression(if_else->expression, type_variable);
		if (expr_type != Type("")) return expr_type;

		expr_type = find_concrete_type_for_type_variable_on_expression(if_else->else_expression, type_variable);
		if (expr_type != Type("")) return expr_type;
	}
	if (call) {
		if (call->type.to_str() == type_variable) {
			for (size_t i = 0; i < call->args.size(); i++) {
				if (call->args[i]->type.to_str() == type_variable) {
					return find_concrete_type_for_type_variable_on_expression(call->args[i], type_variable);
				}
			}
		}
	}
	else if (integer) {
		if (integer->type.to_str() == type_variable) return Type("int64");
	}
	else if (number) {
		if (number->type.to_str() == type_variable) return Type("float64");
	}
	else if (identifier) {}
	else {
		assert(false);
	}
	return Type("");
}

Type find_concrete_type_for_type_variable_on_block(std::shared_ptr<Ast::Block> block, std::string type_variable) {
	for (size_t i = 0; i < block->statements.size(); i++) {
        auto assignment = std::dynamic_pointer_cast<Ast::Assignment>(block->statements[i]);
        auto return_stmt = std::dynamic_pointer_cast<Ast::Return>(block->statements[i]);
        auto if_else_stmt = std::dynamic_pointer_cast<Ast::IfElseStmt>(block->statements[i]);
		auto while_stmt = std::dynamic_pointer_cast<Ast::WhileStmt>(block->statements[i]);
		auto call = std::dynamic_pointer_cast<Ast::Call>(block->statements[i]);
		auto break_stmt = std::dynamic_pointer_cast<Ast::Break>(block->statements[i]);
        auto continue_stmt = std::dynamic_pointer_cast<Ast::Continue>(block->statements[i]);
        
        if (assignment) {
			return find_concrete_type_for_type_variable_on_expression(assignment->expression, type_variable);
		}
        else if (return_stmt && return_stmt->expression) {
			return find_concrete_type_for_type_variable_on_expression(return_stmt->expression, type_variable);
		}
        else if (if_else_stmt) {
			Type block_type = find_concrete_type_for_type_variable_on_block(if_else_stmt->block, type_variable);
			if (block_type != Type("")) return block_type;

			block_type = find_concrete_type_for_type_variable_on_block(if_else_stmt->else_block, type_variable);
			if (block_type != Type("")) return block_type;
		}
		else if (while_stmt) {
			Type block_type = find_concrete_type_for_type_variable_on_block(if_else_stmt->block, type_variable);
			if (block_type != Type("")) return block_type;
		}
		else if (call) {}
		else if (break_stmt) {}
		else if (continue_stmt) {}
        else {
			assert(false);
		}
    }
	return Type("");
}

Type find_concrete_type_for_type_variable(std::shared_ptr<Ast::FunctionSpecialization> specialization, std::string type_variable) {
	if (std::dynamic_pointer_cast<Ast::Expression>(specialization->body)) {
		return find_concrete_type_for_type_variable_on_expression(std::dynamic_pointer_cast<Ast::Expression>(specialization->body), type_variable);
	}
	if (std::dynamic_pointer_cast<Ast::Block>(specialization->body)) {
		return find_concrete_type_for_type_variable_on_block(std::dynamic_pointer_cast<Ast::Block>(specialization->body), type_variable);
	}
	else {
		assert(false);
	}
	return Type("");
}

// Semantic Analysis
// -----------------
Result<Ok, Errors> semantic::analyze(std::shared_ptr<Ast::Program> program) {
	Context context;
	context.add_scope();
	auto block = std::make_shared<Ast::Block>(program->statements, program->functions, program->line, program->col, program->file);
	return context.analyze(block);
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Block> block) {
	this->add_scope();

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
		else if (std::dynamic_pointer_cast<Ast::WhileStmt>(stmt)) {
			auto node = std::dynamic_pointer_cast<Ast::WhileStmt>(stmt);
			result = this->analyze(node);
		}
		else if (std::dynamic_pointer_cast<Ast::Break>(stmt)
		     ||  std::dynamic_pointer_cast<Ast::Continue>(stmt)) {
			if (!this->inside_while) {
				result = Result<Ok, Errors>(Errors{std::string("Error: break or continue must be used inside a loop")});
			}
			else {
				result = Result<Ok, Errors>(Ok{});
				if (block->type == Type("")) {
					block->type = Type("void");
				}
				else if (block->type != Type("void")) {
					result = Result<Ok, Errors>(Errors{std::string("Error: Incompatible return types")});
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

	this->remove_scope();

	if (errors.size() != 0) return Result<Ok, std::vector<Error>>(errors);
	else                    return Result<Ok, std::vector<Error>>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Assignment> node) {
	Binding binding = Binding(node->identifier->value, node);

	// Get expression type
	auto result = this->analyze(node->expression);
	if (result.is_error()) return result;

	// If nonlocal
	if (node->nonlocal) {
		Binding* aux = this->get_binding(node->identifier->value);
		if (!aux) {
			return Result<Ok, Errors>(Errors{errors::undefined_variable(node->identifier)});
		}
		if (aux->assignment && aux->assignment->expression->type != node->expression->type) {
			return Result<Ok, Errors>(Errors{std::string{"Error: Incompatible type for variable"}});
		} 
		*aux = binding;
	}

	// Normal assignment
	else {
		if (this->current_scope().find(node->identifier->value) != this->current_scope().end()) {
			auto assignment = this->current_scope()[node->identifier->value].assignment;
			if (assignment && !(assignment->is_mutable)) {
				return Result<Ok, Errors>(Errors{errors::reassigning_immutable_variable(node->identifier, this->get_binding(node->identifier->value)->assignment)}); // tested in test/errors/reassigning_immutable_variable.dm
			}
		}
		this->current_scope()[node->identifier->value] = binding;
	}
	
	return Result<Ok, Errors>(Ok());
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
	if (node->condition->type != Type("bool")) return Result<Ok, Errors>(Errors{std::string("Error: The condition in a if must be boolean")});

	auto block = this->analyze(node->block);
	if (block.is_error()) return block;

	if (node->else_block) {
		auto else_block = this->analyze(node->else_block);
		if (else_block.is_error()) return else_block;
	}

	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::WhileStmt> node) {
	auto codition = this->analyze(node->condition);
	if (codition.is_error()) return codition;
	if (node->condition->type != Type("bool")) return Result<Ok, Errors>(Errors{std::string("Error: The condition in a if must be boolean")});

	this->inside_while = true;
	auto block = this->analyze(node->block);
	if (block.is_error()) return block;

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

	// Is a recursive call
	int recursive = this->is_recursive_call(node);
	if (recursive != -1) {
		node->type = this->call_stack[recursive].return_type;
		return Result<Ok, Errors>(Ok());
	}

	// If is intrinsic
	auto result = this->get_type_of_intrinsic(node);
	if (result.is_ok()) return Result<Ok, Errors>(Ok());

	// If is user defined
	result = this->get_type_of_user_defined_function(node);
	if (result.is_ok()) return Result<Ok, Errors>(Ok());

	// Undefined function
	return Result<Ok, Errors>(Errors{errors::undefined_function(node)}); // tested in test/errors/undefined_function.dmd
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
	if (node->type.is_type_variable() && this->type_bindings.find(node->type.to_str()) != this->type_bindings.end()) {
		node->type = this->type_bindings[node->type.to_str()];
	}
	else {
		node->type = Type("float64");
	}
	return Result<Ok, Errors>(Ok());
}

Result<Ok, Errors> Context::analyze(std::shared_ptr<Ast::Integer> node) {
	if (node->type.is_type_variable() && this->type_bindings.find(node->type.to_str()) != this->type_bindings.end()) {
		node->type = this->type_bindings[node->type.to_str()];
	}
	else {
		node->type = Type("int64");
	}
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

Result<Ok, Errors> Context::get_type_of_intrinsic(std::shared_ptr<Ast::Call> node) {
	if (intrinsics.find(node->identifier->value) != intrinsics.end()) {
		auto args = get_args_types(node->args);
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
	// Create error message
	std::string error_message = node->identifier->value + "(";
	for (size_t i = 0; i < node->args.size(); i++) {
		error_message += node->args[i]->type.to_str();
		if (i != node->args.size() - 1) {
			error_message += ", ";
		}
	}
	error_message += ") is not defined by the user\n";

	// Check binding exists
	auto binding = this->get_binding(node->identifier->value);
	if (!binding || !binding->is_function()) return Result<Ok, Errors>(Errors{error_message});

	// Find functions with same number of arguments
	std::vector<std::shared_ptr<Ast::Function>> functions_with_same_arg_size = {};
	for (size_t i = 0; i < binding->methods.size(); i++) {
		if (binding->methods[i]->args.size() == node->args.size()) {
			assert(binding->methods[i]->generic);
			functions_with_same_arg_size.push_back(binding->methods[i]);
		}
	}

	// Check specializations
	for (size_t i = 0; i < functions_with_same_arg_size.size(); i++) {
		auto args = get_args_types(node->args);
		std::shared_ptr<Ast::FunctionSpecialization> specialization = nullptr;
		for (auto it = functions_with_same_arg_size[i]->specializations.begin(); it != functions_with_same_arg_size[i]->specializations.end(); it++) {
			if (get_args_types((*it)->args) == args) {
				specialization = *it;
				break;
			}
		}

		// If no specialization was found
		if (!specialization) {
			specialization = this->create_and_analyze_specialization(args, node, functions_with_same_arg_size[i]);
		}

		// If specialization valid
		if (specialization->valid) {
			node->type = specialization->return_type;
			return Result<Ok, Errors>(Ok());
		}
	}

	return Result<Ok, Errors>(Errors{error_message});
}

std::shared_ptr<Ast::FunctionSpecialization> Context::create_and_analyze_specialization(std::vector<Type> args, std::shared_ptr<Ast::Call> call, std::shared_ptr<Ast::Function> function) {
	assert(function->generic);
	if (function->return_type == Type("")) {
		type_inference::analyze(function);
	}

	// Add new specialization
	auto specialization = std::make_shared<Ast::FunctionSpecialization>(function->line, function->col, function->file);
	specialization->identifier = function->identifier;
	specialization->body = function->body->clone();
	for (size_t i = 0; i < function->args.size(); i++) {
		specialization->args.push_back(std::dynamic_pointer_cast<Ast::Identifier>(function->args[i]->clone()));
	}
	specialization->return_type = function->return_type;

	// Create new context
	Context context;
	context.file = this->file;
	context.scopes = this->get_definitions();
	context.add_scope();
	context.call_stack = this->call_stack;

	// Add type bindings
	for (size_t i = 0; i < specialization->args.size(); i++) {
		// If the arg type is a type variable
		if (specialization->args[i]->type.is_type_variable()) {
			std::string type_variable = specialization->args[i]->type.to_str();

			// If is a new type variable
			if (context.type_bindings.find(type_variable) == context.type_bindings.end()) {
				context.type_bindings[type_variable] = call->args[i]->type;
				specialization->args[i]->type = call->args[i]->type;
			}

			// Else compare wih previous type founded for her
			else {
				if (context.type_bindings[type_variable] != call->args[i]->type) {
					specialization->args[i]->type == call->args[i]->type;
					specialization->valid = false;
					return specialization;
				}
				else {
					specialization->args[i]->type = context.type_bindings[type_variable];
				}
			}
		}
	}

	// If return type is a type variable
	if (specialization->return_type.is_type_variable()) {
		if (context.type_bindings.find(specialization->return_type.to_str()) != context.type_bindings.end()) {
			specialization->return_type = context.type_bindings[specialization->return_type.to_str()];
		}
		else {
			Type concrete_type = find_concrete_type_for_type_variable(specialization, specialization->return_type.to_str());
			context.type_bindings[specialization->return_type.to_str()] = concrete_type;
			specialization->return_type = concrete_type;
		}
	}
	
	context.call_stack.push_back(FunctionCall{specialization->identifier->value, get_args_types(specialization->args), specialization->return_type});

	// Add arguments to new scope
	for (size_t i = 0; i != function->args.size(); i++) {
		auto binding = Binding(function->args[i]->value, call->args[i]);
		context.current_scope()[binding.identifier] = binding;
	}

	// Analyze body as a expression
	if (std::dynamic_pointer_cast<Ast::Expression>(specialization->body)) {
		auto result = context.analyze(std::dynamic_pointer_cast<Ast::Expression>(specialization->body));
		if (result.is_ok()) {
			specialization->valid = true;
			specialization->return_type = std::dynamic_pointer_cast<Ast::Expression>(specialization->body)->type;
			function->specializations.push_back(specialization);
			
			if (specialization->return_type == Type("void")) {
				specialization->body = std::make_shared<Ast::Block>(std::vector<std::shared_ptr<Ast::Node>>{specialization->body}, std::vector<std::shared_ptr<Ast::Function>>{}, specialization->body->line, specialization->body->col, specialization->body->file);
			}
		}
	}

	// Analyze body as a block
	else if (std::dynamic_pointer_cast<Ast::Block>(specialization->body)) {
		auto result = context.analyze(std::dynamic_pointer_cast<Ast::Block>(specialization->body));
		if (result.is_ok()) {
			specialization->valid = true;
			specialization->return_type = std::dynamic_pointer_cast<Ast::Block>(specialization->body)->type;
			if (specialization->return_type == Type("")) {
				specialization->return_type = Type("void");	
			}
			function->specializations.push_back(specialization);
		}
	}
	else assert(false);

	return specialization;
}