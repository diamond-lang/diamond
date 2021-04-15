#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

namespace parse {
	Ast::Program program(Source source);
	ParserResult<Ast::Node*> expression(Source source);
	ParserResult<Ast::Node*> binary(Source source, int precedence = 1);
	ParserResult<Ast::Node*> unary(Source source);
	ParserResult<Ast::Node*> number(Source source);
	ParserResult<std::string> token(Source source, std::string regex);
	ParserResult<std::string> whitespace(Source source);
	ParserResult<std::string> regex(Source source, std::string regex);
};

#endif
