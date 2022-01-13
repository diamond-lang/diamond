#ifndef TYPE_INFERENCE_HPP
#define TYPE_INFERENCE_HPP

#include "types.hpp"

namespace type_inference {
    void analyze(std::shared_ptr<Ast::Function> function);
}

#endif