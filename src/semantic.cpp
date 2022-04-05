#include <unordered_map>
#include <cassert>
#include <filesystem>
#include <set>
#include <iostream>

#include "errors.hpp"
#include "semantic.hpp"
#include "intrinsics.hpp"
#include "type_inference.hpp"
#include "utilities.hpp"
#include "lexer.hpp"
#include "parser.hpp"

// Prototypes and definitions
// --------------------------
namespace semantic {
	using Binding = std::variant<
		ast::AssignmentNode*,
		ast::Node*,
		std::vector<ast::FunctionNode*>
	>;

	ast::Type get_binding_type(Binding& binding) {
		if (std::holds_alternative<ast::AssignmentNode*>(binding)) return ast::get_type(std::get<ast::AssignmentNode*>(binding)->expression);
		else if (std::holds_alternative<ast::Node*>(binding)) return ast::get_type(std::get<ast::Node*>(binding));
		else if (std::holds_alternative<std::vector<ast::FunctionNode*>>(binding)) return ast::Type("function");
		else assert(false);
		return ast::Type("");
	}

	bool is_function(Binding& binding) {
		if (std::holds_alternative<std::vector<ast::FunctionNode*>>(binding)) return true;
		else return false;
	}

	struct FunctionCall {
		std::unordered_map<std::string, ast::Type> type_bindings;
		ast::FunctionNode* function_node;
	};

	struct Context {
		std::filesystem::path current_module;
		std::vector<std::unordered_map<std::string, Binding>> scopes;
		std::vector<FunctionCall> call_stack;
		bool inside_loop = false;
		Errors errors;
		ast::Ast& ast;

		Context(ast::Ast& ast) : ast(ast) {}

		Result<Ok, Error> analyze(ast::Node* node);
		Result<Ok, Error> analyze(ast::BlockNode& node);
		Result<Ok, Error> analyze(ast::FunctionNode& node) {return Ok {};}
		Result<Ok, Error> analyze(ast::AssignmentNode& node);
		Result<Ok, Error> analyze(ast::ReturnNode& node);
		Result<Ok, Error> analyze(ast::BreakNode& node) {return Ok {};}
		Result<Ok, Error> analyze(ast::ContinueNode& node) {return Ok {};}
		Result<Ok, Error> analyze(ast::IfElseNode& node);
		Result<Ok, Error> analyze(ast::WhileNode& node);
		Result<Ok, Error> analyze(ast::UseNode& node) {return Ok {};}
		Result<Ok, Error> analyze(ast::IncludeNode& node) {return Ok {};}
		Result<Ok, Error> analyze(ast::CallNode& node);
		Result<Ok, Error> analyze(ast::FloatNode& node);
		Result<Ok, Error> analyze(ast::IntegerNode& node);
		Result<Ok, Error> analyze(ast::IdentifierNode& node);
		Result<Ok, Error> analyze(ast::BooleanNode& node);
		Result<Ok, Error> analyze(ast::StringNode& node) {return Ok {};}

		void add_scope() {
			this->scopes.push_back(std::unordered_map<std::string, Binding>());
		}

		void remove_scope() {
			this->scopes.pop_back();
		}

		std::unordered_map<std::string, Binding>& current_scope() {
			return this->scopes[this->scopes.size() - 1];
		}

		Binding* get_binding(std::string identifier) {
			for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
				if (scope->find(identifier) != scope->end()) {
					return &(*scope)[identifier];
				}
			}
			return nullptr;
		}

		ast::Type get_type(ast::Node* node) {
			if (ast::get_type(node).is_type_variable()) {
				assert(this->call_stack.size() > 0);
				return this->call_stack[this->call_stack.size() - 1].type_bindings[ast::get_type(node).to_str()];
			}
			else {
				return ast::get_type(node);
			}
		}

