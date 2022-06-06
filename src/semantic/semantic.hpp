#ifndef SHARED_SEMANTIC_HPP
#define SHARED_SEMANTIC_HPP

#include <unordered_map>
#include <cassert>
#include <filesystem>
#include <set>
#include <iostream>
#include <cassert>

#include "../ast.hpp"
#include "../errors.hpp"
#include "../semantic.hpp"
#include "../utilities.hpp"
#include "../lexer.hpp"
#include "../parser.hpp"
#include "intrinsics.hpp"
#include "type_inference.hpp"

namespace semantic {
	// Bindings
	enum BindingType {
		AssignmentBinding,
		FunctionArgumentBinding,
		OverloadedFunctionsBinding,
		GenericFunctionBinding
	};

	struct Binding {
		BindingType type;
		std::vector<ast::Node*> value;

		Binding() {}
        Binding(ast::AssignmentNode* assignment);
        Binding(ast::Node* function_argument);
        Binding(ast::FunctionNode* function); 
		Binding(std::vector<ast::FunctionNode*> functions);
	};

	ast::AssignmentNode* get_assignment(Binding binding);
	ast::Node* get_function_argument(Binding binding);
	ast::FunctionNode* get_generic_function(Binding binding);
	std::vector<ast::FunctionNode*> get_overloaded_functions(Binding binding);
	ast::Type get_binding_type(Binding& binding);
	bool is_function(Binding& binding);

	// Context
	struct Context {
		std::filesystem::path current_module;
		std::vector<std::unordered_map<std::string, Binding>> scopes;
		bool inside_loop = false;
		Errors errors;
		ast::Ast& ast;

		Context(ast::Ast& ast) : ast(ast) {}

		Result<Ok, Error> analyze(ast::Node* node);
		Result<Ok, Error> analyze(ast::BlockNode& node);
		Result<Ok, Error> analyze(ast::FunctionNode& node);
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

		void add_scope();
		void remove_scope();
		std::unordered_map<std::string, Binding>& current_scope();
		Binding* get_binding(std::string identifier);
		void add_functions_to_current_scope(ast::BlockNode& block);
		Result<Ok, Error> get_type_of_intrinsic(ast::CallNode& call);
		Result<Ok, Error> get_type_of_user_defined_function(ast::CallNode& call);
	};
}

#endif