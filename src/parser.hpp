#ifndef PARSER_HPP
#define PARSER_HPP

#include "errors.hpp"
#include "shared.hpp"
#include "tokens.hpp"
#include "ast.hpp"

namespace parse {
	struct Source {
		std::vector<token::Token> tokens;
		size_t position = 0;
		size_t line = 1;
		size_t column = 1;
		size_t indentation_level = 0;
		std::filesystem::path file;

		Source() {}
		Source(std::vector<token::Token> tokens, std::filesystem::path file) : tokens(tokens), file(file)  {}
    };

	::Result<std::shared_ptr<Ast::Program>, Errors> program(std::vector<token::Token> tokens, std::filesystem::path file);
};

#endif
