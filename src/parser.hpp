#ifndef PARSER_HPP
#define PARSER_HPP

#include "errors.hpp"
#include "shared.hpp"
#include "tokens.hpp"
#include "ast.hpp"

namespace parse {
	struct Source {
		size_t line;
		size_t column;
		std::filesystem::path file;

		Source(size_t line, size_t column, std::filesystem::path file) : line(line), column(column), file(file) {}
    };
};

struct Parser {
	std::vector<token::Token>& tokens;
	size_t position = 0;
	std::filesystem::path& file;
	Ast::Ast ast;
	std::vector<size_t> indentation_level;
	Errors errors;

	Parser(std::vector<token::Token>& tokens, std::filesystem::path& file) : tokens(tokens), file(file) {}

	Result<Ok, Error> parse_program();
	Result<size_t, Error> parse_block();
	Result<size_t, Error> parse_expression_block();
	Result<size_t, Error> parse_function();
	Result<size_t, Error> parse_statement();
	Result<size_t, Error> parse_assignment();
	Result<size_t, Error> parse_return_stmt();
	Result<size_t, Error> parse_break_stmt();
	Result<size_t, Error> parse_continue_stmt();
	Result<size_t, Error> parse_if_else();
	Result<size_t, Error> parse_while_stmt();
	Result<size_t, Error> parse_use_stmt();
	Result<size_t, Error> parse_include_stmt();
	Result<size_t, Error> parse_call();
	Result<size_t, Error> parse_expression();
	Result<size_t, Error> parse_not_expr();
	Result<size_t, Error> parse_binary(int precedence = 1);
	Result<size_t, Error> parse_negation();
	Result<size_t, Error> parse_primary();
	Result<size_t, Error> parse_grouping();
	Result<size_t, Error> parse_float();
	Result<size_t, Error> parse_integer();
	Result<size_t, Error> parse_boolean();
	Result<size_t, Error> parse_identifier();
	Result<size_t, Error> parse_identifier(token::TokenVariant token);
	Result<size_t, Error> parse_string();
	Result<token::Token, Error> parse_token(token::TokenVariant token);

	token::Token current();
	size_t current_indentation();
	void advance();
	void advance_until_next_statement();
	bool at_end();
	bool match(std::vector<token::TokenVariant> tokens);
	parse::Source source();
};

#endif