		ast::Type get_type(ast::Type type) {
			if (type.is_type_variable()) {
				assert(this->call_stack.size() > 0);
				return this->call_stack[this->call_stack.size() - 1].type_bindings[type.to_str()];
			}
			else {
				return type;
			}
		}

		std::vector<ast::Type> get_types(std::vector<ast::Node*> nodes) {
			std::vector<ast::Type> types;
			for (size_t i = 0; i < nodes.size(); i++) {
				types.push_back(this->get_type(nodes[i]));
			}
			return types;
		}

		void add_module_functions(std::filesystem::path module_path, std::set<std::filesystem::path> already_included_modules = {}) {
			if (this->ast.modules.find(module_path.string()) == this->ast.modules.end()) {
				// Read file
				Result<std::string, Error> result = utilities::read_file(module_path.string());
				if (result.is_error()) {
					std::cout << result.get_error().value;
					exit(EXIT_FAILURE);
				}
				std::string file = result.get_value();

				// Lex
				auto tokens = lexer::lex(module_path);
				if (tokens.is_error()) {
					for (size_t i = 0; i < tokens.get_error().size(); i++) {
						std::cout << tokens.get_error()[i].value << "\n";
					}
					exit(EXIT_FAILURE);
				}

				// Parse
				auto parsing_result = parse::module(this->ast, tokens.get_value(), module_path);
				if (parsing_result.is_error()) {
					std::vector<Error> errors = parsing_result.get_errors();
					for (size_t i = 0; i < errors.size(); i++) {
						std::cout << errors[i].value << '\n';
					}
					exit(EXIT_FAILURE);
				}
			}
		}

		void add_functions_to_current_scope(ast::BlockNode& block) {
			// Add functions from current block to current context
			for (size_t i = 0; i < block.functions.size(); i++) {
				auto& function = std::get<ast::FunctionNode>(*block.functions[i]);
				auto& identifier = std::get<ast::IdentifierNode>(*function.identifier).value;

				auto& scope = this->current_scope();
				if (scope.find(identifier) == scope.end()) {
					auto binding = std::vector<ast::FunctionNode*>{(ast::FunctionNode*) block.functions[i]};
					scope[identifier] = binding;
				}
				else if (std::holds_alternative<std::vector<ast::FunctionNode*>>(scope[identifier])) {
					std::get<std::vector<ast::FunctionNode*>>(scope[identifier]).push_back((ast::FunctionNode*) block.functions[i]);
				}
			}

			auto current_directory = std::filesystem::canonical(std::filesystem::current_path() / this->current_module).parent_path();

			// Parse used modules yet not parsed
			for (size_t i = 0; i < block.use_include_statements.size(); i++) {
				std::string path;
				if (std::holds_alternative<ast::UseNode>(*block.use_include_statements[i])) {
					path = std::get<ast::StringNode>(*std::get<ast::UseNode>(*block.use_include_statements[i]).path).value;
				}
				else {
					path = std::get<ast::StringNode>(*std::get<ast::IncludeNode>(*block.use_include_statements[i]).path).value;
				}
				
				auto module_path = current_directory / (path + ".dmd");
				assert(std::filesystem::exists(module_path));
				module_path = std::filesystem::canonical(module_path);
				this->add_module_functions(module_path);
			}
		}

		int is_recursive_call(ast::CallNode& call) {
			auto& identifier = std::get<ast::IdentifierNode>(*call.identifier).value;

			for (int i = 0; i < this->call_stack.size(); i++) {
				if (identifier == std::get<ast::IdentifierNode>(*this->call_stack[i].function_node->identifier).value
				&& this->get_types(call.args) == this->get_types(this->call_stack[i].function_node->args)) {
					return i;
				}
			}
			return -1;
		}

