#ifndef TYPE_INFERENCE_HPP
#define TYPE_INFERENCE_HPP

#include "../ast.hpp"

namespace type_inference {
    void analyze(ast::FunctionNode* function);
}

#endif