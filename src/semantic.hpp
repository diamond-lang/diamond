#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "ast.hpp"
#include "shared.hpp"
#include "semantic/scopes.hpp"

namespace semantic {
    Result<Ok, Errors> analyze(ast::Ast& ast);
    Result<Ok, Errors> analyze_module(ast::Ast& ast, std::filesystem::path module_path);
    bool are_types_compatible(ast::FunctionNode& function, semantic::FunctionsAndTypesScopes& function_and_types_scopes, ast::Type function_type, ast::Type argument_type);
    bool are_types_compatible(ast::FunctionNode& function, semantic::FunctionsAndTypesScopes& function_and_types_scopes, std::vector<ast::Type> function_types, std::vector<ast::Type> argument_types);
    bool are_arguments_compatible(ast::FunctionNode& function, semantic::FunctionsAndTypesScopes& function_and_types_scopes, std::vector<bool> call_args_mutability, std::vector<ast::Type> function_types, std::vector<ast::Type> argument_types);
}

#endif
