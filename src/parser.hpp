#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

void parse(SourceFile source);
// ParserResult<Ast::Node> parse_expression(SourceFile source);
// ParserResult<Ast::Node> parse_binary(SourceFile source, int precedence = 0);
// ParserResult<Ast::Node> parse_unary(SourceFile source);
ParserResult<std::string> parse_binary_operator(SourceFile source);
ParserResult<std::string> parse_add(SourceFile source);
ParserResult<std::string> parse_minus(SourceFile source);
ParserResult<std::string> parse_mul(SourceFile source);
ParserResult<std::string> parse_div(SourceFile source);
ParserResult<std::string> parse_string(SourceFile source, std::string str);
ParserResult<std::string> parse_regex(SourceFile source, std::string regex);

#endif
