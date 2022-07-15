#include "semantic.hpp"
#include "../semantic.hpp"
#include "type_inference.hpp"

// Bindings
// --------
semantic::Binding::Binding(ast::AssignmentNode* assignment) : type(AssignmentBinding), value({(ast::Node*) assignment}) {}
semantic::Binding::Binding(ast::Node* function_argument) : type(FunctionArgumentBinding), value({function_argument}) {}
semantic::Binding::Binding(ast::FunctionNode* function) : type(GenericFunctionBinding), value({(ast::Node*) function}) {}
semantic::Binding::Binding(std::vector<ast::FunctionNode*> functions) {
    this->type = semantic::OverloadedFunctionsBinding;
    for (size_t i = 0; i < functions.size(); i++) {
        this->value.push_back((ast::Node*) functions[i]);
    }
}

ast::AssignmentNode* semantic::get_assignment(semantic::Binding binding) {
    assert(binding.type == semantic::AssignmentBinding);
    return (ast::AssignmentNode*)binding.value[0];
}

ast::Node* semantic::get_function_argument(semantic::Binding binding) {
    assert(binding.type == semantic::FunctionArgumentBinding);
    return binding.value[0];
}

ast::FunctionNode* semantic::get_generic_function(semantic::Binding binding) {
    assert(binding.type == semantic::GenericFunctionBinding);
    return (ast::FunctionNode*) binding.value[0];
}

std::vector<ast::FunctionNode*> semantic::get_overloaded_functions(semantic::Binding binding) {
    assert(binding.type == semantic::OverloadedFunctionsBinding);
    std::vector<ast::FunctionNode*> functions;
    for (auto& function: binding.value) {
        functions.push_back((ast::FunctionNode*)function);
    }
    return functions;
}

ast::Type semantic::get_binding_type(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return ast::get_type(((ast::AssignmentNode*)binding.value[0])->expression);
        case semantic::FunctionArgumentBinding: return ast::get_type(binding.value[0]);
        case semantic::OverloadedFunctionsBinding: return ast::Type("function");
        case semantic::GenericFunctionBinding: return ast::Type("function");
    }
    assert(false);
    return ast::Type("");
}

bool semantic::is_function(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return false;
        case semantic::FunctionArgumentBinding: return false;
        case semantic::OverloadedFunctionsBinding: return true;
        case semantic::GenericFunctionBinding: return true;
    }
    assert(false);
    return false;
}

// Helper functions
// ----------------
semantic::Context::Context(ast::Ast& ast) : ast(ast) {
	// Add intrinsic functions 
	this->add_scope();
	
	for (auto it = intrinsics.begin(); it != intrinsics.end(); it++) {
		auto& identifier = it->first;
		auto& scope = this->current_scope();
		std::vector<ast::FunctionNode*> overloaded_functions;
		for (auto& prototype: it->second) {
			// Create function node
			auto function_node = ast::FunctionNode {};
			function_node.generic = false;

			// Create identifier node
			auto identifier_node = ast::IdentifierNode {};
			identifier_node.value = identifier;
			ast.push_back(identifier_node);

			function_node.identifier = (ast::IdentifierNode*) ast.last_element();

			// Create args nodes
			for (auto& arg: prototype.first) {
				auto arg_node = ast::IdentifierNode {};
				arg_node.type = arg;
				ast.push_back(arg_node);
				function_node.args.push_back(ast.last_element());
			}

			function_node.return_type = prototype.second;

			ast.push_back(function_node);
			overloaded_functions.push_back((ast::FunctionNode*) ast.last_element());
		}
		scope[identifier] = Binding(overloaded_functions);
	}
}

void semantic::Context::add_scope() {
	this->scopes.push_back(std::unordered_map<std::string, semantic::Binding>());
}

void semantic::Context::remove_scope() {
	this->scopes.pop_back();
}

std::unordered_map<std::string, semantic::Binding>& semantic::Context::current_scope() {
	return this->scopes[this->scopes.size() - 1];
}

semantic::Binding* semantic::Context::get_binding(std::string identifier) {
	for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
		if (scope->find(identifier) != scope->end()) {
			return &(*scope)[identifier];
		}
	}
	return nullptr;
}

