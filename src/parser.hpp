#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast.hpp"
#include "shared.hpp"
#include "tokens.hpp"

namespace parse {
    std::variant<ast::Ast, Errors> program(
        const std::vector<token::Token>& tokens,
        const std::filesystem::path& file
    );
    std::variant<Ok, Errors> module(
        ast::Ast& ast,
        const std::vector<token::Token>& tokens,
        std::filesystem::path& file
    );
};

#endif
