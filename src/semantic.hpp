#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "types.hpp"

namespace semantic {
	Result<Ok, Errors> analyze(std::shared_ptr<Ast::Program> program);
}

#endif