void semantic::Context::add_functions_to_current_scope(ast::BlockNode& block) {
	// Add functions from current block to current context
	for (size_t i = 0; i < block.functions.size(); i++) {
		auto& function = *block.functions[i];
		auto& identifier = function.identifier->value;

		auto& scope = this->current_scope();
		if (scope.find(identifier) == scope.end()) {
			if (function.generic) {
				scope[identifier] = Binding(block.functions[i]);

			}
			else {
				scope[identifier] = Binding({block.functions[i]});
			}
		}
		else if (is_function(scope[identifier])) {
			if (scope[identifier].type == GenericFunctionBinding) {
				this->errors.push_back(Error{"Error: Trying to overload generic function!"});
			}
			else {
				scope[identifier].value.push_back((ast::Node*) block.functions[i]);
			}
		}
		else {
			assert(false);
		}
	}
}

std::vector<std::unordered_map<std::string, semantic::Binding>> semantic::Context::get_definitions() {
	std::vector<std::unordered_map<std::string, Binding>> scopes;
	for (size_t i = 0; i < this->scopes.size(); i++) {
		scopes.push_back(std::unordered_map<std::string, Binding>());
		for (auto it = this->scopes[i].begin(); it != this->scopes[i].end(); it++) {
			if (semantic::is_function(it->second)) {
				scopes[i][it->first] = it->second;
			}
		}
	}
	return scopes;
}


Result<Ok, Error> semantic::Context::get_type_of_function(ast::CallNode& call) {
	semantic::Binding* binding = this->get_binding(call.identifier->value);
    if (!binding || !is_function(*binding)) {
        this->errors.push_back(errors::undefined_function(call, this->current_module));
        return Error {};
    }
	else if (binding->type == semantic::OverloadedFunctionsBinding) {
		for (auto function: semantic::get_overloaded_functions(*binding)) {
			// Check arguments types match
			if (ast::get_types(function->args) == ast::get_types(call.args)) {
				call.type = function->return_type;
				return Ok {};
			}
		}

		this->errors.push_back(errors::undefined_function(call, this->current_module));
		return Error {};
	}
	else if (binding->type == semantic::GenericFunctionBinding) {
		auto result = this->get_type_of_generic_function(ast::get_types(call.args), semantic::get_generic_function(*binding));
		if (result.is_ok()) {
			call.type = result.get_value();
			return Ok {};
		}
		else {
			return Error {};
		}
	}
	else {
		this->errors.push_back(errors::undefined_function(call, this->current_module));
        return Error {};
	}
}

Result<ast::Type, Error> semantic::Context::get_type_of_generic_function(std::vector<ast::Type> args, ast::FunctionNode* function, std::vector<ast::FunctionPrototype> call_stack) {
	// Check specializations
	for (auto& specialization: function->specializations) {
		if (specialization.args == args) {
			return specialization.return_type;
		}
	}

	// Else check constraints and add specialization
	ast::FunctionSpecialization specialization;

	// Add arguments
	for (size_t i = 0; i < args.size(); i++) {
		ast::Type type = args[i];
		ast::Type type_variable = ast::get_type(function->args[i]);
		
		if (type_variable.is_type_variable()) {
			// If it was no already included
			if (specialization.type_bindings.find(type_variable.to_str()) == specialization.type_bindings.end()) {
				specialization.type_bindings[type_variable.to_str()] = type;	
			}
			// Else compare with previous type founded for her
			else if (specialization.type_bindings[type_variable.to_str()] != type) {
				this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
				return Error {};
			}
		}
		else if (type != type_variable) {
			this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
			return Error {};
		}

		specialization.args.push_back(type);
	}

	// Check constraints
	call_stack.push_back(ast::FunctionPrototype{function->identifier->value, args, ast::Type("")});
	for (auto& constraint: function->constraints) {
		this->check_constraint(specialization.type_bindings, constraint, call_stack);
	}

	// Add return type to specialization
	if (function->return_type.is_type_variable()) {
		specialization.return_type = specialization.type_bindings[function->return_type.to_str()];
	}
	else {
		specialization.return_type = function->return_type;
	}

	// Add specialization to function
	function->specializations.push_back(specialization);

	return specialization.return_type;
}

