#include <iostream>
#include <algorithm>
#include <map>
#include <regex>

#include "parser.hpp"

void parse(SourceFile source) {
	// std::vector<std::unique_ptr<Ast::Node>> expressions;
	//
	// while (!source.at_end()) {
	// 	auto result = parse_expression(source);
	// 	if (!result.error()) {
	// 		expressions.push_back(std::move(result.value));
	// 		source = result.source;
	// 	}

	parse_regex(source, "\\*");
	// }

	while (!at_end(source)) {
		if (current(source) == '\n') break;
		auto result = parse_binary_operator(source);
		if (!result.error()) {
			std::cout << *(result.value.get()) << '\n';
			source = result.source;
		}
		else break;
	}
}

// ParserResult<Ast::Node> parse_expression(SourceFile source) {
// 	return parse_binary(source);
// }

// ParserResult<Ast::Node> parse_binary(SourceFile source, int precedence) {
// 	static std::map<std::string, int> bin_op_precedence;
// 	bin_op_precedence["+"] = 1;
// 	bin_op_precedence["-"] = 1;
// 	bin_op_precedence["*"] = 2;
// 	bin_op_precedence["/"] = 2;
//
// 	if (precedence > std::get<1>(*std::max_element(bin_op_precedence.begin(), bin_op_precedence.end()))) {
// 		return parse_unary(source);
// 	}
// 	else {
// 		auto left = parse_binary(source, precedence + 1);
//
// 		if (left.error()) return left;
// 		source = left.source;
//
// 		while (true) {
// 			auto op = parse_binary_operator(source);
// 			if (bin_op_precedence[op.value] != precedence) break;
// 			source = operator.source;
// 		}
// 	}
//
// 	ParserResult<Ast::Node> node;
// 	return node;
// }

ParserResult<std::string> parse_binary_operator(SourceFile source) {
	if (!parse_add(source).error())   return parse_add(source);
	if (!parse_minus(source).error()) return parse_minus(source);
	if (!parse_mul(source).error())   return parse_mul(source);
	if (!parse_div(source).error())   return parse_div(source);
	else return ParserResult<std::string>(source, "Expecting binary operator");
}

ParserResult<std::string> parse_add(SourceFile source)   {return parse_string(source, "+");}
ParserResult<std::string> parse_minus(SourceFile source) {return parse_string(source, "-");}
ParserResult<std::string> parse_mul(SourceFile source)   {return parse_string(source, "*");}
ParserResult<std::string> parse_div(SourceFile source)   {return parse_string(source, "/");}

ParserResult<std::string> parse_string(SourceFile source, std::string str) {
	if (match(source, str)) return ParserResult<std::string>(std::make_unique<std::string>(str), source + str.size());
	else                    return ParserResult<std::string>(source, "Expecting \"" + str + "\"");
}

ParserResult<std::string> parse_regex(SourceFile source, std::string regex) {
	std::smatch sm;
	if (std::regex_search((std::string::const_iterator) source.it, (std::string::const_iterator) source.end, sm, std::regex(regex), std::regex_constants::match_continuous)) {
		return ParserResult<std::string>(std::make_unique<std::string>(sm[0]), source + sm[0].str().size());
	}
	else {
		return ParserResult<std::string>(source, "Expecting \"" + regex + "\"");
	}
}
