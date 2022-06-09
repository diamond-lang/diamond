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

Result<Ok, Error> semantic::Context::get_type_of_intrinsic(ast::CallNode& call) {
	auto& identifier = call.identifier->value;
	if (intrinsics.find(identifier) != intrinsics.end()) {
		auto args = ast::get_types(call.args);
		auto prototypes = intrinsics[identifier];
		for (size_t i = 0; i < prototypes.size(); i++) {
			if (args == prototypes[i].first) {
				if (!call.type.is_type_variable()) {
					call.type = prototypes[i].second;
				}
				return Ok {};
			}
		}
	}

	return Error {};
}

Result<Ok, Error> semantic::Context::get_type_of_user_defined_function(ast::CallNode& call) {
	assert(false);
	return Ok {};
}

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
	semantic::Context context(ast);
	context.current_module = ast.file;
	context.analyze(ast.program);

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
			case ast::Break: break;
			case ast::Continue: break;
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
		type_inference::analyze(*this, &node);
		ast::print((ast::Node*) &node);
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
				this->errors.push_back(errors::reassigning_immutable_variable(*node.identifier, node, this->current_module));
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
	
	auto result = this->get_type_of_intrinsic(node);
	if (result.is_ok()) return Ok {};

	result = this->get_type_of_user_defined_function(node);
	if (result.is_ok()) return Ok {};

	this->errors.push_back(errors::undefined_function(node, this->current_module)); // tested in test/errors/undefined_function.dmd
	return Error {};
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