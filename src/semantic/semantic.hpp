#ifndef SEMANTIC_ANALYZE_HPP
#define SEMANTIC_ANALYZE_HPP

#include "context.hpp"

namespace semantic {
    Result<Ok, Error> analyze_block_or_expression(Context& context, ast::Node* node);
    Result<Ok, Error> analyze(Context& context, ast::BlockNode& node);
    Result<Ok, Error> analyze(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> analyze(Context& context, ast::Type& type);
    Result<Ok, Error> analyze(Context& context, ast::TypeNode& node);
    Result<ast::Type, Error> get_function_type(Context& context, ast::Node* function_or_interface, std::vector<bool> call_args_mutability, std::vector<ast::Type> call_args, ast::Type call_type);
}

#endif