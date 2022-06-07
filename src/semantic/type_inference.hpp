#ifndef TYPE_INFERENCE_HPP
#define TYPE_INFERENCE_HPP

#include "../ast.hpp"
#include "semantic.hpp"

namespace type_inference {
    void analyze(semantic::Context& semantic_context, ast::FunctionNode* function);
}

#endif