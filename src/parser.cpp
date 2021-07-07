#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"

// Prototypes
// ----------

bool is_assignment(Source source);


//  Implementations
//  ---------------

Result<std::shared_ptr<Ast::Program>, std::vector<Error>> parse::program(Source source) {
	std::vector<std::shared_ptr<Ast::Node>> statements;
	std::vector<std::shared_ptr<Ast::Function>> functions;
	std::vector<Error> errors;

	while (!at_end(source)) {
		// Parse comment
		if (parse::comment(source).is_ok()) {
			source = parse::comment(source).get_source();
		}
		else if (parse::function(source).is_ok()) {
			auto result = parse::function(source);
			if (result.is_ok()) {
				functions.push_back(std::dynamic_pointer_cast<Ast::Function>(result.get_value()));
				source = result.get_source();
			}
			else {
				errors.push_back(result.get_error());
			}
		}
		else if (is_assignment(source)) {
			auto result = parse::assignment(source);
			if (result.is_ok()) {
				statements.push_back(result.get_value());
				source = result.get_source();
			}
			else {
				errors.push_back(result.get_error());
			}
		}
		else {
			// Parse expression
			auto result = parse::expression(source);
			if (!result.is_error()) {
				statements.push_back(result.get_value());
				source = result.get_source();
			}
			else {
				errors.push_back(result.get_error());
			}
		}

		if (!parse::token(source, ".").is_error() && parse::token(source, ".").get_value() != "\n") {
			errors.push_back(Error(errors::unexpected_character(parse::token(source, ".").get_source())));
		}

		// Advance until new line
		while (current(source) != '\n') source = source + 1;
		while (current(source) == '\n') source = source + 1; // Eat new lines
	}

	if (errors.size() == 0) return Result<std::shared_ptr<Ast::Program>, std::vector<Error>>(std::make_shared<Ast::Program>(statements, functions, 1, 1, source.file));
	else                    return Result<std::shared_ptr<Ast::Program>, std::vector<Error>>(errors);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::function(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(left_paren.get_source(), left_paren.get_error().error_message);
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Identifier>> args;
	while (true) {
		auto arg_identifier = parse::identifier(source);
		if (arg_identifier.is_error()) return arg_identifier;
		source = arg_identifier.get_source();
		args.push_back(std::dynamic_pointer_cast<Ast::Identifier>(arg_identifier.get_value()));

		if (parse::token(source, ",").is_ok()) source = parse::token(source, ",").get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(right_paren.get_source(), right_paren.get_error().error_message);
	source = right_paren.get_source();

	auto is = parse::token(source, "is");
	if (is.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(is.get_source(), is.get_error().error_message);;
	source = is.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::call(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(left_paren.get_source(), left_paren.get_error().error_message);
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Node>> args;
	while (true) {
		auto arg = parse::expression(source);
		if (arg.is_error()) return arg;
		source = arg.get_source();
		args.push_back(arg.get_value());

		if (parse::token(source, ",").is_ok()) source = parse::token(source, ",").get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(right_paren.get_source(), right_paren.get_error().error_message);
	source = right_paren.get_source();

	auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::assignment(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto be = parse::token(source, "be");
	if (be.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(be.get_source(), be.get_error().error_message);
	source = be.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	auto node = std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), expression.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::expression(Source source) {
	return parse::binary(source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::binary(Source source, int precedence) {
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
			std::vector<std::shared_ptr<Ast::Node>> args;
			args.push_back(left.get_value());
			args.push_back(right.get_value());
			left.value = std::make_shared<Ast::Call>(identifier, args, source.line, source.col, source.file);
		}

		return ParserResult<std::shared_ptr<Ast::Node>>(left.get_value(), source);
	}
}

ParserResult<std::shared_ptr<Ast::Node>> parse::unary(Source source) {
	if (parse::number(source).is_ok())     return parse::number(source);
	if (parse::call(source).is_ok())       return parse::call(source);
	if (parse::identifier(source).is_ok()) return parse::identifier(source);
	return ParserResult<std::shared_ptr<Ast::Node>>(source, errors::unexpected_character(parse::token(source, "(?=.)").get_source()));
}

ParserResult<std::shared_ptr<Ast::Node>> parse::number(Source source) {
	auto result = parse::token(source, "([0-9]*[.])?[0-9]+");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(source, errors::expecting_number(result.get_source()));
	source = result.get_source();
	double value = atof(result.get_value().c_str());
	auto node = std::make_shared<Ast::Number>(value, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Node>> parse::identifier(Source source) {
	auto result = parse::token(source, "[a-zA-Z_][a-zA-Z0-9_]*");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(source, errors::expecting_identifier(result.get_source()));
	source = parse::token(source, "(?=.)").get_source();
	if (result.get_value() == "false" || result.get_value() == "true") {
		auto node = std::make_shared<Ast::Boolean>(result.get_value() == "false" ? false : true, source.line, source.col, source.file);
		return ParserResult<std::shared_ptr<Ast::Node>>(node, result.get_source());
	}
	else {
		auto node = std::make_shared<Ast::Identifier>(result.get_value(), source.line, source.col, source.file);
		return ParserResult<std::shared_ptr<Ast::Node>>(node, result.get_source());
	}
}

ParserResult<std::shared_ptr<Ast::Node>> parse::op(Source source) {
	auto result = parse::token(source, "(\\+|-|\\*|\\/|<=|<|>=|>|==)");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(source, errors::expecting_identifier(result.get_source()));
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

bool is_assignment(Source source) {
	auto result = parse::identifier(source);
	if (result.is_ok() && parse::token(result.get_source(), "be").is_ok()) return true;
	else                                                                   return false;
}
