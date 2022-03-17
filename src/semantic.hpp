#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "ast.hpp"
#include "shared.hpp"

namespace semantic {
	Result<Ok, Errors> analyze(Ast::Ast& ast);
}

#endif