Result<Ok, Error> semantic::Context::check_constraint(std::unordered_map<std::string, ast::Type>& type_bindings, ast::FunctionConstraint constraint, std::vector<ast::FunctionPrototype> call_stack) {
	if (constraint.identifier == "Number") {
		ast::Type type_variable = constraint.args[0];

		// If type variable was not already included
		if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
			type_bindings[type_variable.to_str()] = ast::Type("int64");	
		}
		// Else compare with previous type founded for her
		else if (type_bindings[type_variable.to_str()] != ast::Type("int64")
		&&       type_bindings[type_variable.to_str()] != ast::Type("float64")) {
			assert(false);
			return Error {};
		}

		return Ok {};
	}
	else if (constraint.identifier == "Float") {
		ast::Type type_variable = constraint.args[0];

		// If return type was not already included
		if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
			type_bindings[type_variable.to_str()] = ast::Type("float64");	
		}
		// Else compare with previous type founded for her
		else if (type_bindings[type_variable.to_str()] != ast::Type("float64")) {
			assert(false);
			return Error {};
		}

		return Ok {};
	}
	else {
		semantic::Binding* binding = this->get_binding(constraint.identifier);
		if (!binding || !is_function(*binding)) {
			this->errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
			return Error {};
		}
		else if (binding->type == semantic::OverloadedFunctionsBinding) {
			for (auto function: semantic::get_overloaded_functions(*binding)) {
				// Check arguments types match
				if (ast::get_types(function->args) == ast::get_concrete_types(constraint.args, type_bindings)) {
					ast::Type type_variable = constraint.return_type;

					// If return type was not already included
					if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
						type_bindings[type_variable.to_str()] = function->return_type;	
					}
					// Else compare with previous type founded for her
					else if (type_bindings[type_variable.to_str()] != function->return_type) {
						this->errors.push_back(Error{"Error: Incompatible types in function constraints"});
						return Error {};
					}
				}
			}
			return Error {};
		}
		else if (binding->type == semantic::GenericFunctionBinding) {
			// Check if constraints are already being checked
			for (auto i = call_stack.rbegin(); i != call_stack.rend(); i++) {
				if (*i == ast::FunctionPrototype{constraint.identifier, ast::get_concrete_types(constraint.args, type_bindings), ast::Type("")}) {
					return Ok {};
				}
			}

			// Check constraints otherwise
			auto result = this->get_type_of_generic_function(ast::get_concrete_types(constraint.args, type_bindings), semantic::get_generic_function(*binding), call_stack);
			if (result.is_error()) return Error {};
		}
		else {
			this->errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
			return Error {};
		}

		return Ok {};
	}
}

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
	semantic::Context context(ast);
	context.current_module = ast.file;
	context.analyze((ast::Node*) ast.program);

	if (context.errors.size() > 0) return context.errors;
	else                           return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::Node* node) {
	return std::visit([this](auto& variant) {return this->analyze(variant);}, *node);
}

