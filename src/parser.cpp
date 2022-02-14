#include <iostream>
#include <algorithm>
#include <map>
#include <regex>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"
#include "ast.hpp"

// Prototypes and definitions
// --------------------------
namespace parse {
	bool at_end(parse::Source source) {
		return source.position >= (*source.tokens).size();
	}
	
	token::Token current(parse::Source source) {
		assert(!at_end(source));
		return (*source.tokens)[source.position];
	}

	size_t current_indentation(parse::Source source) {
		assert(source.indentation_level.size() > 0);
		return source.indentation_level[source.indentation_level.size() - 1];
	}

	Source advance(parse::Source source) {
		if (at_end(source)) return source;
		source.position++;
		if (at_end(source)) return source;
		source.line = current(source).line;
		source.column = current(source).column;
		return source;
	}

	Source advance(parse::Source source, unsigned int offset) {
		if (offset == 0) return source;
		else             return advance(advance(source), offset - 1);
	}

	template <class T>
	struct Result {
		bool ok;
		T value;
		Source source;
		Errors errors;

		Result<T>(T value, Source source) : ok(true), value(value), source(source) {}
		Result<T>(Errors errors) : ok(false), errors(errors) {}

		bool is_ok()    {return this->ok;}
		bool is_error() {return !this->is_ok();}
		T get_value() {
			assert(this->ok);
			return this->value;
		}
		Source get_source() {
			assert(this->ok);
			return this->source;
		}
		Errors get_errors() {
			assert(!(this->ok));
			return this->errors;
		}
	};

	Source advance_until_next_statement(parse::Source source);
	size_t get_indentation(parse::Source source);

