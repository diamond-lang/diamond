#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

namespace parse {
	void program(SourceFile source);
	// ParserResult<Ast::Node> expression(SourceFile source);
	// ParserResult<Ast::Node> binary(SourceFile source, int precedence = 0);
	// ParserResult<Ast::Node> unary(SourceFile source);
	ParserResult<std::string> binary_operator(SourceFile source);
	ParserResult<std::string> add(SourceFile source);
	ParserResult<std::string> minus(SourceFile source);
	ParserResult<std::string> mul(SourceFile source);
	ParserResult<std::string> div(SourceFile source);
	ParserResult<std::string> string(SourceFile source, std::string str);
	ParserResult<std::string> regex(SourceFile source, std::string regex);
};

#endif