Result<Ok, Error> semantic::Context::analyze(ast::BlockNode& node) {
	node.type = ast::Type("");
	size_t number_of_errors = this->errors.size();

	// Add scope
	this->add_scope();

	// Add functions to the current scope
	this->add_functions_to_current_scope(node);

	// Analyze functions
	for (size_t i = 0; i < node.functions.size(); i++) {
		this->analyze(*node.functions[i]);
	}

	// Analyze statements
	for (size_t i = 0; i < node.statements.size(); i++) {
		// Analyze
		auto result = this->analyze(node.statements[i]);

		// Type checking
		switch (node.statements[i]->index()) {
			case ast::Assignment: break;
			case ast::Call: {
				if (result.is_ok() && ast::get_type(node.statements[i]) != ast::Type("void")) {
					this->errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*node.statements[i]), this->current_module)); // tested in test/errors/unhandled_return_value.dm
				}
				break;
			}
			case ast::Return: {
				if (result.is_ok()) {
					auto return_type = std::get<ast::ReturnNode>(*node.statements[i]).expression.has_value() ? ast::get_type(std::get<ast::ReturnNode>(*node.statements[i]).expression.value()) : ast::Type("void");
					if (node.type == ast::Type("")) node.type = return_type;
					else if (node.type != return_type) this->errors.push_back(Error("Error: Incompatible return type"));
				}
				break;
			}
			case ast::IfElse: {
				if (result.is_ok()) {
					auto if_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).if_branch);
					if (if_type != ast::Type("")) {
						if (node.type == ast::Type("")) node.type = if_type;
						else if (node.type != if_type) this->errors.push_back(Error("Error: Incompatible return type"));
					}

					if (std::get<ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
						auto else_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).else_branch.value());
						if (else_type != ast::Type("")) {
							if (node.type == ast::Type("")) node.type = else_type;
							else if (node.type != else_type) this->errors.push_back(Error("Error: Incompatible return type"));
						}
					}
				}
				break;
			}
			case ast::While: break;
			case ast::Break:
			case ast::Continue: {
				if (node.type == ast::Type("")) {
					node.type = ast::Type("void");
				}
				else if (node.type != ast::Type("void")) {
					this->errors.push_back(Error{"Error: Incompatible return types"});
				}
				break;
			}
			default: assert(false);
		}
	}

	// Remove scope
	this->remove_scope();

	if (this->errors.size() > number_of_errors) return Error {};
	else                                        return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::FunctionNode& node) {
	if (node.generic) {
		if (node.return_type == ast::Type("")) {
			auto result = type_inference::analyze(*this, &node);
			if (result.is_error()) return Error {};	
		}
	}
	else assert(false);
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::AssignmentNode& node) {
	auto& identifier = node.identifier->value;

	// Analyze expression
	auto result = this->analyze(node.expression);
	if (result.is_error()) return result;

	// nonlocal assignment
	if (node.nonlocal) {
		Binding* binding = this->get_binding(identifier);
		if (!binding) {
			this->errors.push_back(errors::undefined_variable(*node.identifier, this->current_module));
			return Error {};
		}
		if (semantic::get_binding_type(*binding) != ast::get_type(node.expression)) {
			this->errors.push_back(std::string("Error: Incompatible type for variable"));
			return Error {};
		}
		*binding = &node;
	}

	// normal assignment
	else {
		if (this->current_scope().find(identifier) != this->current_scope().end()
		&& this->current_scope()[identifier].type == AssignmentBinding) {
			auto assignment = get_assignment(this->current_scope()[identifier]);
			if (!assignment->is_mutable) {
				this->errors.push_back(errors::reassigning_immutable_variable(*node.identifier, *assignment, this->current_module));
				return Error {};
			}
		}
		this->current_scope()[identifier] = &node;
	}

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::ReturnNode& node) {
	if (node.expression.has_value()) {
		auto result = this->analyze(node.expression.value());
		if (result.is_error()) return result;
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IfElseNode& node) {
	auto condition = this->analyze(node.condition);
	if (condition.is_error()) return condition;
	if (ast::get_type(node.condition) != ast::Type("bool")) {
		this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
	}

	auto if_branch = this->analyze(node.if_branch);
	if (if_branch.is_error()) return if_branch;

	if (!node.else_branch.has_value()) return Ok {};

	auto else_branch = this->analyze(node.else_branch.value());
	if (else_branch.is_error()) return else_branch;

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::WhileNode& node) {
	auto condition = this->analyze(node.condition);
	if (condition.is_error()) return condition;
	if (ast::get_type(node.condition) != ast::Type("bool")) {
		this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
	}

	auto block = this->analyze(node.block);
	if (block.is_error()) return block;

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::CallNode& node) {
	for (size_t i = 0; i < node.args.size(); i++) {
		auto result = this->analyze(node.args[i]);
		if (result.is_error()) return result;
	}
	
	return this->get_type_of_function(node);
}

Result<Ok, Error> semantic::Context::analyze(ast::FloatNode& node) {
	node.type = ast::Type("float64");
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IntegerNode& node) {
	node.type = ast::Type("int64");
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IdentifierNode& node) {
	Binding* binding = this->get_binding(node.value);
	if (!binding) {
		this->errors.push_back(errors::undefined_variable(node, this->current_module)); // tested in test/errors/undefined_variable.dm
		return Error {};
	}
	else {
		node.type = semantic::get_binding_type(*binding);
		return Ok {};
	}
}

Result<Ok, Error> semantic::Context::analyze(ast::BooleanNode& node) {
	node.type = ast::Type("bool");
	return Ok {};
}