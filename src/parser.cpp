#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "parser.hpp"

Ast::Program parse::program(Source source) {
	std::vector<Ast::Node*> expressions;

	while (!at_end(source)) {
		auto result = parse::expression(source);
		if (!result.error()) {
			expressions.push_back(result.value);
			source = result.source;
		}
		while (current(source) == '\n') source = source + 1; // Eat new lines
	}

	return Ast::Program(expressions, 1, 1, source.file);
}

ParserResult<Ast::Node*> parse::expression(Source source) {
	auto result = parse::number(source);
	double value = atof(result.value.c_str());
	Ast::Number* node = new Ast::Number(value, source.line, source.col, source.file);
	return ParserResult<Ast::Node*>(node, result.source);
}

ParserResult<std::string> parse::number(Source source) {return parse::regex(source, "([0-9]*[.])?[0-9]+");}

ParserResult<std::string> parse::string(Source source, std::string str) {
	if (match(source, str)) return ParserResult<std::string>(str, source + str.size());
	else                    return ParserResult<std::string>(source, "Expecting \"" + str + "\"");
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
