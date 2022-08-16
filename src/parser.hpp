#ifndef PARSER_HPP
#define PARSER_HPP

#include "errors.hpp"
#include "shared.hpp"
#include "tokens.hpp"
#include "ast.hpp"

namespace parse {
    Result<ast::Ast, Errors> program(const std::vector<token::Token>& tokens, const std::filesystem::path& file);
    Result<Ok, Errors> module(ast::Ast& ast, const std::vector<token::Token>& tokens, const std::filesystem::path& file);
};

#endif
