#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"

Result<std::shared_ptr<Ast::Program>, std::vector<Error>> parse::program(Source source) {
	std::vector<std::shared_ptr<Ast::Node>> statements;
	std::vector<std::shared_ptr<Ast::Function>> functions;
	std::vector<Error> errors;

	bool there_was_an_error;
	while (!at_end(source)) {
		there_was_an_error = false;

		// Parse comment
		if (parse::comment(source).is_ok()) {
			source = parse::comment(source).get_source();
		}
		else if (match(source, "function ")) {
			auto result = parse::function(source);
			if (result.is_ok()) {
				functions.push_back(std::dynamic_pointer_cast<Ast::Function>(result.get_value()));
				source = result.get_source();
			}
			else {
				errors.push_back(result.get_error());
				there_was_an_error = true;
			}
		}
		else {
			auto result = parse::statement(source);
			if (result.is_ok()) {
				statements.push_back(result.get_value());
				source = result.get_source();
			}
			else {
				errors.push_back(result.get_error());
				there_was_an_error = true;
			}
		}
	
		if (!there_was_an_error && !parse::token(source, ".").is_error() && parse::token(source, ".").get_value() != "\n") {
			errors.push_back(Error(errors::unexpected_character(parse::token(source, ".").get_source()))); // tested in test/errors/expecting_line_ending.dm
		}

		// Advance until new line
		while (current(source) != '\n') source = source + 1;
		while (current(source) == '\n') source = source + 1;
	}

	if (errors.size() == 0) return Result<std::shared_ptr<Ast::Program>, std::vector<Error>>(std::make_shared<Ast::Program>(statements, functions, 1, 1, source.file));
	else                    return Result<std::shared_ptr<Ast::Program>, std::vector<Error>>(errors);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::function(Source source) {
	auto keyword = parse::token(source, "function");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_source(), keyword.get_error().error_message);
	source = keyword.get_source();

	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(identifier.get_source(), identifier.get_error().error_message);
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(left_paren.get_source(), left_paren.get_error().error_message);
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Identifier>> args;
	while (true) {
		auto arg_identifier = parse::identifier(source);
		if (arg_identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(arg_identifier.get_source(), arg_identifier.get_error().error_message);
		source = arg_identifier.get_source();
		args.push_back(std::dynamic_pointer_cast<Ast::Identifier>(arg_identifier.get_value()));

		if (parse::token(source, ",").is_ok()) source = parse::token(source, ",").get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(right_paren.get_source(), right_paren.get_error().error_message);
	source = right_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(expression.get_source(), expression.get_error().error_message);
	source = expression.get_source();

	auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression.get_value(), source.line, source.col, source.file);
	node->generic = true;
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::statement(Source source) {
	if (parse::assignment(source).is_ok()) return parse::assignment(source);
	if (parse::call(source).is_ok()) {
		auto result = parse::call(source);
		return ParserResult<std::shared_ptr<Ast::Node>>(std::dynamic_pointer_cast<Ast::Node>(result.get_value()), result.get_source());
	}
	return ParserResult<std::shared_ptr<Ast::Node>>(source, errors::expecting_statement(source));
}

ParserResult<std::shared_ptr<Ast::Node>> parse::assignment(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(identifier.get_source(), identifier.get_error().error_message);
	source = identifier.get_source();

	auto be = parse::token(source, "be");
	if (be.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(be.get_source(), be.get_error().error_message);
	source = be.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(expression.get_source(), expression.get_error().error_message);
	source = expression.get_source();

	auto node = std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), expression.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::call(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(left_paren.get_source(), left_paren.get_error().error_message);
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Expression>> args;
	while (true) {
		auto arg = parse::expression(source);
		if (arg.is_error()) return arg;
		source = arg.get_source();
		args.push_back(arg.get_value());

		if (parse::token(source, ",").is_ok()) source = parse::token(source, ",").get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(right_paren.get_source(), right_paren.get_error().error_message);
	source = right_paren.get_source();

	auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::expression(Source source) {
	auto op = parse::identifier(source);
	if (op.is_ok() && std::dynamic_pointer_cast<Ast::Identifier>(op.get_value())->value == "not") {
		source = op.get_source();

		auto expression = parse::binary(source);
		if (expression.is_error()) return expression;
		source = expression.get_source();

		std::vector<std::shared_ptr<Ast::Expression>> args;
		args.push_back(expression.get_value());
		auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(op.get_value()), args, source.line, source.col, source.file);
		return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
	}

	return parse::binary(source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::binary(Source source, int precedence) {
	static std::map<std::string, int> bin_op_precedence;
	bin_op_precedence["=="] = 1;
	bin_op_precedence["<"] = 2;
	bin_op_precedence["<="] = 2;
	bin_op_precedence[">"] = 2;
	bin_op_precedence[">="] = 2;
	bin_op_precedence["+"] = 3;
	bin_op_precedence["-"] = 3;
	bin_op_precedence["*"] = 4;
	bin_op_precedence["/"] = 4;

	if (precedence > std::max_element(bin_op_precedence.begin(), bin_op_precedence.end(), [] (auto a, auto b) { return a.second < b.second;})->second) {
		return parse::unary(source);
	}
	else {
		auto left = parse::binary(source, precedence + 1);

		if (left.is_error()) return left;
		source = left.get_source();

		while (true) {
			auto op = parse::op(source);
			if (op.is_error() || bin_op_precedence[std::dynamic_pointer_cast<Ast::Identifier>(op.get_value())->value] != precedence) break;
			source = op.get_source();

			auto right = parse::binary(source, precedence + 1);
			if (right.is_error()) return right;
			source = right.get_source();

			auto identifier = std::dynamic_pointer_cast<Ast::Identifier>(op.get_value());
			std::vector<std::shared_ptr<Ast::Expression>> args;
			args.push_back(left.get_value());
			args.push_back(right.get_value());
			left.value = std::make_shared<Ast::Call>(identifier, args, source.line, source.col, source.file);
		}

		return ParserResult<std::shared_ptr<Ast::Expression>>(left.get_value(), source);
	}
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::unary(Source source) {
	auto op = parse::op(source);
	if (op.is_ok() && std::dynamic_pointer_cast<Ast::Identifier>(op.get_value())->value == "-") {
		source = op.get_source();

		auto expression = parse::unary(source);
		if (expression.is_error()) return expression;
		source = expression.get_source();

		std::vector<std::shared_ptr<Ast::Expression>> args;
		args.push_back(expression.get_value());
		auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(op.get_value()), args, source.line, source.col, source.file);
		return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
	}

	return parse::primary(source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::primary(Source source) {
	if (parse::token(source, "\\(").is_ok()) return parse::grouping(source);
	if (parse::number(source).is_ok())       return parse::number(source);
	if (parse::integer(source).is_ok())      return parse::integer(source);
	if (parse::call(source).is_ok())         return parse::call(source);
	if (parse::boolean(source).is_ok())      return parse::boolean(source);
	if (parse::identifier(source).is_ok())   return parse::identifier(source);
	return ParserResult<std::shared_ptr<Ast::Expression>>(source, errors::unexpected_character(parse::token(source, "(?=.)").get_source())); // tested in test/errors/expecting_primary.dm
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::grouping(Source source) {
	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(left_paren.get_source(), left_paren.get_error().error_message);
	source = left_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(right_paren.get_source(), right_paren.get_error().error_message);
	source = right_paren.get_source();

	return ParserResult<std::shared_ptr<Ast::Expression>>(expression.get_value(), source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::number(Source source) {
	auto result = parse::token(source, "([0-9]*)?[.][0-9]+");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(source, result.get_error().error_message);
	source = result.get_source();
	double value = atof(result.get_value().c_str());
	auto node = std::make_shared<Ast::Number>(value, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::integer(Source source) {
	auto result = parse::token(source, "[0-9]+");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(source, result.get_error().error_message);
	source = result.get_source();
	char* ptr;
	int64_t value = strtol(result.get_value().c_str(), &ptr, 10);
	auto node = std::make_shared<Ast::Integer>(value, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::boolean(Source source) {
	auto result = parse::token(source, "(false|true)");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(source, result.get_error().error_message);
	source = result.get_source();
	auto node = std::make_shared<Ast::Boolean>(result.get_value() == "false" ? false : true, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::identifier(Source source) {
	auto result = parse::token(source, "[a-zA-Z_][a-zA-Z0-9_]*");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(source, result.get_error().error_message);
	source = result.get_source();
	auto node = std::make_shared<Ast::Identifier>(result.get_value(), source.line, source.col - result.get_value().size(), source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::op(Source source) {
	auto result = parse::token(source, "(\\+|-|\\*|\\/|<=|<|>=|>|==)");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(source, result.get_error().error_message);
	source = parse::token(source, "(?=.)").get_source();
	auto node = std::make_shared<Ast::Identifier>(result.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, result.get_source());
}

ParserResult<std::string> parse::token(Source source, std::string regex) {
	while (!parse::whitespace(source).is_error() || !parse::comment(source).is_error()) {
		if (!parse::whitespace(source).is_error()) {
			source = parse::whitespace(source).get_source();
		}
		else if (!parse::comment(source).is_error()) {
			source = parse::comment(source).get_source();
		}
	}
	return parse::regex(source, regex);
}

ParserResult<std::string> parse::whitespace(Source source) {
	return parse::regex(source, "[ \\r\\t]+");
}

ParserResult<std::string> parse::comment(Source source) {
	if (parse::regex(source, "(---)(.|\\n|[\\r\\n])*(---)").is_ok()) return parse::regex(source, "(---)(.|\\n|[\\r\\n])*(---)"); // Multiline comment
	else                                                             return parse::regex(source, "(--).*"); // Single line comment
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