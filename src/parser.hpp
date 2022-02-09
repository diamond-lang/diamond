#ifndef PARSER_HPP
#define PARSER_HPP

#include "shared.hpp"
#include "tokens.hpp"
#include "ast.hpp"

namespace parse {
	::Result<std::shared_ptr<Ast::Program>, Errors> program(std::vector<token::Token> tokens);
};

#endif
