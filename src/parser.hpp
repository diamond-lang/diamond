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
		size_t line;
		size_t column;
		std::vector<size_t> indentation_level;
		std::filesystem::path file;

		Source() {}
		Source(std::vector<token::Token> tokens, std::filesystem::path file) : tokens(tokens), file(file)  {
			if (tokens.size() > 0) {
				this->line = tokens[0].line;
				this->column = tokens[0].column;
			}
		}
    };

	::Result<std::shared_ptr<Ast::Program>, Errors> program(std::vector<token::Token> tokens, std::filesystem::path file);
};

#endif
