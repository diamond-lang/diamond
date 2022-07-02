#ifndef TYPE_INFERENCE_HPP
#define TYPE_INFERENCE_HPP

#include "../ast.hpp"
#include "../shared.hpp"

namespace type_inference {
    Result<Ok, Error> analyze(semantic::Context& semantic_context, ast::FunctionNode* function);
}

#endif