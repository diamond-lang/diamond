#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

namespace parse {
	Result<std::shared_ptr<Ast::Program>, Errors> program(Source source);
	ParserResult<std::shared_ptr<Ast::Block>> block(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> expression_block(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> function(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> statement(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> assignment(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> return_stmt(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> if_else_stmt(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> call(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> expression(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> if_else_expr(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> not_expr(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> binary(Source source, int precedence = 1);
	ParserResult<std::shared_ptr<Ast::Expression>> unary(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> primary(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> grouping(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> number(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> integer(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> boolean(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> identifier(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> identifier(Source source, std::string identifier);
	ParserResult<std::shared_ptr<Ast::Node>> op(Source source);
	ParserResult<std::string> token(Source source, std::string regex);
	ParserResult<std::string> whitespace(Source source);
	ParserResult<std::string> comment(Source source);
	ParserResult<std::string> regex(Source source, std::string regex);
};

#endif
