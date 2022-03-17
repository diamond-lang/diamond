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

	Ast::Type get_type(Binding& binding) {
		if (std::holds_alternative<Ast::AssignmentNode*>(binding)) return std::get<Ast::AssignmentNode*>(binding)->type;
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
		std::string identifier;
		std::vector<Ast::Type> args;
		Ast::Type return_type = Ast::Type("");
		Ast::FunctionNode* function = nullptr; 
	};

	struct Context {
		std::filesystem::path current_module;
		std::vector<std::unordered_map<std::string, Binding>> scopes;
		std::unordered_map<std::string, Ast::Type> type_bindings;
		std::vector<FunctionCall> call_stack;
		bool inside_loop = false;
		Errors errors;
		Ast::Ast& ast;

		Result<Ok, Error> analyze(Ast::Node* node);
		Result<Ok, Error> analyze(Ast::BlockNode& node);
		Result<Ok, Error> analyze(Ast::FunctionNode& node) {}
		Result<Ok, Error> analyze(Ast::AssignmentNode& node);
		Result<Ok, Error> analyze(Ast::ReturnNode& node);
		Result<Ok, Error> analyze(Ast::BreakNode& node) {}
		Result<Ok, Error> analyze(Ast::ContinueNode& node) {}
		Result<Ok, Error> analyze(Ast::IfElseNode& node);
		Result<Ok, Error> analyze(Ast::WhileNode& node);
		Result<Ok, Error> analyze(Ast::UseNode& node) {}
		Result<Ok, Error> analyze(Ast::IncludeNode& node) {}
		Result<Ok, Error> analyze(Ast::CallNode& node);
		Result<Ok, Error> analyze(Ast::FloatNode& node);
		Result<Ok, Error> analyze(Ast::IntegerNode& node);
		Result<Ok, Error> analyze(Ast::IdentifierNode& node);
		Result<Ok, Error> analyze(Ast::BooleanNode& node);
		Result<Ok, Error> analyze(Ast::StringNode& node) {}

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
				if (identifier == this->call_stack[i].identifier
				&& Ast::get_types(call.args) == this->call_stack[i].args) {
					return i;
				}
			}
			return -1;
		}

		Result<Ok, Error> get_type_of_intrinsic(Ast::CallNode& call) {
			auto& identifier = std::get<Ast::IdentifierNode>(*call.identifier).value;
			if (intrinsics.find(identifier) != intrinsics.end()) {
				auto args = Ast::get_types(call.args);
				auto prototypes = intrinsics[identifier];
				for (size_t i = 0; i < prototypes.size(); i++) {
					if (args == prototypes[i].first) {
						call.type = prototypes[i].second;
						return Ok {};
					}
				}
			}

			return Error {};
		}

		Result<Ok, Error> get_type_of_user_defined_function(Ast::CallNode& call) {
			auto& identifier = std::get<Ast::IdentifierNode>(*call.identifier).value;

			// Create error message
			std::string error_message = identifier + "(";
			for (size_t i = 0; i < call.args.size(); i++) {
				error_message += Ast::get_type(call.args[i]).to_str();
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
				assert(functions[i]->generic);

				// Check function same number of arguments as the call
				if (functions[i]->args.size() != call.args.size()) continue;

				// Type infer function
				if (functions[i]->return_type == Ast::Type("")) {
					asdasd
				}
			}
		}
	};
};

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(Ast::Ast& ast) {

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
				if (result.is_ok() && Ast::get_type(node.statements[i]) != Ast::Type("void")) {
					this->errors.push_back(errors::unhandled_return_value(std::get<Ast::CallNode>(*node.statements[i]), this->current_module)); // tested in test/errors/unhandled_return_value.dm
				}
				break;
			}
			case Ast::Return: {
				if (result.is_ok()) {
					auto return_type = std::get<Ast::ReturnNode>(*node.statements[i]).expression.has_value() ? Ast::get_type(std::get<Ast::ReturnNode>(*node.statements[i]).expression.value()) : Ast::Type("void");
					if (node.type == Ast::Type("")) node.type = return_type;
					else this->errors.push_back(Error("Error: incomaptible return types"));
				}
				break;
			}
			case Ast::IfElse: {
				if (result.is_ok()) {
					auto if_type = Ast::get_type(std::get<Ast::IfElseNode>(*node.statements[i]).if_branch);
					if (if_type != Ast::Type("")) {
						if (node.type == Ast::Type("")) node.type = if_type;
						else this->errors.push_back(Error("Error: Incamptible return type"));
					}

					if (std::get<Ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
						auto else_type = Ast::get_type(std::get<Ast::IfElseNode>(*node.statements[i]).else_branch.value());
						if (else_type != Ast::Type("")) {
							if (node.type == Ast::Type("")) node.type = else_type;
							else this->errors.push_back(Error("Error: Incamptible return type"));
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
		if (get_type(*binding) != Ast::get_type(node.expression)) {
			this->errors.push_back(std::string("Error: Incompatible type for variable"));
			return Error {};
		}
		*binding = (Ast::Node*) &node;
	}

	// normal assignment
	else {
		if (this->current_scope().find(identifier) != this->current_scope().end()
		|| std::holds_alternative<Ast::AssignmentNode*>(this->current_scope()[identifier])) {
			auto assignment = std::get<Ast::AssignmentNode*>(this->current_scope()[identifier]);
			if (!assignment->is_mutable) {
				this->errors.push_back(errors::reassigning_immutable_variable(std::get<Ast::IdentifierNode>(*node.identifier), node, this->current_module));
				return Error {};
			}
		}
		this->current_scope()[identifier] = (Ast::Node*) &node;
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
	if (Ast::get_type(node.condition) != Ast::Type("bool")) {
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
	if (Ast::get_type(node.condition) != Ast::Type("bool")) {
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
		node.type = this->call_stack[recursive].return_type;
		node.function = this->call_stack[recursive].function;
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
	if (node.type.is_type_variable() && this->type_bindings.find(node.type.to_str()) != this->type_bindings.end()) {
		node.type = this->type_bindings[node.type.to_str()];
	}
	else {
		node.type = Ast::Type("float64");
	}
	return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(Ast::IntegerNode& node) {
	if (node.type.is_type_variable() && this->type_bindings.find(node.type.to_str()) != this->type_bindings.end()) {
		node.type = this->type_bindings[node.type.to_str()];
	}
	else {
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
		node.type = semantic::get_type(*binding);
		return Ok {};
	}
}

Result<Ok, Error> semantic::Context::analyze(Ast::BooleanNode& node) {
	node.type = Ast::Type("bool");
	return Ok {};
}
