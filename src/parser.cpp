#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"

Result<std::shared_ptr<Ast::Program>, std::vector<Error>> parse::program(Source source) {
	auto result = parse::block(source);
	if (result.is_error()) return Result<std::shared_ptr<Ast::Program>, Errors>(result.get_errors());
	else                   return Result<std::shared_ptr<Ast::Program>, Errors>(std::dynamic_pointer_cast<Ast::Program>(result.get_value()));
}

ParserResult<std::shared_ptr<Ast::Block>> parse::block(Source source) {
	// Create variables to store statements and functions
	std::vector<std::shared_ptr<Ast::Node>> statements;
	std::vector<std::shared_ptr<Ast::Function>> functions;
	std::vector<Error> errors;

	// Main loop, parses line by line
	while (!at_end(source)) {
		// Eat new lines or comments
		while (current(source) == '\n' || parse::comment(source).is_ok()) {
			if (current(source) == '\n') source = source + 1;
			else                         source = parse::comment(source).get_source();
		}

		if (at_end(source)) break;

		// Check indentation
		Source indent = parse::indent(source).get_source();
		if (indent.col != source.indentation_level) {
			if (indent.col < source.indentation_level) {
				break;
			}
			else {
				errors.push_back(errors::unexpected_indent(source)); // tested in test/errors/unexpected_indentation_2.dm
				while (current(source) != '\n') source = source + 1; // advances until new line
				continue;
			}
		}

		// Parse statement
		auto result = parse::statement(source);
		if (result.is_ok()) {
			source = result.get_source();
		
			if (std::dynamic_pointer_cast<Ast::Function>(result.get_value())) {
				functions.push_back(std::dynamic_pointer_cast<Ast::Function>(result.get_value()));
			}
			else {
				statements.push_back(result.get_value());
			}

			if (!at_end(source) && current(source) != '\n') {
				errors.push_back(errors::unexpected_character(parse::whitespace(source).get_source())); // tested in test/errors/expecting_line_ending.dm
			}
		}
		else {
			auto result_errors = result.get_errors();
			errors.insert(errors.end(), result_errors.begin(), result_errors.end());
		}

		// Advance until new line
		while (current(source) != '\n') source = source + 1;
	}

	// Return
	if (errors.size() == 0) return ParserResult<std::shared_ptr<Ast::Block>>(std::make_shared<Ast::Block>(statements, functions, 1, 1, source.file), source);
	else                    return ParserResult<std::shared_ptr<Ast::Block>>(errors);
}

static bool is_assignment(Source source) {
	auto result = parse::identifier(source);
	if (result.is_ok() && parse::token(result.get_source(), "be").is_ok()) return true;
	else                                                                   return false;
}

ParserResult<std::shared_ptr<Ast::Node>> parse::statement(Source source) {
	if (parse::identifier(source, "function").is_ok()) return parse::function(source);
	if (is_assignment(source)) return parse::assignment(source);
	if (parse::call(source).is_ok()) {
		auto result = parse::call(source);
		return ParserResult<std::shared_ptr<Ast::Node>>(std::dynamic_pointer_cast<Ast::Node>(result.get_value()), result.get_source());
	}
	return ParserResult<std::shared_ptr<Ast::Node>>(Errors{errors::expecting_statement(source)}); // tested in test/errors/expecting_statement.dm
}

