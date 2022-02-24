#include <algorithm>
#include <map>
#include <cstdlib>

#include "errors.hpp"
#include "parser.hpp"
#include "ast.hpp"

// Prototypes and definitions
// --------------------------
token::Token Parser::current() {
	assert(this->position < this->tokens.size());
	return this->tokens[this->position];
}

size_t Parser::current_indentation() {
	assert(this->indentation_level.size() > 0);
	return this->indentation_level[this->indentation_level.size() - 1];
}

void Parser::advance() {
	if (!this->at_end()) {
		this->position++;
	}
}

void Parser::advance_until_next_statement() {
	while (!this->at_end() && this->current() == token::NewLine) {
		this->advance();
	}
}

bool Parser::at_end() {
	return this->current() == token::EndOfFile;
}

bool Parser::match(std::vector<token::TokenVariant> tokens) {
	for (size_t i = 0; i < tokens.size(); i++) {
		if (this->position + i >= this->tokens.size() || this->tokens[this->position + i] != tokens[i]) {
			return false;
		}
	}
	return true;
}

Location Parser::location() {
	return Location(this->current().line, this->current().column, this->file);
}

// Parsing
// -------
#include <iostream>
Result<Ok, Error> Parser::parse_program() {
	auto result = this->parse_block();
	if (result.is_error()) return Error {};
	this->ast.program = result.get_value();
	return Ok {};
}

Result<size_t, Error> Parser::parse_block() {
	bool there_was_errors = false;

	// Create node
	Ast::BlockNode block = {this->current().line, this->current().column};

	// Advance until next statement
	this->advance_until_next_statement();

	// Set new indentation level
	if (this->indentation_level.size() == 0) {
		this->indentation_level.push_back(1);
	}
	else {
		size_t previous = this->current_indentation();
		this->indentation_level.push_back(this->current().column);
		if (previous >= this->current_indentation()) {
			this->errors.push_back(errors::expecting_new_indentation_level(this->location())); // tested in errors/expecting_new_indentation_level.dm
			return Error {};
		}
	}

	// Main loop, parses line by line
	while (!this->at_end()) {
		// Advance until next statement
		size_t backup = this->position;
		this->advance_until_next_statement();
		if (this->at_end()) break;

		// Check indentation
		if (this->current().column < this->current_indentation()) {
			this->position = backup;
			break;
		}
		else if (this->current().column > this->current_indentation()) {
			this->errors.push_back(errors::unexpected_indent(this->location())); // tested in test/errors/unexpected_indentation_1.dm and test/errors/unexpected_indentation_2.dm
			there_was_errors = true;
			while (!this->at_end() && this->current() != token::NewLine) this->advance(); // advances until new line
			continue;
		}

		// Parse statement
		auto result = this->parse_statement();
		if (result.is_ok()) {
			switch (this->ast.nodes[result.get_value()].index()) {
				case Ast::Function:
					block.functions.push_back(result.get_value());
					break;
				
				case Ast::Use:
				case Ast::Include:
					block.use_include_statements.push_back(result.get_value());
					break;
				
				default:
					block.statements.push_back(result.get_value());
			}

			if (!this->at_end() && this->current() != token::NewLine) {
				this->errors.push_back(errors::unexpected_character(this->location())); // tested in test/errors/expecting_line_ending.dm
				there_was_errors = true;
			}
		}
		else {
			there_was_errors = true;
		}

		// Advance until new line
		while (!this->at_end() && this->current() != token::NewLine) this->advance();
	}

	// Pop indentation level
	this->indentation_level.pop_back();

	// Return
	if (there_was_errors) return Error {};
	else {
		this->ast.nodes.push_back(block);
		return this->ast.nodes.size() - 1;
	}
}

Result<size_t, Error> Parser::parse_statement() {
	switch (this->current().variant) {
		case token::Function: return this->parse_function();
		case token::Return:   return this->parse_return_stmt();
		case token::If:       return this->parse_if_else();
		case token::While:    return this->parse_while_stmt();
		case token::Break:    return this->parse_break_stmt();
		case token::Continue: return this->parse_continue_stmt();
		case token::Use:      return this->parse_use_stmt();
		case token::Include:  return this->parse_include_stmt();
		case token::NonLocal: return this->parse_assignment();
		case token::Identifier:
			if (this->match({token::Identifier, token::Equal}))     return this->parse_assignment();
			if (this->match({token::Identifier, token::Be}))        return this->parse_assignment();
			if (this->match({token::Identifier, token::LeftParen})) return this->parse_call();
		default:
			break;
	}
	this->errors.push_back(errors::expecting_statement(this->location())); // tested in test/errors/expecting_statement.dm
	return Error {};
}