	Result<std::shared_ptr<Ast::Block>> block(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> expression_block(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> function(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> statement(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> assignment(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> return_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> break_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> continue_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> if_else_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> while_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> use_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> include_stmt(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> call(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> expression(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> if_else_expr(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> not_expr(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> binary(parse::Source source, int precedence = 1);
	Result<std::shared_ptr<Ast::Expression>> unary(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> primary(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> grouping(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> number(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> integer(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> boolean(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> identifier(parse::Source source);
	Result<std::shared_ptr<Ast::Expression>> string(parse::Source source);
	Result<std::shared_ptr<Ast::Node>> op(parse::Source source);
	Result<token::Token> token(parse::Source source, token::TokenVariant token);
}

parse::Source parse::advance_until_next_statement(parse::Source source) {
	while (!at_end(source) && current(source) == token::NewLine) {
		source = advance(source);
	}

	return source;
}

size_t parse::get_indentation(parse::Source source) {
	return source.column;
}

// Parsing
// -------
::Result<std::shared_ptr<Ast::Program>, Errors> parse::program(std::vector<token::Token> tokens, std::filesystem::path file) {
	auto result = parse::block(parse::Source(&tokens, file));
	if (result.is_error()) return ::Result<std::shared_ptr<Ast::Program>, Errors>(result.get_errors());
	else {
		auto block = result.get_value();
		auto program = std::make_shared<Ast::Program>(block->statements, block->functions, block->use_statements, block->line, block->col, block->file);
		return ::Result<std::shared_ptr<Ast::Program>, Errors>(program);
	}
}

parse::Result<std::shared_ptr<Ast::Block>> parse::block(parse::Source source) {
	// Advance until next statement
	source = advance_until_next_statement(source);

	// Set new indentation level
	if (source.indentation_level.size() == 0) {
		source.indentation_level.push_back(1);
	}
	else {
		size_t previous = current_indentation(source);
		source.indentation_level.push_back(advance_until_next_statement(source).column);
		if (previous >= current_indentation(source)) {
			return parse::Result<std::shared_ptr<Ast::Block>>(Errors{errors::expecting_new_indentation_level(source)}); // tested in errors/expecting_new_indentation_level.dm
		}
	}
 
	// Create variables to store statements and functions
	std::vector<std::shared_ptr<Ast::Use>> use_statements;
	std::vector<std::shared_ptr<Ast::Node>> statements;
	std::vector<std::shared_ptr<Ast::Function>> functions;
	std::vector<Error> errors;

	// Main loop, parses line by line
	while (!at_end(source)) {
		Source backup = source;

		// Advance until next statement
		source = advance_until_next_statement(source);
		if (at_end(source)) break;

		// Check indentation
		if (current(source).column < current_indentation(source)) {
			source = backup;
			break;
		}
		if (current(source).column > current_indentation(source)) {
			auto aux = source;
			aux.column = 1;
			errors.push_back(errors::unexpected_indent(aux)); // tested in test/errors/unexpected_indentation_1.dm and test/errors/unexpected_indentation_2.dm
			while (!at_end(source) && current(source) != token::NewLine) source = advance(source); // advances until new line
			continue;
		}

		// Parse statement
		auto result = parse::statement(source);
		if (result.is_ok()) {
			source = result.get_source();

			if (std::dynamic_pointer_cast<Ast::Function>(result.get_value())) {
				functions.push_back(std::dynamic_pointer_cast<Ast::Function>(result.get_value()));
			}
			else if (std::dynamic_pointer_cast<Ast::Use>(result.get_value())) {
				use_statements.push_back(std::dynamic_pointer_cast<Ast::Use>(result.get_value()));
			}
			else {
				statements.push_back(result.get_value());
			}

			if (!at_end(source) && current(source) != token::NewLine) {
				errors.push_back(errors::unexpected_character(source)); // tested in test/errors/expecting_line_ending.dm
			}
		}
		else {
			auto result_errors = result.get_errors();
			errors.insert(errors.end(), result_errors.begin(), result_errors.end());
		}

		// Advance until new line
		while (!at_end(source) && current(source) != token::NewLine) source = advance(source);
	}

	// Pop indentation level
	source.indentation_level.pop_back();

	// Return
	if (errors.size() == 0) return Result<std::shared_ptr<Ast::Block>>(std::make_shared<Ast::Block>(statements, functions, use_statements, 1, 1, source.file), source);
	else                    return Result<std::shared_ptr<Ast::Block>>(errors);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::expression_block(parse::Source source) {
	// Advance until next statement
	source = advance_until_next_statement(source);

	// Set new indentation level
	size_t previous = current_indentation(source);
	source.indentation_level.push_back(advance_until_next_statement(source).column);
	if (previous >= current_indentation(source)) {
		return parse::Result<std::shared_ptr<Ast::Expression>>(Errors{errors::expecting_new_indentation_level(source)}); // tested in errors/expecting_new_indentation_level.dm
	}

	// Parse expression
	auto expression = parse::expression(source);
	if (expression.is_error()) {
		return expression;
	}
	source = expression.get_source();

	if (!at_end(source) && current(source) != token::NewLine) {
		return parse::Result<std::shared_ptr<Ast::Expression>>(Errors{errors::unexpected_character(source)});
	}

	// Pop back indentation level
	source.indentation_level.pop_back();

	return parse::Result<std::shared_ptr<Ast::Expression>>(expression.get_value(), source);
}

static bool is_assignment(parse::Source source) {
	auto result = parse::identifier(source);
	if      (result.is_ok() && parse::token(result.get_source(), token::Be).is_ok())    return true;
	else if (result.is_ok() && parse::token(result.get_source(), token::Equal).is_ok()) return true;
	else if (parse::token(source, token::NonLocal).is_ok())                             return true;
	else                                                                                return false;
}

parse::Result<std::shared_ptr<Ast::Node>> parse::statement(parse::Source source) {
	if (parse::token(source, token::Function).is_ok()) return parse::function(source);
	if (parse::token(source, token::Return).is_ok())   return parse::return_stmt(source);
	if (parse::token(source, token::If).is_ok())       return parse::if_else_stmt(source);
	if (parse::token(source, token::While).is_ok())    return parse::while_stmt(source);
	if (parse::token(source, token::Break).is_ok())    return parse::break_stmt(source);
	if (parse::token(source, token::Continue).is_ok()) return parse::continue_stmt(source);
	if (parse::token(source, token::Use).is_ok())      return parse::use_stmt(source);
	if (parse::token(source, token::Include).is_ok())  return parse::include_stmt(source);
	if (is_assignment(source)) return parse::assignment(source);
	if (parse::call(source).is_ok()) {
		auto result = parse::call(source);
		return parse::Result<std::shared_ptr<Ast::Node>>(std::dynamic_pointer_cast<Ast::Node>(result.get_value()), result.get_source());
	}
	return parse::Result<std::shared_ptr<Ast::Node>>(Errors{errors::expecting_statement(source)}); // tested in test/errors/expecting_statement.dm
}

parse::Result<std::shared_ptr<Ast::Node>> parse::function(parse::Source source) {
	auto keyword = parse::token(source, token::Function);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(identifier.get_errors());
	source = identifier.get_source();

	auto left_paren = parse::token(source, token::LeftParen);
	if (left_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(left_paren.get_errors());
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Identifier>> args;
	while (current(source) != token::RightParen && current(source) != token::EndOfFile) {
		auto arg_identifier = parse::identifier(source);
		if (arg_identifier.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(arg_identifier.get_errors());
		source = arg_identifier.get_source();
		args.push_back(std::dynamic_pointer_cast<Ast::Identifier>(arg_identifier.get_value()));

		if (parse::token(source, token::Comma).is_ok()) source = parse::token(source, token::Comma).get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, token::RightParen);
	if (right_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(right_paren.get_errors());
	source = right_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_ok()) {
		source = expression.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression.get_value(), source.line, source.column, source.file);
		node->generic = true;
		return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto block = parse::block(source);
	if (block.is_ok()) {
		source = block.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, block.get_value(), source.line, source.column, source.file);
		node->generic = true;
		return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto expression_block = parse::expression_block(source);
	
	if (expression_block.is_ok()) {
		source = expression_block.get_source();
		auto node = std::make_shared<Ast::Function>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, expression_block.get_value(), source.line, source.column, source.file);
		node->generic = true;
		return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
	}

	return parse::Result<std::shared_ptr<Ast::Node>>(block.get_errors());
}

parse::Result<std::shared_ptr<Ast::Node>> parse::assignment(parse::Source source) {
	auto nonlocal = parse::token(source, token::NonLocal);
	if (nonlocal.is_ok()) {
		source = nonlocal.get_source();
	}

	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(identifier.get_errors());
	source = identifier.get_source();

	auto equal = parse::token(source, token::Equal);
	auto be = parse::token(source, token::Be);
	if (equal.is_error() && be.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(Errors{std::string("Errors: Expecting '=' or 'be'.")});
	if (equal.is_ok()) source = equal.get_source();
	if (be.is_ok()) source = be.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(expression.get_errors());
	source = expression.get_source();

	auto node = std::make_shared<Ast::Assignment>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), expression.get_value(), equal.is_ok(), nonlocal.is_ok(), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::return_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::Return);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto expression = parse::expression(source);
	if (expression.is_ok()) {
		source = expression.get_source();
		auto node = std::make_shared<Ast::Return>(expression.get_value(), source.line, source.column, source.file);
		return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
	}

	auto node = std::make_shared<Ast::Return>(nullptr, source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::break_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::Break);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto node = std::make_shared<Ast::Break>(source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::continue_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::Continue);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto node = std::make_shared<Ast::Continue>(source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::if_else_stmt(parse::Source source) {
	size_t indentation_level = source.column;

	auto keyword = parse::token(source, token::If);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
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
	if (aux.column == indentation_level) {

		// Match else
		auto keyword_else = parse::token(aux, token::Else);
		if (keyword_else.is_ok()) {
			aux = keyword_else.get_source();
			auto else_block = parse::block(aux);
			if (else_block.is_error()) return else_block.get_errors();
			aux = else_block.get_source();
			auto node = std::make_shared<Ast::IfElseStmt>(condition.get_value(), block.get_value(), else_block.get_value(), aux.line, aux.column, aux.file);
			return parse::Result<std::shared_ptr<Ast::Node>>(node, aux);
		}
	}

	// Return
	auto node = std::make_shared<Ast::IfElseStmt>(condition.get_value(), block.get_value(), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::while_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::While);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto condition = parse::expression(source);
	if (condition.is_error()) return condition.get_errors();
	source = condition.get_source();

	auto block = parse::block(source);
	if (block.is_error()) return block.get_errors();
	source = block.get_source();

	// Return
	auto node = std::make_shared<Ast::WhileStmt>(condition.get_value(), block.get_value(), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::use_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::Use);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto path = parse::string(source);
	if (path.is_error()) return path.get_errors();
	source = path.get_source();

	// Return
	auto node = std::make_shared<Ast::Use>(std::dynamic_pointer_cast<Ast::String>(path.get_value()), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Node>> parse::include_stmt(parse::Source source) {
	auto keyword = parse::token(source, token::Include);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Node>>(keyword.get_errors());
	source = keyword.get_source();

	auto path = parse::string(source);
	if (path.is_error()) return path.get_errors();
	source = path.get_source();

	// Return
	auto node = std::make_shared<Ast::Use>(std::dynamic_pointer_cast<Ast::String>(path.get_value()), source.line, source.column, source.file);
	node->include = true;
	return parse::Result<std::shared_ptr<Ast::Node>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::call(parse::Source source) {
	auto identifier = parse::identifier(source);
	if (identifier.is_error()) return identifier;
	source = identifier.get_source();

	auto left_paren = parse::token(source, token::LeftParen);
	if (left_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(left_paren.get_errors());
	source = left_paren.get_source();

	std::vector<std::shared_ptr<Ast::Expression>> args;
	while (current(source) != token::RightParen && current(source) != token::EndOfFile) {
		auto arg = parse::expression(source);
		if (arg.is_error()) return arg;
		source = arg.get_source();
		args.push_back(arg.get_value());

		if (parse::token(source, token::Comma).is_ok()) source = parse::token(source, token::Comma).get_source();
		else                                   break;
	}

	auto right_paren = parse::token(source, token::RightParen);
	if (right_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(right_paren.get_errors());
	source = right_paren.get_source();

	auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(identifier.get_value()), args, source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::expression(parse::Source source) {
	if (parse::if_else_expr(source).is_ok()) return parse::if_else_expr(source);
	return parse::not_expr(source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::if_else_expr(parse::Source source) {
	size_t indentation_level = source.column;

	auto keyword = parse::token(source, token::If);
	if (keyword.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(keyword.get_errors());
	source = keyword.get_source();

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
	if (aux.column == indentation_level) {
		// Match else
		auto keyword_else = parse::token(aux, token::Else);
		if (keyword_else.is_ok()) {
			aux = keyword_else.get_source();

			auto else_expression = parse::expression(aux);
			if (else_expression.is_error()) {
				else_expression = parse::expression_block(aux);
				if (else_expression.is_error()) return parse::expression(aux).get_errors();
			}
			aux = else_expression.get_source();
			
			auto node = std::make_shared<Ast::IfElseExpr>(condition.get_value(), expression.get_value(), else_expression.get_value(), aux.line, aux.column, aux.file);
			return parse::Result<std::shared_ptr<Ast::Expression>>(node, aux);
		}
	}

	// Return
	return parse::Result<std::shared_ptr<Ast::Expression>>(Errors{std::string("Expecting else branch in if/else expression")});
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::not_expr(parse::Source source) {
	auto op = parse::token(source, token::Not);
	if (op.is_ok()) {
		source = op.get_source();

		auto expression = parse::binary(source);
		if (expression.is_error()) return expression;
		source = expression.get_source();

		std::vector<std::shared_ptr<Ast::Expression>> args;
		args.push_back(expression.get_value());
		auto node = std::make_shared<Ast::Call>(std::make_shared<Ast::Identifier>(op.get_value().get_literal(), source.line, source.column, source.file), args, source.line, source.column, source.file);
		return parse::Result<std::shared_ptr<Ast::Expression>>(node, source);
	}

	return parse::binary(source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::binary(parse::Source source, int precedence) {
	static std::map<std::string, int> bin_op_precedence;
	bin_op_precedence["or"] = 1;
	bin_op_precedence["and"] = 2;
	bin_op_precedence["=="] = 3;
	bin_op_precedence["<"] = 4;
	bin_op_precedence["<="] = 4;
	bin_op_precedence[">"] = 4;
	bin_op_precedence[">="] = 4;
	bin_op_precedence["+"] = 5;
	bin_op_precedence["-"] = 5;
	bin_op_precedence["*"] = 6;
	bin_op_precedence["/"] = 6;
	bin_op_precedence["%"] = 6;

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
			left.value = std::make_shared<Ast::Call>(identifier, args, source.line, source.column, source.file);
		}

		return parse::Result<std::shared_ptr<Ast::Expression>>(left.get_value(), source);
	}
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::unary(parse::Source source) {
	auto op = parse::op(source);
	if (op.is_ok() && std::dynamic_pointer_cast<Ast::Identifier>(op.get_value())->value == "-") {
		source = op.get_source();

		auto expression = parse::unary(source);
		if (expression.is_error()) return expression;
		source = expression.get_source();

		std::vector<std::shared_ptr<Ast::Expression>> args;
		args.push_back(expression.get_value());
		auto node = std::make_shared<Ast::Call>(std::dynamic_pointer_cast<Ast::Identifier>(op.get_value()), args, source.line, source.column, source.file);
		return parse::Result<std::shared_ptr<Ast::Expression>>(node, source);
	}

	return parse::primary(source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::primary(parse::Source source) {
	if (parse::token(source, token::LeftParen).is_ok()) return parse::grouping(source);
	if (parse::number(source).is_ok())       return parse::number(source);
	if (parse::integer(source).is_ok())      return parse::integer(source);
	if (parse::call(source).is_ok())         return parse::call(source);
	if (parse::boolean(source).is_ok())      return parse::boolean(source);
	if (parse::identifier(source).is_ok())   return parse::identifier(source);
	return parse::Result<std::shared_ptr<Ast::Expression>>(Errors{errors::unexpected_character(source)}); // tested in test/errors/expecting_primary.dm
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::grouping(parse::Source source) {
	auto left_paren = parse::token(source, token::LeftParen);
	if (left_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(left_paren.get_errors());
	source = left_paren.get_source();

	auto expression = parse::expression(source);
	if (expression.is_error()) return expression;
	source = expression.get_source();

	auto right_paren = parse::token(source, token::RightParen);
	if (right_paren.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(right_paren.get_errors());
	source = right_paren.get_source();

	return parse::Result<std::shared_ptr<Ast::Expression>>(expression.get_value(), source);
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::number(parse::Source source) {
	auto result = parse::token(source, token::Float);
	if (result.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	double value = atof(result.get_value().get_literal().c_str());
	auto node = std::make_shared<Ast::Number>(value, source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::integer(parse::Source source) {
	auto result = parse::token(source, token::Integer);
	if (result.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(result.get_errors());
	source = result.get_source();
	char* ptr;
	int64_t value = strtol(result.get_value().get_literal().c_str(), &ptr, 10);
	auto node = std::make_shared<Ast::Integer>(value, source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, result.get_source());
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::boolean(parse::Source source) {
	if (current(source) != token::True && current(source) != token::False) return parse::Result<std::shared_ptr<Ast::Expression>>(Errors{Error("Error: Expecting true or false values")});
	auto node = std::make_shared<Ast::Boolean>(current(source) == token::False ? false : true, source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, advance(source));
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::identifier(parse::Source source) {
	auto result = parse::token(source, token::Identifier);
	if (result.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(result.get_errors());
	auto node = std::make_shared<Ast::Identifier>(result.get_value().get_literal(), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, advance(source));
}

parse::Result<std::shared_ptr<Ast::Expression>> parse::string(parse::Source source) {
	auto result = parse::token(source, token::String);
	if (result.is_error()) return parse::Result<std::shared_ptr<Ast::Expression>>(result.get_errors());
	auto node = std::make_shared<Ast::String>(result.get_value().get_literal(), source.line, source.column, source.file);
	return parse::Result<std::shared_ptr<Ast::Expression>>(node, advance(source));
}

parse::Result<std::shared_ptr<Ast::Node>> parse::op(parse::Source source) {
	const static std::vector<token::TokenVariant> operators = {
		token::Plus,
		token::Minus,
		token::Star,
		token::Slash,
		token::LessEqual,
		token::Less,
		token::Greater,
		token::GreaterEqual,
		token::EqualEqual,
		token::And,
		token::Or,
		token::Modulo
	};

	for (size_t i = 0; i < operators.size(); i++) {
		auto result = parse::token(source, operators[i]);
		if (result.is_ok()) {
			auto node = std::make_shared<Ast::Identifier>(result.get_value().get_literal(), source.line, source.column, source.file);
			return parse::Result<std::shared_ptr<Ast::Node>>(node, result.get_source());
		}
	}

	return parse::Result<std::shared_ptr<Ast::Node>>(Errors{Error("Error: Expecing operator\n")});
}

parse::Result<token::Token> parse::token(parse::Source source, token::TokenVariant token) {
	if (!at_end(source) && current(source) == token) {
		return parse::Result<token::Token>(current(source), advance(source));
	}
	else {
		return parse::Result<token::Token>(Errors{Error("Error: Unexpected token\n")});
	}
}