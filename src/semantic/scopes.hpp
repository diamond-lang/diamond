#ifndef SEMANTIC_FUNCTION_SCOPES_HPP
#define SEMANTIC_FUNCTION_SCOPES_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <set>

#include "../ast.hpp"
#include "../utilities.hpp"

namespace semantic {
    using FunctionsAndTypesScope = std::unordered_map<std::string, ast::Node*>;

    struct FunctionsAndTypesScopes {
        std::vector<FunctionsAndTypesScope> scopes;

        void add_scope();
        void remove_scope();
        FunctionsAndTypesScope& current_scope();
        ast::Node* get_binding(std::string identifier);

        Result<Ok, Error> add_definitions_to_current_scope(std::vector<ast::FunctionNode*>& functions, std::vector<ast::InterfaceNode*>& interfaces, std::vector<ast::TypeNode*>& types);
        Result<Ok, Errors> add_definitions_from_block_to_scope(ast::Ast& ast, std::filesystem::path module_path, ast::BlockNode& block);
        Result<Ok, Errors> add_module_functions(ast::Ast& ast, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules);
    };
}

#endif