#include <iostream>
#include <algorithm>
#include <map>
#include <regex>

#include "parser.hpp"

void parse::program(SourceFile source) {
	// std::vector<std::unique_ptr<Ast::Node>> expressions;
	//
	// while (!source.at_end()) {
	// 	auto result = parse::expression(source);
	// 	if (!result.error()) {
	// 		expressions.push_back(std::move(result.value));
	// 		source = result.source;
	// 	}

	parse::regex(source, "\\*");
	// }

	while (!at_end(source)) {
		if (current(source) == '\n') break;
		auto result = parse::binary_operator(source);
		if (!result.error()) {
			std::cout << *(result.value.get()) << '\n';
			source = result.source;
		}
		else break;
	}
}

// ParserResult<Ast::Node> parse::expression(SourceFile source) {
// 	return parse::binary(source);
// }

// ParserResult<Ast::Node> parse::binary(SourceFile source, int precedence) {
// 	static std::map<std::string, int> bin_op_precedence;
// 	bin_op_precedence["+"] = 1;
// 	bin_op_precedence["-"] = 1;
// 	bin_op_precedence["*"] = 2;
// 	bin_op_precedence["/"] = 2;
//
// 	if (precedence > std::get<1>(*std::max_element(bin_op_precedence.begin(), bin_op_precedence.end()))) {
// 		return parse::unary(source);
// 	}
// 	else {
// 		auto left = parse::binary(source, precedence + 1);
//
// 		if (left.error()) return left;
// 		source = left.source;
//
// 		while (true) {
// 			auto op = parse::binary_operator(source);
// 			if (bin_op_precedence[op.value] != precedence) break;
// 			source = operator.source;
// 		}
// 	}
//
// 	ParserResult<Ast::Node> node;
// 	return node;
// }

ParserResult<std::string> parse::binary_operator(SourceFile source) {
	if (!parse::add(source).error())   return parse::add(source);
	if (!parse::minus(source).error()) return parse::minus(source);
	if (!parse::mul(source).error())   return parse::mul(source);
	if (!parse::div(source).error())   return parse::div(source);
	else return ParserResult<std::string>(source, "Expecting binary operator");
}

ParserResult<std::string> parse::add(SourceFile source)   {return parse::string(source, "+");}
ParserResult<std::string> parse::minus(SourceFile source) {return parse::string(source, "-");}
ParserResult<std::string> parse::mul(SourceFile source)   {return parse::string(source, "*");}
ParserResult<std::string> parse::div(SourceFile source)   {return parse::string(source, "/");}

ParserResult<std::string> parse::string(SourceFile source, std::string str) {
	if (match(source, str)) return ParserResult<std::string>(std::make_unique<std::string>(str), source + str.size());
	else                    return ParserResult<std::string>(source, "Expecting \"" + str + "\"");
}

ParserResult<std::string> parse::regex(SourceFile source, std::string regex) {
	std::smatch sm;
	if (std::regex_search((std::string::const_iterator) source.it, (std::string::const_iterator) source.end, sm, std::regex(regex), std::regex_constants::match_continuous)) {
		return ParserResult<std::string>(std::make_unique<std::string>(sm[0]), source + sm[0].str().size());
	}
	else {
		return ParserResult<std::string>(source, "Expecting \"" + regex + "\"");
	}
}
