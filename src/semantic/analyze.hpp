#ifndef SEMANTIC_ANALYZE_HPP
#define SEMANTIC_ANALYZE_HPP

#include "context.hpp"

namespace semantic {
    Result<Ok, Error> analyze_block_or_expression(Context& context, ast::Node* node);
    Result<Ok, Error> analyze(Context& context, ast::BlockNode& node);
    Result<Ok, Error> analyze(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> analyze(Context& context, ast::Type& type);
    Result<Ok, Error> analyze(Context& context, ast::TypeNode& node);
}

#endif