Result<size_t, Error> Parser::parse_function() {
	// Create node
	auto function = Ast::FunctionNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::Function);
	if (keyword.is_error()) return Error {};

	// Parse indentifier
	auto identifier = this->parse_identifier();
	if (identifier.is_error()) return identifier;
	function.identifier = identifier.get_value();

	// Parse left paren
	auto left_paren = this->parse_token(token::LeftParen);
	if (left_paren.is_error()) return Error {}; 

	// Parse args
	while (this->current() != token::RightParen && !this->at_end()) {
		auto arg = this->parse_identifier();
		if (arg.is_error()) return arg;
		function.args.push_back(arg.get_value());

		if (this->current() == token::Comma) this->advance();
		else                                 break;
	}

	// Parse right paren
	auto right_paren = this->parse_token(token::RightParen);
	if (right_paren.is_error()) return Error {}; 

	// Parse body
	switch (this->current().variant) {
		case token::NewLine: {
			auto block = this->parse_block();
			if (block.is_error()) return Error {};
			function.body = block.get_value();
			break;
		}
		
		default: { 
			auto expression = this->parse_expression();
			if (expression.is_error()) return Error {};
			function.body = expression.get_value();
		}
	}

	this->ast.nodes.push_back(function);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_assignment() {
	// Create node
	auto assignment = Ast::AssignmentNode {this->current().line, this->current().column};

	// Parse nonlocal
	if (this->current() == token::NonLocal) {	
		assignment.nonlocal = true;
		this->advance();
	}

	// Parse identifier
	auto identifier = this->parse_identifier();
	if (identifier.is_error()) return Error {};
	assignment.identifier = identifier.get_value();

	// Parse equal or be
	switch (this->current().variant) {
		case token::Equal:
			assignment.is_mutable = true;
			break;
		
		case token::Be:
			assignment.is_mutable = false;
			break;
		
		default:
			this->errors.push_back(Error("Error: Expecting token Equal or Be"));
			return Error {};
	}
	this->advance();

	// Parse expression
	auto expression = this->parse_expression();
	if (expression.is_error()) return expression;
	assignment.expression = expression.get_value();

	this->ast.nodes.push_back(assignment);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_return_stmt() {
	// Create node
	auto return_stmt = Ast::ReturnNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::Return);
	if (keyword.is_error()) return Error {};

	// Parse expression
	if (!this->at_end() && this->current() != token::NewLine) {
		auto expression = this->parse_expression();
		if (expression.is_error()) return expression;
		return_stmt.expression = expression.get_value();
	}

	this->ast.nodes.push_back(return_stmt);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_break_stmt() {
	// Create node
	auto break_node = Ast::BreakNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::Break);
	if (keyword.is_error()) return Error {};

	this->ast.nodes.push_back(break_node);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_continue_stmt() {
	// Create node
	auto continue_node = Ast::ContinueNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::Continue);
	if (keyword.is_error()) return Error {};

	this->ast.nodes.push_back(continue_node);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_if_else() {
	size_t indentation_level = this->current().column;

	// Create node
	auto if_else = Ast::IfElseNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::If);
	if (keyword.is_error()) return Error {};

	// Parse condition
	auto condition = this->parse_expression();
	if (condition.is_error()) return condition;
	if_else.condition = condition.get_value();

	// Parse if branch
	switch (this->current().variant) {
		case token::NewLine: {
			auto block = this->parse_block();
			if (block.is_error()) return Error {};
			if_else.if_branch = block.get_value();
			break;
		}
		
		default: { 
			auto expression = this->parse_expression();
			if (expression.is_error()) return Error {};
			if_else.if_branch = expression.get_value();
		}
	}

	// Adance until new statement
	this->advance_until_next_statement();

	// Match indentation
	if (this->current().column == indentation_level && this->current() == token::Else) {
		this->advance();

		// Parse else branch
		switch (this->current().variant) {
			case token::NewLine: {
				auto block = this->parse_block();
				if (block.is_error()) return Error {};
				if_else.else_branch = block.get_value();
				break;
			}
			
			default: { 
				auto expression = this->parse_expression();
				if (expression.is_error()) return Error {};
				if_else.else_branch = expression.get_value();
			}
		}
	}

	this->ast.nodes.push_back(if_else);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_while_stmt() {
	// Create node
	auto while_stmt = Ast::WhileNode {this->current().line, this->current().column};
	
	// Parse keyword
	auto keyword = this->parse_token(token::While);
	if (keyword.is_error()) return Error {};

	// Parse condition
	auto condition = this->parse_expression();
	if (condition.is_error()) return condition;
	while_stmt.condition = condition.get_value();

	// Parse block
	auto block = this->parse_block();
	if (block.is_error()) return Error {};
	while_stmt.block = block.get_value();

	// Return
	this->ast.nodes.push_back(while_stmt);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_use_stmt() {
	// Create node
	auto use_stmt = Ast::UseNode {this->current().line, this->current().column};
	
	// Parse keyword
	auto keyword = this->parse_token(token::Use);
	if (keyword.is_error()) return Error {};

	// Parse path
	auto path = this->parse_string();
	if (path.is_error()) return path;
	use_stmt.path = path.get_value();

	this->ast.nodes.push_back(use_stmt);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_include_stmt() {
	// Create node
	auto include_stmt = Ast::IncludeNode {this->current().line, this->current().column};

	// Parse keyword
	auto keyword = this->parse_token(token::Include);
	if (keyword.is_error()) return Error {};

	// Parse path
	auto path = this->parse_string();
	if (path.is_error()) return path;
	include_stmt.path = path.get_value();

	// Return
	this->ast.nodes.push_back(include_stmt);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_call() {
	// Create node
	auto call = Ast::CallNode {this->current().line, this->current().column};

	// Parse indentifier
	auto identifier = this->parse_identifier();
	if (identifier.is_error()) return identifier;
	call.identifier = identifier.get_value();

	// Parse left paren
	auto left_paren = this->parse_token(token::LeftParen);
	if (left_paren.is_error()) return Error {}; 

	// Parse args
	while (this->current() != token::RightParen && !this->at_end()) {
		auto arg = this->parse_expression();
		if (arg.is_error()) return arg;
		call.args.push_back(arg.get_value());

		if (this->current() == token::Comma) this->advance();
		else                                 break;
	}

	// Parse right paren
	auto right_paren = this->parse_token(token::RightParen);
	if (right_paren.is_error()) return Error {}; 

	this->ast.nodes.push_back(call);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_expression() {
	switch (this->current().variant) {
		case token::If:  return this->parse_if_else();
		case token::Not: return this->parse_not_expr();
		default:         return this->parse_binary();
	}
}

Result<size_t, Error> Parser::parse_not_expr() {
	// Create node
	auto call = Ast::CallNode {this->current().line, this->current().column};

	// Parse indentifier
	auto identifier = this->parse_identifier(token::Not);
	if (identifier.is_error()) return identifier;
	call.identifier = identifier.get_value();

	// Parse expression
	auto expression = this->parse_expression();
	if (expression.is_error()) return expression;
	call.args.push_back(expression.get_value());

	this->ast.nodes.push_back(call);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_binary(int precedence) {
	static std::map<token::TokenVariant, int> operators;
	operators[token::Or] = 1;
	operators[token::And] = 2;
	operators[token::EqualEqual] = 3;
	operators[token::Less] = 4;
	operators[token::LessEqual] = 4;
	operators[token::Greater] = 4;
	operators[token::GreaterEqual] = 4;
	operators[token::Plus] = 5;
	operators[token::Minus] = 5;
	operators[token::Star] = 6;
	operators[token::Slash] = 6;
	operators[token::Modulo] = 6;

	if (precedence > std::max_element(operators.begin(), operators.end(), [] (auto a, auto b) { return a.second < b.second;})->second) {
		return this->parse_primary();
	}
	else {
		// Parse left
		auto left = this->parse_binary(precedence + 1);
		if (left.is_error()) return left;

		while (true) {
			// Create node
			auto call = Ast::CallNode {this->current().line, this->current().column};
	
			// Parse operator
			auto op = this->current().variant;
			if (operators.find(op) == operators.end() || operators[op] != precedence) break;

			auto identifier = this->parse_identifier(op);
			if (identifier.is_error()) return identifier;

			// Parse right
			auto right = this->parse_binary(precedence + 1);
			if (right.is_error()) return right;

			call.identifier = identifier.get_value();
			call.args.push_back(left.get_value());
			call.args.push_back(right.get_value());
			this->ast.nodes.push_back(call);
			left = this->ast.nodes.size() - 1;
		}

		return left;
	}
}

Result<size_t, Error> Parser::parse_negation() {
	// Create node
	auto call = Ast::CallNode {this->current().line, this->current().column};

	// Parse indentifier
	auto identifier = this->parse_identifier(token::Minus);
	if (identifier.is_error()) return identifier;
	call.identifier = identifier.get_value();

	// Parse expression
	auto expression = this->parse_primary();
	if (expression.is_error()) return expression;
	call.args.push_back(expression.get_value());

	this->ast.nodes.push_back(call);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_primary() {
	switch (this->current().variant) {
		case token::Minus:     return this->parse_negation();
		case token::LeftParen: return this->parse_grouping();
		case token::Float:     return this->parse_float();
		case token::Integer:   return this->parse_integer();
		case token::True:      return this->parse_boolean();
		case token::False:     return this->parse_boolean();
		case token::Identifier: {
			if (this->match({token::Identifier, token::LeftParen})) return this->parse_call();
			else                                                    return this->parse_identifier();
		}
		default: break;
	}
	this->errors.push_back(errors::unexpected_character(this->location())); // tested in test/errors/expecting_primary.dm
	return Error {};
}

Result<size_t, Error> Parser::parse_grouping() {
	// Parse left paren
	auto left_paren = this->parse_token(token::LeftParen);
	if (left_paren.is_error()) return Error {}; 

	// Parse expression
	auto expression = this->parse_expression();
	if (expression.is_error()) return expression;

	// Parse right paren
	auto right_paren = this->parse_token(token::RightParen);
	if (right_paren.is_error()) return Error {};

	return expression.get_value();
}

Result<size_t, Error> Parser::parse_float() {
	// Create node
	auto float_node = Ast::FloatNode {this->current().line, this->current().column};
	
	// Parse float 
	if (this->current() != token::Float) assert(false);
	float_node.value = atof(this->current().get_literal().c_str());	

	this->advance();
	this->ast.nodes.push_back(float_node);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_integer() {
	// Create node
	auto integer = Ast::IntegerNode {this->current().line, this->current().column};

	// Parse integer
	if (this->current() != token::Integer) assert(false);
	char* ptr;
	integer.value = strtol(this->current().get_literal().c_str(), &ptr, 10);	

	this->advance();
	this->ast.nodes.push_back(integer);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_boolean() {
	// Create node
	auto boolean = Ast::BooleanNode {this->current().line, this->current().column};

	// Parse boolean
	if (this->current() != token::True && this->current() != token::False) assert(false);
	boolean.value = this->current() == token::True;

	this->advance();
	this->ast.nodes.push_back(boolean);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_identifier() {
	return this->parse_identifier(token::Identifier);
}

Result<size_t, Error> Parser::parse_identifier(token::TokenVariant token) {
	// Create node
	auto identifier = Ast::IdentifierNode {this->current().line, this->current().column};

	// Parse identifier
	auto result = this->parse_token(token);
	if (result.is_error()) return Error {};
	identifier.value = result.get_value().get_literal();

	this->ast.nodes.push_back(identifier);
	return this->ast.nodes.size() - 1;
}

Result<size_t, Error> Parser::parse_string() {
	// Create node
	auto string = Ast::StringNode {this->current().line, this->current().column};

	// Parse string
	auto result = this->parse_token(token::String);
	if (result.is_error()) return Error {};
	string.value = result.get_value().get_literal();

	this->advance();
	this->ast.nodes.push_back(string);
	return this->ast.nodes.size() - 1;
}

Result<token::Token, Error> Parser::parse_token(token::TokenVariant token) {
	if (this->current() != token) {
		this->errors.push_back(errors::unexpected_character(this->location()));
		return Error {};
	}
	auto curr = this->current();
	this->advance();
	return curr;
}