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
		Ast::AssignmentNode*,
		Ast::Node*,
		std::vector<Ast::FunctionNode*>
	>;

	Ast::Type get_binding_type(Binding& binding) {
		if (std::holds_alternative<Ast::AssignmentNode*>(binding)) return Ast::get_type(std::get<Ast::AssignmentNode*>(binding)->expression);
		else if (std::holds_alternative<Ast::Node*>(binding)) return Ast::get_type(std::get<Ast::Node*>(binding));
		else if (std::holds_alternative<std::vector<Ast::FunctionNode*>>(binding)) return Ast::Type("function");
		else assert(false);
		return Ast::Type("");
	}

	bool is_function(Binding& binding) {
		if (std::holds_alternative<std::vector<Ast::FunctionNode*>>(binding)) return true;
		else return false;
	}

	struct FunctionCall {
		std::unordered_map<std::string, Ast::Type> type_bindings;
		Ast::FunctionNode* function_node;
	};

	struct Context {
		std::filesystem::path current_module;
		std::vector<std::unordered_map<std::string, Binding>> scopes;
		std::vector<FunctionCall> call_stack;
		bool inside_loop = false;
		Errors errors;
		Ast::Ast& ast;

		Context(Ast::Ast& ast) : ast(ast) {}

		Result<Ok, Error> analyze(Ast::Node* node);
		Result<Ok, Error> analyze(Ast::BlockNode& node);
		Result<Ok, Error> analyze(Ast::FunctionNode& node) {return Ok {};}
		Result<Ok, Error> analyze(Ast::AssignmentNode& node);
		Result<Ok, Error> analyze(Ast::ReturnNode& node);
		Result<Ok, Error> analyze(Ast::BreakNode& node) {return Ok {};}
		Result<Ok, Error> analyze(Ast::ContinueNode& node) {return Ok {};}
		Result<Ok, Error> analyze(Ast::IfElseNode& node);
		Result<Ok, Error> analyze(Ast::WhileNode& node);
		Result<Ok, Error> analyze(Ast::UseNode& node) {return Ok {};}
		Result<Ok, Error> analyze(Ast::IncludeNode& node) {return Ok {};}
		Result<Ok, Error> analyze(Ast::CallNode& node);
		Result<Ok, Error> analyze(Ast::FloatNode& node);
		Result<Ok, Error> analyze(Ast::IntegerNode& node);
		Result<Ok, Error> analyze(Ast::IdentifierNode& node);
		Result<Ok, Error> analyze(Ast::BooleanNode& node);
		Result<Ok, Error> analyze(Ast::StringNode& node) {return Ok {};}

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

		void add_functions_to_current_scope(Ast::BlockNode& block) {
			// Add functions from current block to current context
			for (size_t i = 0; i < block.functions.size(); i++) {
				auto& function = std::get<Ast::FunctionNode>(*block.functions[i]);
				auto& identifier = std::get<Ast::IdentifierNode>(*function.identifier).value;

				auto& scope = this->current_scope();
				if (scope.find(identifier) == scope.end()) {
					auto binding = std::vector<Ast::FunctionNode*>{(Ast::FunctionNode*) block.functions[i]};
					scope[identifier] = binding;
				}
				else if (std::holds_alternative<std::vector<Ast::FunctionNode*>>(scope[identifier])) {
					std::get<std::vector<Ast::FunctionNode*>>(scope[identifier]).push_back((Ast::FunctionNode*) block.functions[i]);
				}
			}

			auto current_directory = std::filesystem::canonical(std::filesystem::current_path() / this->current_module).parent_path();

			// Parse used modules yet not parsed
			for (size_t i = 0; i < block.use_include_statements.size(); i++) {
				std::string path;
				if (std::holds_alternative<Ast::UseNode>(*block.use_include_statements[i])) {
					path = std::get<Ast::StringNode>(*std::get<Ast::UseNode>(*block.use_include_statements[i]).path).value;
				}
				else {
					path = std::get<Ast::StringNode>(*std::get<Ast::IncludeNode>(*block.use_include_statements[i]).path).value;
				}
				
				auto module_path = current_directory / (path + ".dmd");
				assert(std::filesystem::exists(module_path));
				module_path = std::filesystem::canonical(module_path);
				this->add_module_functions(module_path);
			}
		}

		int is_recursive_call(Ast::CallNode& call) {
			auto& identifier = std::get<Ast::IdentifierNode>(*call.identifier).value;

			for (int i = 0; i < this->call_stack.size(); i++) {
				if (identifier == std::get<Ast::IdentifierNode>(*this->call_stack[i].function_node->identifier).value
				&& this->get_types(call.args) == this->get_types(this->call_stack[i].function_node->args)) {
					return i;
				}
			}
			return -1;
		}

		Result<Ok, Error> get_type_of_intrinsic(Ast::CallNode& call) {
			auto& identifier = std::get<Ast::IdentifierNode>(*call.identifier).value;
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

		Ast::Type find_concrete_type_for_type_variable(Ast::Node* node, std::string type_variable) {
			switch (node->index()) {
				case Ast::Block: {
					auto& block = std::get<Ast::BlockNode>(*node);
					for (size_t i = 0; i < block.statements.size(); i++) {
						auto stmt_type = this->find_concrete_type_for_type_variable(block.statements[i], type_variable);
						if (stmt_type != Ast::Type("")) return stmt_type;
					}
					break;
				}
				case Ast::Assignment: {
					return this->find_concrete_type_for_type_variable(std::get<Ast::AssignmentNode>(*node).expression, type_variable);
				}
				case Ast::Return: {
					auto& return_node = std::get<Ast::ReturnNode>(*node);
					if (return_node.expression.has_value()) return this->find_concrete_type_for_type_variable(return_node.expression.value(), type_variable);
					break;
				}
				case Ast::While: {
					return this->find_concrete_type_for_type_variable(std::get<Ast::WhileNode>(*node).block, type_variable);
				}
				case Ast::Break:
				case Ast::Continue: {
					break;
				}
				case Ast::IfElse: {
					auto& if_else = std::get<Ast::IfElseNode>(*node);
					auto if_branch = this->find_concrete_type_for_type_variable(if_else.if_branch, type_variable);
					if (if_branch != Ast::Type("")) return if_branch;

					if (if_else.else_branch.has_value()) {
						auto else_branch = this->find_concrete_type_for_type_variable(if_else.else_branch.value(), type_variable);
						if (else_branch != Ast::Type("")) return else_branch;
					}
					break;
				}
				case Ast::Call: {
					auto& call = std::get<Ast::CallNode>(*node);
					if (call.type == type_variable) {
						for (size_t i = 0; i < call.args.size(); i++) {
							if (Ast::get_type(call.args[i]).to_str() == type_variable) {
								return this->find_concrete_type_for_type_variable(call.args[i], type_variable);
							}
						}
					}
					break;
				}
				case Ast::Float: {
					if (Ast::get_type(node) == type_variable) return Ast::Type("float64");
					break;
				}
				case Ast::Integer: {
					if (Ast::get_type(node) == type_variable) return Ast::Type("int64");
					break;
				}
				case Ast::Identifier: {
					break;
				}
				default: assert(false);
			}
			return Ast::Type("");
		}

		Ast::Type get_type(Ast::Node* node) {
			if (Ast::get_type(node).is_type_variable()) {
				assert(this->call_stack.size() > 0);
				return this->call_stack[this->call_stack.size() - 1].type_bindings[Ast::get_type(node).to_str()];
			}
			else {
				return Ast::get_type(node);
			}
		}

		Ast::Type get_type(Ast::Type type) {
			if (type.is_type_variable()) {
				assert(this->call_stack.size() > 0);
				return this->call_stack[this->call_stack.size() - 1].type_bindings[type.to_str()];
			}
			else {
				return type;
			}
		}

		std::vector<Ast::Type> get_types(std::vector<Ast::Node*> nodes) {
			std::vector<Ast::Type> types;
			for (size_t i = 0; i < nodes.size(); i++) {
				types.push_back(this->get_type(nodes[i]));
			}
			return types;
		}

		Ast::FunctionSpecialization create_and_analyze_specialization(Ast::CallNode& call, Ast::FunctionNode& function) {
			assert(function.generic);

			// Typer infer function is not type inferred yet
			if (function.return_type == Ast::Type("")) {
				type_inference::analyze(&function);
			}

			// Add new specialization
			Ast::FunctionSpecialization specialization;
			specialization.args = Ast::get_types(function.args);
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
					Ast::Type concrete_type = this->find_concrete_type_for_type_variable(function.body, specialization.return_type.to_str());
					function_call.type_bindings[specialization.return_type.to_str()] = concrete_type;
					specialization.return_type = concrete_type;
				}
			}

			// Add function to call stack to be able to detect recursion
			context.call_stack.push_back(function_call);

			// Add arguments to new scope
			for (size_t i = 0; i != function.args.size(); i++) {
				auto identifier = std::get<Ast::IdentifierNode>(*function.args[i]).value;
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

		Result<Ok, Error> get_type_of_user_defined_function(Ast::CallNode& call) {
			auto& identifier = std::get<Ast::IdentifierNode>(*call.identifier).value;

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
			auto& functions = std::get<std::vector<Ast::FunctionNode*>>(*binding);
			for (size_t i = 0; i < functions.size(); i++) {
				// Continue if number of arguments of the function isn't the same as the call
				if (functions[i]->args.size() != call.args.size()) continue;

				// Check if the function was already checked with current argument types
				auto args = Ast::get_types(functions[i]->args);
				std::optional<Ast::FunctionSpecialization> specialization = std::nullopt;
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
					if (std::holds_alternative<std::vector<Ast::FunctionNode*>>(it->second)) {
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
Result<Ok, Errors> semantic::analyze(Ast::Ast& ast) {
	semantic::Context context(ast);
	context.current_module = ast.file;
	context.analyze(ast.program);

	if (context.errors.size() > 0) return context.errors;
	else                           return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::Node* node) {
	return std::visit([this](auto& variant) {return this->analyze(variant);}, *node);
}

Result<Ok, Error> semantic::Context::analyze(Ast::BlockNode& node) {
	node.type = Ast::Type("");
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
			case Ast::Assignment: break;
			case Ast::Call: {
				if (result.is_ok() && this->get_type(node.statements[i]) != Ast::Type("void")) {
					this->errors.push_back(errors::unhandled_return_value(std::get<Ast::CallNode>(*node.statements[i]), this->current_module)); // tested in test/errors/unhandled_return_value.dm
				}
				break;
			}
			case Ast::Return: {
				if (result.is_ok()) {
					auto return_type = std::get<Ast::ReturnNode>(*node.statements[i]).expression.has_value() ? this->get_type(std::get<Ast::ReturnNode>(*node.statements[i]).expression.value()) : Ast::Type("void");
					if (node.type == Ast::Type("")) node.type = return_type;
					else if (node.type != return_type) this->errors.push_back(Error("Error: incompatible return types"));
				}
				break;
			}
			case Ast::IfElse: {
				if (result.is_ok()) {
					auto if_type = this->get_type(std::get<Ast::IfElseNode>(*node.statements[i]).if_branch);
					if (if_type != Ast::Type("")) {
						if (node.type == Ast::Type("")) node.type = if_type;
						else if (node.type != if_type) this->errors.push_back(Error("Error: Incamptible return type"));
					}

					if (std::get<Ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
						auto else_type = this->get_type(std::get<Ast::IfElseNode>(*node.statements[i]).else_branch.value());
						if (else_type != Ast::Type("")) {
							if (node.type == Ast::Type("")) node.type = else_type;
							else if (node.type != else_type) this->errors.push_back(Error("Error: Incamptible return type"));
						}
					}
				}
				break;
			}
			case Ast::While: break;
			case Ast::Break: break;
			case Ast::Continue: break;
			default: assert(false);
		}
	}

	// Remove scope
	this->remove_scope();

	if (this->errors.size() > number_of_errors) return Error {};
	else                                        return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::AssignmentNode& node) {
	auto& identifier = std::get<Ast::IdentifierNode>(*node.identifier).value;

	// Analyze expression
	auto result = this->analyze(node.expression);
	if (result.is_error()) return result;

	// nonlocal assignment
	if (node.nonlocal) {
		Binding* binding = this->get_binding(identifier);
		if (!binding) {
			this->errors.push_back(errors::undefined_variable(std::get<Ast::IdentifierNode>(*node.identifier), this->current_module));
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
		&& std::holds_alternative<Ast::AssignmentNode*>(this->current_scope()[identifier])) {
			auto assignment = std::get<Ast::AssignmentNode*>(this->current_scope()[identifier]);
			if (!assignment->is_mutable) {
				this->errors.push_back(errors::reassigning_immutable_variable(std::get<Ast::IdentifierNode>(*node.identifier), node, this->current_module));
				return Error {};
			}
		}
		this->current_scope()[identifier] = &node;
	}

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::ReturnNode& node) {
	if (node.expression.has_value()) {
		auto result = this->analyze(node.expression.value());
		if (result.is_error()) return result;
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::IfElseNode& node) {
	auto condition = this->analyze(node.condition);
	if (condition.is_error()) return condition;
	if (this->get_type(node.condition) != Ast::Type("bool")) {
		this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
	}

	auto if_branch = this->analyze(node.if_branch);
	if (if_branch.is_error()) return if_branch;

	if (!node.else_branch.has_value()) return Ok {};

	auto else_branch = this->analyze(node.else_branch.value());
	if (else_branch.is_error()) return else_branch;

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::WhileNode& node) {
	auto condition = this->analyze(node.condition);
	if (condition.is_error()) return condition;
	if (this->get_type(node.condition) != Ast::Type("bool")) {
		this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
	}

	auto block = this->analyze(node.block);
	if (block.is_error()) return block;

	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::CallNode& node) {
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

Result<Ok, Error> semantic::Context::analyze(Ast::FloatNode& node) {
	if (node.type == Ast::Type("")) {
		node.type = Ast::Type("float64");
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::IntegerNode& node) {
	if (node.type == Ast::Type("")) {
		node.type = Ast::Type("int64");
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::IdentifierNode& node) {
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

Result<Ok, Error> semantic::Context::analyze(Ast::BooleanNode& node) {
	node.type = Ast::Type("bool");
	return Ok {};
}