		Result<Ok, Error> get_type_of_intrinsic(ast::CallNode& call) {
			auto& identifier = std::get<ast::IdentifierNode>(*call.identifier).value;
			if (intrinsics.find(identifier) != intrinsics.end()) {
				auto args = this->get_types(call.args);
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

		ast::Type find_concrete_type_for_type_variable(ast::Node* node, std::string type_variable) {
			switch (node->index()) {
				case ast::Block: {
					auto& block = std::get<ast::BlockNode>(*node);
					for (size_t i = 0; i < block.statements.size(); i++) {
						auto stmt_type = this->find_concrete_type_for_type_variable(block.statements[i], type_variable);
						if (stmt_type != ast::Type("")) return stmt_type;
					}
					break;
				}
				case ast::Assignment: {
					return this->find_concrete_type_for_type_variable(std::get<ast::AssignmentNode>(*node).expression, type_variable);
				}
				case ast::Return: {
					auto& return_node = std::get<ast::ReturnNode>(*node);
					if (return_node.expression.has_value()) return this->find_concrete_type_for_type_variable(return_node.expression.value(), type_variable);
					break;
				}
				case ast::While: {
					return this->find_concrete_type_for_type_variable(std::get<ast::WhileNode>(*node).block, type_variable);
				}
				case ast::Break:
				case ast::Continue: {
					break;
				}
				case ast::IfElse: {
					auto& if_else = std::get<ast::IfElseNode>(*node);
					auto if_branch = this->find_concrete_type_for_type_variable(if_else.if_branch, type_variable);
					if (if_branch != ast::Type("")) return if_branch;

					if (if_else.else_branch.has_value()) {
						auto else_branch = this->find_concrete_type_for_type_variable(if_else.else_branch.value(), type_variable);
						if (else_branch != ast::Type("")) return else_branch;
					}
					break;
				}
				case ast::Call: {
					auto& call = std::get<ast::CallNode>(*node);
					if (call.type == type_variable) {
						for (size_t i = 0; i < call.args.size(); i++) {
							if (ast::get_type(call.args[i]).to_str() == type_variable) {
								return this->find_concrete_type_for_type_variable(call.args[i], type_variable);
							}
						}
					}
					break;
				}
				case ast::Float: {
					if (ast::get_type(node) == type_variable) return ast::Type("float64");
					break;
				}
				case ast::Integer: {
					if (ast::get_type(node) == type_variable) return ast::Type("int64");
					break;
				}
				case ast::Identifier: {
					break;
				}
				default: assert(false);
			}
			return ast::Type("");
		}

		ast::FunctionSpecialization create_and_analyze_specialization(ast::CallNode& call, ast::FunctionNode& function) {
			assert(function.generic);

			// Typer infer function is not type inferred yet
			if (function.return_type == ast::Type("")) {
				type_inference::analyze(&function);
			}

			// Add new specialization
			ast::FunctionSpecialization specialization;
			specialization.args = ast::get_types(function.args);
			specialization.return_type = function.return_type;

			// Create new context
			Context context(this->ast);
			context.add_scope();
			context.call_stack = this->call_stack;
			context.current_module = this->current_module;
			context.scopes = this->get_definitions();

			// Create function call
			FunctionCall function_call;
			function_call.function_node = &function;

			// Add type bindings
			for (size_t i = 0; i < specialization.args.size(); i++) {
				// If the arg type is a type variable
				if (specialization.args[i].is_type_variable()) {
					std::string type_variable = specialization.args[i].to_str();

					// If is a new type variable
					if (function_call.type_bindings.find(type_variable) == function_call.type_bindings.end()) {
						function_call.type_bindings[type_variable] = this->get_type(call.args[i]);
						specialization.args[i] = this->get_type(call.args[i]);
					}

					// Else compare wih previous type founded for her
					else {
						if (function_call.type_bindings[type_variable] != this->get_type(call.args[i])) {
							specialization.valid = false;
							return specialization;
						}
						else {
							specialization.args[i] = function_call.type_bindings[type_variable];
						}
					}
				}
			}

			// If return type is a type variable
			if (specialization.return_type.is_type_variable()) {
				if (function_call.type_bindings.find(specialization.return_type.to_str()) != function_call.type_bindings.end()) {
					specialization.return_type = function_call.type_bindings[specialization.return_type.to_str()];
				}
				else {
					ast::Type concrete_type = this->find_concrete_type_for_type_variable(function.body, specialization.return_type.to_str());
					function_call.type_bindings[specialization.return_type.to_str()] = concrete_type;
					specialization.return_type = concrete_type;
				}
			}

			// Add function to call stack to be able to detect recursion
			context.call_stack.push_back(function_call);

			// Add arguments to new scope
			for (size_t i = 0; i != function.args.size(); i++) {
				auto identifier = std::get<ast::IdentifierNode>(*function.args[i]).value;
				context.current_scope()[identifier] = function.args[i];
			}

			// Analyze body
			auto result = context.analyze(function.body);
			if (result.is_ok()) {
				specialization.valid = true;
				specialization.return_type = context.get_type(function.body);
			}
			else {
				this->errors.insert(this->errors.begin(), context.errors.begin(), context.errors.end());
			}

			return specialization;
		}

		Result<Ok, Error> get_type_of_user_defined_function(ast::CallNode& call) {
			auto& identifier = std::get<ast::IdentifierNode>(*call.identifier).value;

			// Create error message
			std::string error_message = identifier + "(";
			for (size_t i = 0; i < call.args.size(); i++) {
				error_message += this->get_type(call.args[i]).to_str();
				if (i != call.args.size() - 1) {
					error_message += ", ";
				}
			}
			error_message += ") is not defined by the user\n";

			// Check binding exists
			Binding* binding = this->get_binding(identifier);
			if (!binding || !is_function(*binding)) {
				this->errors.push_back(error_message);
				return Error {};
			}

			// Iterate over functions with same name
			auto& functions = std::get<std::vector<ast::FunctionNode*>>(*binding);
			for (size_t i = 0; i < functions.size(); i++) {
				// Continue if number of arguments of the function isn't the same as the call
				if (functions[i]->args.size() != call.args.size()) continue;

				// Check if the function was already checked with current argument types
				auto args = ast::get_types(functions[i]->args);
				std::optional<ast::FunctionSpecialization> specialization = std::nullopt;
				for (size_t j = 0; j < functions[i]->specializations.size(); j++) {
					if (this->get_types(call.args) == functions[i]->specializations[j].args) {
						specialization = functions[i]->specializations[j];
					}
				}
				
				// If the function wasn't checked with the arguments
				if (!specialization.has_value()) {
					specialization = this->create_and_analyze_specialization(call, *functions[i]);
					functions[i]->specializations.push_back(specialization.value());
				}

				// If specialization valid
				if (specialization.value().valid) {
					if (specialization.value().return_type == ast::Type("")) {
						specialization.value().return_type = ast::Type("void");
					}

					if (call.type.is_type_variable()) {
						this->call_stack[this->call_stack.size() - 1].type_bindings[call.type.to_str()] = specialization.value().return_type;
					}
					else {
						call.type = specialization.value().return_type;
					}
					call.function = functions[i];
					return Ok {};
				}
			}

			this->errors.push_back(error_message);
			return Error {};
		}

		std::vector<std::unordered_map<std::string, Binding>> get_definitions() {
			std::vector<std::unordered_map<std::string, Binding>> scopes;
			for (size_t i = 0; i < this->scopes.size(); i++) {
				scopes.push_back(std::unordered_map<std::string, Binding>());
				for (auto it = this->scopes[i].begin(); it != this->scopes[i].end(); it++) {
					if (std::holds_alternative<std::vector<ast::FunctionNode*>>(it->second)) {
						scopes[i][it->first] = it->second;
					}
				}
			}
			return scopes;
		}
	};
};

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

