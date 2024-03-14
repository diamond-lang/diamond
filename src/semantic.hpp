#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "ast.hpp"
#include "shared.hpp"

namespace semantic {
    Result<Ok, Errors> analyze(ast::Ast& ast);
    bool are_types_compatible(ast::FunctionNode& function, ast::Type function_type, ast::Type argument_type);
    bool are_types_compatible(ast::FunctionNode& function, std::vector<ast::Type> function_types, std::vector<ast::Type> argument_types);
}

#endif
