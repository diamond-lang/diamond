#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

namespace parse {
	Result<std::shared_ptr<Ast::Program>, std::vector<Error>> program(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> function(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> assignment(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> call(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> expression(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> binary(Source source, int precedence = 1);
	ParserResult<std::shared_ptr<Ast::Expression>> unary(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> primary(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> grouping(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> number(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> integer(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> boolean(Source source);
	ParserResult<std::shared_ptr<Ast::Expression>> identifier(Source source);
	ParserResult<std::shared_ptr<Ast::Node>> op(Source source);
	ParserResult<std::string> token(Source source, std::string regex);
	ParserResult<std::string> whitespace(Source source);
	ParserResult<std::string> comment(Source source);
	ParserResult<std::string> regex(Source source, std::string regex);
};

#endif