	// Analyze statements
	for (size_t i = 0; i < node.statements.size(); i++) {
		// Analyze
		auto result = this->analyze(node.statements[i]);

		// Type checking
		switch (node.statements[i]->index()) {
			case ast::Assignment: break;
			case ast::Call: {
				if (result.is_ok() && this->get_type(node.statements[i]) != ast::Type("void")) {
					this->errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*node.statements[i]), this->current_module)); // tested in test/errors/unhandled_return_value.dm
				}
				break;
			}
			case ast::Return: {
				if (result.is_ok()) {
					auto return_type = std::get<ast::ReturnNode>(*node.statements[i]).expression.has_value() ? this->get_type(std::get<ast::ReturnNode>(*node.statements[i]).expression.value()) : ast::Type("void");
					if (node.type == ast::Type("")) node.type = return_type;
					else if (node.type != return_type) this->errors.push_back(Error("Error: Incompatible return type"));
				}
				break;
			}
			case ast::IfElse: {
				if (result.is_ok()) {
					auto if_type = this->get_type(std::get<ast::IfElseNode>(*node.statements[i]).if_branch);
					if (if_type != ast::Type("")) {
						if (node.type == ast::Type("")) node.type = if_type;
						else if (node.type != if_type) this->errors.push_back(Error("Error: Incompatible return type"));
					}

					if (std::get<ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
						auto else_type = this->get_type(std::get<ast::IfElseNode>(*node.statements[i]).else_branch.value());
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

Result<Ok, Error> semantic::Context::analyze(ast::AssignmentNode& node) {
	auto& identifier = std::get<ast::IdentifierNode>(*node.identifier).value;

	// Analyze expression
	auto result = this->analyze(node.expression);
	if (result.is_error()) return result;

	// nonlocal assignment
	if (node.nonlocal) {
		Binding* binding = this->get_binding(identifier);
		if (!binding) {
			this->errors.push_back(errors::undefined_variable(std::get<ast::IdentifierNode>(*node.identifier), this->current_module));
			return Error {};
		}
		if (semantic::get_binding_type(*binding) != this->get_type(node.expression)) {
			this->errors.push_back(std::string("Error: Incompatible type for variable"));
			return Error {};
		}
		*binding = &node;
	}

	// normal assignment
	else {
		if (this->current_scope().find(identifier) != this->current_scope().end()
		&& std::holds_alternative<ast::AssignmentNode*>(this->current_scope()[identifier])) {
			auto assignment = std::get<ast::AssignmentNode*>(this->current_scope()[identifier]);
			if (!assignment->is_mutable) {
				this->errors.push_back(errors::reassigning_immutable_variable(std::get<ast::IdentifierNode>(*node.identifier), node, this->current_module));
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
	if (this->get_type(node.condition) != ast::Type("bool")) {
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
	if (this->get_type(node.condition) != ast::Type("bool")) {
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

	int recursive = this->is_recursive_call(node);
	if (recursive != -1) {
		node.type = this->call_stack[recursive].function_node->return_type;
		node.function = this->call_stack[recursive].function_node;
		return Ok {};
	}

	auto result = this->get_type_of_intrinsic(node);
	if (result.is_ok()) return Ok {};

	result = this->get_type_of_user_defined_function(node);
	if (result.is_ok()) return Ok {};

	this->errors.push_back(errors::undefined_function(node, this->current_module)); // tested in test/errors/undefined_function.dmd
	return Error {};
}

Result<Ok, Error> semantic::Context::analyze(ast::FloatNode& node) {
	auto& type_bindings = this->call_stack[this->call_stack.size() - 1].type_bindings;
	if (node.type == ast::Type("") || type_bindings.find(node.type.to_str()) == type_bindings.end()) {
		node.type = ast::Type("float64");
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IntegerNode& node) {
	auto& type_bindings = this->call_stack[this->call_stack.size() - 1].type_bindings;
	if (node.type == ast::Type("") || type_bindings.find(node.type.to_str()) == type_bindings.end()) {
		node.type = ast::Type("int64");
	}
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