ParserResult<std::shared_ptr<Ast::Node>> parse::function(Source source) {
	auto keyword = parse::identifier(source, "function");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(identifier.get_errors());
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(left_paren.get_errors());
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Identifier>> args;
	while (true) {
		auto arg_identifier = parse::identifier(source);
		if (arg_identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(arg_identifier.get_errors());
		source = arg_identifier.get_source();
		args.push_back(std::dynamic_pointer_cast<Ast::Identifier>(arg_identifier.get_value()));

		if (parse::token(source, ",").is_ok()) source = parse::token(source, ",").get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(right_paren.get_errors());
	source = right_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(expression.get_errors());
	source = expression.get_source();

	auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression.get_value(), source.line, source.col, source.file);
	node->generic = true;
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::assignment(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(identifier.get_errors());
	source = identifier.get_source();

	auto be = parse::token(source, "be");
	if (be.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(be.get_errors());
	source = be.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(expression.get_errors());
	source = expression.get_source();

	auto node = std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), expression.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::call(Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(left_paren.get_errors());
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
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(right_paren.get_errors());
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
	return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{errors::unexpected_character(parse::token(source, "(?=.)").get_source())}); // tested in test/errors/expecting_primary.dm
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::grouping(Source source) {
	auto left_paren = parse::token(source, "\\(");
	if (left_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(left_paren.get_errors());
	source = left_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	auto right_paren = parse::token(source, "\\)");
	if (right_paren.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(right_paren.get_errors());
	source = right_paren.get_source();

	return ParserResult<std::shared_ptr<Ast::Expression>>(expression.get_value(), source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::number(Source source) {
	auto result = parse::token(source, "([0-9]*)?[.][0-9]+");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	double value = atof(result.get_value().c_str());
	auto node = std::make_shared<Ast::Number>(value, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::integer(Source source) {
	auto result = parse::token(source, "[0-9]+");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	char* ptr;
	int64_t value = strtol(result.get_value().c_str(), &ptr, 10);
	auto node = std::make_shared<Ast::Integer>(value, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::boolean(Source source) {
	auto result = parse::token(source, "(false|true)");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	auto node = std::make_shared<Ast::Boolean>(result.get_value() == "false" ? false : true, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::identifier(Source source) {
	auto result = parse::token(source, "[a-zA-Z_][a-zA-Z0-9_]*");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	auto node = std::make_shared<Ast::Identifier>(result.get_value(), source.line, source.col - result.get_value().size(), source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::identifier(Source source, std::string identifier) {
	auto result = parse::identifier(source);
	if (result.is_ok() && std::dynamic_pointer_cast<Ast::Identifier>(result.get_value())->value == identifier) return result;
	else return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{"Expecting \"" + identifier + "\""});

}
ParserResult<std::shared_ptr<Ast::Node>> parse::op(Source source) {
	auto result = parse::token(source, "(\\+|-|\\*|\\/|<=|<|>=|>|==)");
	if (result.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(result.get_errors());
	source = parse::token(source, "(?=.)").get_source();
	auto node = std::make_shared<Ast::Identifier>(result.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, result.get_source());
}

ParserResult<std::string> parse::token(Source source, std::string regex) {
	while (parse::whitespace(source).is_ok() || parse::comment(source).is_ok()) {
		if (parse::whitespace(source).is_ok()) {
			source = parse::whitespace(source).get_source();
		}
		else if (parse::comment(source).is_ok()) {
			source = parse::comment(source).get_source();
		}
	}
	return parse::regex(source, regex);
}

ParserResult<std::string> parse::whitespace(Source source) {
	return parse::regex(source, "[ \\r\\t]+");
}

ParserResult<std::string> parse::comment(Source source) {
	auto result = parse::whitespace(source);
	if (result.is_ok()) source = result.get_source(); // Ignore white space before comment
	if (parse::regex(source, "(---)(.|\\n|[\\r\\n])*(---)").is_ok()) return parse::regex(source, "(---)(.|\\n|[\\r\\n])*(---)"); // Multiline comment
	else                                                             return parse::regex(source, "(--).*"); // Single line comment
}

ParserResult<std::string> parse::regex(Source source, std::string regex) {
	std::smatch sm;
	if (std::regex_search((std::string::const_iterator) source.it, (std::string::const_iterator) source.end, sm, std::regex(regex), std::regex_constants::match_continuous)) {
		return ParserResult<std::string>(sm[0].str(), source + sm[0].str().size());
	}
	else {
		return ParserResult<std::string>(Errors{"Expecting \"" + regex + "\""});
	}
}

ParserResult<void*> parse::indent(Source source) {
	while (current(source) == ' ') source = source + 1;
	return ParserResult<void*>(nullptr, source);
}
