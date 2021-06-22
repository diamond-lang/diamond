#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "types.hpp"

namespace semantic {
	Result<Ok, std::vector<Error>> analyze(std::shared_ptr<Ast::Program> program);
}

#endif
