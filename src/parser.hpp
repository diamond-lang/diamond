#ifndef PARSER_HPP
#define PARSER_HPP

#include "errors.hpp"
#include "shared.hpp"
#include "tokens.hpp"
#include "ast.hpp"

namespace parse {
	Result<Ast::Ast, Errors> program(std::vector<token::Token>& tokens, std::filesystem::path& file);
};

#endif
