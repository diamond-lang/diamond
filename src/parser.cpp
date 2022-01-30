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
	else {
		auto block = result.get_value();
		auto program = std::make_shared<Ast::Program>(block->statements, block->functions, block->use_statements, block->line, block->col, block->file);
		return Result<std::shared_ptr<Ast::Program>, Errors>(program);
	}
}

Source eat_indentation(Source source) {
	while(current(source) == ' ' || current(source) == '\t') {
		source = source + 1;
	}
	return source;
}

size_t new_indentation_level(Source source) {
	if (source.indentation_level == -1) {
		source.indentation_level = 1;
	}
	else {
		// Set indentation or error
		Source indent = eat_indentation(source);
		if (indent.col > source.indentation_level) {
			source.indentation_level = indent.col;
		}
		else {
			source.indentation_level = -1;
		}
	}
	return source.indentation_level;
}

Source advance_until_next_statement(Source source) {
	while (current(source) == '\n' || parse::comment(source).is_ok()) {
		if (current(source) == '\n') source = source + 1;
		else                         source = parse::comment(source).get_source();
	}

	return source;
}

ParserResult<std::shared_ptr<Ast::Block>> parse::block(Source source) {
	// Advance until next statement
	source = advance_until_next_statement(source);

	// Set new indentation level
	size_t indentation_level = new_indentation_level(source);
	if (indentation_level == -1) return ParserResult<std::shared_ptr<Ast::Block>>(Errors{errors::expecting_new_indentation_level(source)}); // tested in errors/expecting_new_indentation_level.dm
	source.indentation_level = indentation_level;

	// Create variables to store statements and functions
	std::vector<std::shared_ptr<Ast::Use>> use_statements;
	std::vector<std::shared_ptr<Ast::Node>> statements;
	std::vector<std::shared_ptr<Ast::Function>> functions;
	std::vector<Error> errors;

	// Main loop, parses line by line
	while (!at_end(source)) {
		Source aux = source;

		// Advance until next statement
		source = advance_until_next_statement(source);
		if (at_end(source)) break;

		// Check indentation
		Source indent = eat_indentation(source);
		if (indent.col < indentation_level) {
			source = aux;
			break;
		}
		else if (indent.col > indentation_level) {
			errors.push_back(errors::unexpected_indent(source)); // tested in test/errors/unexpected_indentation_1.dm and test/errors/unexpected_indentation_2.dm
			while (current(source) != '\n') source = source + 1; // advances until new line
			continue;
		}

		// Parse statement
		auto result = parse::statement(source);
		if (result.is_ok()) {
			source = result.get_source();
			source.indentation_level = indentation_level; // remember indentation level

			if (std::dynamic_pointer_cast<Ast::Function>(result.get_value())) {
				functions.push_back(std::dynamic_pointer_cast<Ast::Function>(result.get_value()));
			}
			else if (std::dynamic_pointer_cast<Ast::Use>(result.get_value())) {
				use_statements.push_back(std::dynamic_pointer_cast<Ast::Use>(result.get_value()));
			}
			else {
				statements.push_back(result.get_value());
			}

			if (!at_end(source) && current(source) != '\n') {
				errors.push_back(errors::unexpected_character(parse::regex(source, "[ \\r\\t]*").get_source())); // tested in test/errors/expecting_line_ending.dm
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
	if (errors.size() == 0) return ParserResult<std::shared_ptr<Ast::Block>>(std::make_shared<Ast::Block>(statements, functions, use_statements, 1, 1, source.file), source);
	else                    return ParserResult<std::shared_ptr<Ast::Block>>(errors);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::expression_block(Source source) {
	// Advance until next statement
	source = advance_until_next_statement(source);

	// Set new indentation level
	size_t indentation_level = new_indentation_level(source);
	if (indentation_level == -1) return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{errors::expecting_new_indentation_level(source)}); // tested in errors/expecting_new_indentation_level.dm
	source.indentation_level = indentation_level;

	// Parse expression
	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	if (!at_end(source) && current(source) != '\n') {
		return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{errors::unexpected_character(parse::regex(source, "[ \\r\\t]*").get_source())});
	}

	return ParserResult<std::shared_ptr<Ast::Expression>>(expression.get_value(), source);
}

static bool is_assignment(Source source) {
	auto result = parse::identifier(source);
	if      (result.is_ok() && parse::token(result.get_source(), "be").is_ok()) return true;
	else if (result.is_ok() && parse::token(result.get_source(), "=").is_ok())  return true;
	else if (parse::token(source, "nonlocal").is_ok())                          return true;
	else                                                                        return false;
}

ParserResult<std::shared_ptr<Ast::Node>> parse::statement(Source source) {
	if (parse::identifier(source, "function").is_ok()) return parse::function(source);
	if (parse::identifier(source, "return").is_ok())   return parse::return_stmt(source);
	if (parse::identifier(source, "if").is_ok())       return parse::if_else_stmt(source);
	if (parse::identifier(source, "while").is_ok())    return parse::while_stmt(source);
	if (parse::identifier(source, "break").is_ok())    return parse::break_stmt(source);
	if (parse::identifier(source, "continue").is_ok()) return parse::continue_stmt(source);
	if (parse::identifier(source, "use").is_ok())      return parse::use_stmt(source);
	if (parse::identifier(source, "include").is_ok())  return parse::include_stmt(source);
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
	while (current(source) != ')') {
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
	if (expression.is_ok()) {
		source = expression.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression.get_value(), source.line, source.col, source.file);
		node->generic = true;
		return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto block = parse::block(source);
	if (block.is_ok()) {
		source = block.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, block.get_value(), source.line, source.col, source.file);
		node->generic = true;
		return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto expression_block = parse::expression_block(source);
	if (expression_block.is_ok()) {
		source = expression_block.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression_block.get_value(), source.line, source.col, source.file);
		node->generic = true;
		return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
	}

	return ParserResult<std::shared_ptr<Ast::Node>>(block.get_errors());
}

ParserResult<std::shared_ptr<Ast::Node>> parse::assignment(Source source) {
	auto nonlocal = parse::token(source, "nonlocal");
	if (nonlocal.is_ok()) {
		source = nonlocal.get_source();
	}

	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(identifier.get_errors());
	source = identifier.get_source();

	auto equal = parse::token(source, "=");
	auto be = parse::token(source, "be");
	if (equal.is_error() && be.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(Errors{std::string("Errors: Expecting '=' or 'be'.")});
	if (equal.is_ok()) source = equal.get_source();
	if (be.is_ok()) source = be.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(expression.get_errors());
	source = expression.get_source();

	auto node = std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), expression.get_value(), equal.is_ok(), nonlocal.is_ok(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::return_stmt(Source source) {
	auto keyword = parse::identifier(source, "return");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto expression = parse::expression(source);
	if (expression.is_ok()) {
		source = expression.get_source();
		auto node = std::make_shared<Ast::Return>(expression.get_value(), source.line, source.col, source.file);
		return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto node = std::make_shared<Ast::Return>(nullptr, source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::break_stmt(Source source) {
	auto keyword = parse::identifier(source, "break");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto node = std::make_shared<Ast::Break>(source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::continue_stmt(Source source) {
	auto keyword = parse::identifier(source, "continue");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto node = std::make_shared<Ast::Continue>(source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::if_else_stmt(Source source) {
	size_t indentation_level = source.indentation_level;

	auto keyword = parse::identifier(source, "if");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto condition = parse::expression(source);
	if (condition.is_error()) return condition.get_errors();
	source = condition.get_source();

	auto block = parse::block(source);
	if (block.is_error()) return block.get_errors();
	source = block.get_source();

	// Adance until new statement
	Source aux = advance_until_next_statement(source);

	// Match indentation
	Source indent = eat_indentation(aux);
	if (indent.col == indentation_level) {
		aux = indent;
		aux.indentation_level = indentation_level;

		// Match else
		auto keyword_else = parse::identifier(aux, "else");
		if (keyword_else.is_ok()) {
			aux = keyword_else.get_source();
			auto else_block = parse::block(aux);
			if (else_block.is_error()) return else_block.get_errors();
			aux = else_block.get_source();
			auto node = std::make_shared<Ast::IfElseStmt>(condition.get_value(), block.get_value(), else_block.get_value(), aux.line, aux.col, aux.file);
			return ParserResult<std::shared_ptr<Ast::Node>>(node, aux);
		}
	}

	// Return
	auto node = std::make_shared<Ast::IfElseStmt>(condition.get_value(), block.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::while_stmt(Source source) {
	auto keyword = parse::identifier(source, "while");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto condition = parse::expression(source);
	if (condition.is_error()) return condition.get_errors();
	source = condition.get_source();

	auto block = parse::block(source);
	if (block.is_error()) return block.get_errors();
	source = block.get_source();

	// Return
	auto node = std::make_shared<Ast::WhileStmt>(condition.get_value(), block.get_value(), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::use_stmt(Source source) {
	auto keyword = parse::identifier(source, "use");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto path = parse::string(source);
	if (path.is_error()) return path.get_errors();
	source = path.get_source();

	// Return
	auto node = std::make_shared<Ast::Use>(std::dynamic_pointer_cast<Ast::String>(path.get_value()), source.line, source.col, source.file);
	return ParserResult<std::shared_ptr<Ast::Node>>(node, source);
}

ParserResult<std::shared_ptr<Ast::Node>> parse::include_stmt(Source source) {
	auto keyword = parse::identifier(source, "include");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto path = parse::string(source);
	if (path.is_error()) return path.get_errors();
	source = path.get_source();

	// Return
	auto node = std::make_shared<Ast::Use>(std::dynamic_pointer_cast<Ast::String>(path.get_value()), source.line, source.col, source.file);
	node->include = true;
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
	while (current(source) != ')') {
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
	if (parse::if_else_expr(source).is_ok()) return parse::if_else_expr(source);
	return parse::not_expr(source);
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::if_else_expr(Source source) {
	auto keyword = parse::identifier(source, "if");
	if (keyword.is_error()) return ParserResult<std::shared_ptr<Ast::Expression>>(keyword.get_errors());
	source = keyword.get_source();

	size_t indentation_level = keyword.get_value()->col;

	auto condition = parse::expression(source);
	if (condition.is_error()) return condition.get_errors();
	source = condition.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) {
		expression = parse::expression_block(source);
		if (expression.is_error()) return parse::expression(source).get_errors();
	}
	source = expression.get_source();

	// Adance until new statement
	Source aux = advance_until_next_statement(source);

	// Match indentation
	Source indent = eat_indentation(aux);
	if (indent.col == indentation_level) {
		aux = indent;
		aux.indentation_level = indentation_level;

		// Match else
		auto keyword_else = parse::identifier(aux, "else");
		if (keyword_else.is_ok()) {
			aux = keyword_else.get_source();

			auto else_expression = parse::expression(aux);
			if (else_expression.is_error()) {
				else_expression = parse::expression_block(aux);
				if (else_expression.is_error()) return parse::expression(aux).get_errors();
			}
			aux = else_expression.get_source();
			
			auto node = std::make_shared<Ast::IfElseExpr>(condition.get_value(), expression.get_value(), else_expression.get_value(), aux.line, aux.col, aux.file);
			return ParserResult<std::shared_ptr<Ast::Expression>>(node, aux);
		}
	}

	// Return
	return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{std::string("Expecting else branch in if/else expression")});
}

ParserResult<std::shared_ptr<Ast::Expression>> parse::not_expr(Source source) {
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
	return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{errors::unexpected_character(parse::regex(source, "[ \\r\\t]*").get_source())}); // tested in test/errors/expecting_primary.dm
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

ParserResult<std::shared_ptr<Ast::Expression>> parse::string(Source source) {
	source = parse::whitespace(source).get_source();
	
	if (current(source) != '"') return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{std::string("Error: Expecting \"(?<=\").*(?=\")")});
	source = source + 1;

	std::string result = "";
	while (!at_end(source) && current(source) != '"' && current(source) != '\n') {
		result += current(source);
		source = source + 1;
	}

	if (current(source) != '"') return ParserResult<std::shared_ptr<Ast::Expression>>(Errors{std::string("Error: Expecting \"(?<=\").*(?=\")")});
	source = source + 1;

	auto node = std::make_shared<Ast::String>(result, source.line, source.col - result.size() - 1, source.file);
	return ParserResult<std::shared_ptr<Ast::Expression>>(node, source);
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
