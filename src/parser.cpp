#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"

Ast::Program parse::program(Source source) {
	std::vector<Ast::Node*> expressions;

	while (!at_end(source)) {
		// Parse comment
		if (!parse::comment(source).error()) {
			source = parse::comment(source).source;
		}
		else {
			// Parse expression
			auto result = parse::expression(source);
			if (!result.error()) {
				expressions.push_back(result.value);
				source = result.source;
			}
			else {
				std::cout << result.error_message << '\n';
			}
		}

		// Advance until new line
		while (current(source) != '\n') source = source + 1;
		while (current(source) == '\n') source = source + 1; // Eat new lines
	}

	return Ast::Program(expressions, 1, 1, source.file);
}

ParserResult<Ast::Node*> parse::expression(Source source) {
	return parse::binary(source);
}

ParserResult<Ast::Node*> parse::binary(Source source, int precedence) {
	static std::map<std::string, int> bin_op_precedence;
	bin_op_precedence["+"] = 1;
	bin_op_precedence["-"] = 1;
	bin_op_precedence["*"] = 2;
	bin_op_precedence["/"] = 2;
	bin_op_precedence["<"] = 3;
	bin_op_precedence["<="] = 3;
	bin_op_precedence[">"] = 3;
	bin_op_precedence[">="] = 3;

	if (precedence > std::get<1>(*std::max_element(bin_op_precedence.begin(), bin_op_precedence.end()))) {
		return parse::unary(source);
	}
	else {
		auto left = parse::binary(source, precedence + 1);

		if (left.error()) return left;
		source = left.source;

		while (true) {
			auto op = parse::token(source, "(\\+|-|\\*|\\/|<=|<|>=|>)");
			if (op.error() || bin_op_precedence[op.value] != precedence) break;
			source = op.source;

			auto right = parse::binary(source, precedence + 1);
			if (right.error()) return right;
			source = right.source;

			std::string identifier = op.value;
			std::vector<Ast::Node*> args;
			args.push_back(left.value);
			args.push_back(right.value);
			left.value = new Ast::Call(identifier, args, source.line, source.col, source.file);
		}

		return ParserResult<Ast::Node*>(left.value, source);
	}
}

ParserResult<Ast::Node*> parse::unary(Source source) {
	return parse::number(source);
}

ParserResult<Ast::Node*> parse::number(Source source) {
	auto result = parse::token(source, "([0-9]*[.])?[0-9]+");
	if (result.error()) return ParserResult<Ast::Node*>(source, errors::expecting_number(result.source));
	double value = atof(result.value.c_str());
	Ast::Number* node = new Ast::Number(value, source.line, source.col, source.file);
	return ParserResult<Ast::Node*>(node, result.source);
}

ParserResult<std::string> parse::token(Source source, std::string regex) {
	while (!parse::whitespace(source).error() || !parse::comment(source).error()) {
		if (!parse::whitespace(source).error()) {
			source = parse::whitespace(source).source;
		}
		else if (!parse::comment(source).error()) {
			source = parse::comment(source).source;
		}
	}
	return parse::regex(source, regex);
}

ParserResult<std::string> parse::whitespace(Source source) {
	return parse::regex(source, "[ \\r\\t]+");
}

ParserResult<std::string> parse::comment(Source source) {
	return parse::regex(source, "(--).*");
}

ParserResult<std::string> parse::regex(Source source, std::string regex) {
	std::smatch sm;
	if (std::regex_search((std::string::const_iterator) source.it, (std::string::const_iterator) source.end, sm, std::regex(regex), std::regex_constants::match_continuous)) {
		return ParserResult<std::string>(sm[0].str(), source + sm[0].str().size());
	}
	else {
		return ParserResult<std::string>(source, "Expecting \"" + regex + "\"");
	}
}
