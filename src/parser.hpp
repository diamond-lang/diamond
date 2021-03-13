#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

namespace parse {
	Ast::Program program(Source source);
	ParserResult<Ast::Node*> expression(Source source);
	ParserResult<std::string> number(Source source);
	ParserResult<std::string> string(Source source, std::string str);
	ParserResult<std::string> regex(Source source, std::string regex);
};

#endif
