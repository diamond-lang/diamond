#include "ast.hpp"

#include <cassert>
#include <iostream>

// Type
// ----
bool Type::operator==(const Type &t) const {
	if (this->name == t.name && this->parameters.size() == t.parameters.size()) {
		for (size_t i = 0; i < this->parameters.size(); i++) {
			if (this->parameters[i] != t.parameters[i]) {
				return false;
			}
		}
		return true;
	}
	else {
		return false;
	}
}

bool Type::operator!=(const Type &t) const {
	return !(t == *this);
}

std::string Type::to_str(std::string output) const {
	if (this->parameters.size() == 0) {
		output += this->name;
	}
	else {
		output += "(";
		output += this->name;
		output += " of ";
		output += this->parameters[0].to_str(output);
		for (size_t i = 1; i < this->parameters.size(); i++) {
			output += ", ";
			output += this->parameters[0].to_str(output);
		}
		output += ")";
	}
	return output;
}

bool Type::is_type_variable() const {
	std::string str = this->to_str();
	if (str.size() > 0 && str[0] == '$') return true;
	else                                 return false;
}

// Print
// -----
std::vector<bool> append(std::vector<bool> vec, bool val) {
	vec.push_back(val);
	return vec;
}

void put_indent_level(size_t indent_level, std::vector<bool> last) {
	if (indent_level == 0) return;
	for (int i = 0; i < indent_level - 1; i++) {
		if (last[i]) {
			std::cout << "   ";
		} else {
			std::cout << "│  ";
		}
	}
	if (last[indent_level - 1]) {
		std::cout << "└──"; 
	} else {
		std::cout << "├──";
	}
}


void Ast::print(const Ast& ast, size_t indent_level, std::vector<bool> last, bool concrete) {
	std::cout << "program\n";
	print(ast, ast.program, indent_level, last, concrete);
}

void Ast::print_with_concrete_types(const Ast& ast, size_t node, size_t indent_level, std::vector<bool> last) {
	print(ast, node, indent_level, last, true);
}

void Ast::print(const Ast& ast, size_t node, size_t indent_level, std::vector<bool> last, bool concrete) {
	switch (ast.nodes[node].index()) {
		case Block: {
			auto& statements = std::get<BlockNode>(ast.nodes[node]).statements;
			auto& functions = std::get<BlockNode>(ast.nodes[node]).functions;
			auto& use_include_statements = std::get<BlockNode>(ast.nodes[node]).use_include_statements;

			for (size_t i = 0; i < use_include_statements.size(); i++) {
				print(ast, use_include_statements[i], indent_level + 1,  append(last, (statements.size() == 0 && functions.size() == 0 && i == use_include_statements.size() - 1)), concrete);
			}
			for (size_t i = 0; i < statements.size(); i++) {
				print(ast, statements[i], indent_level + 1, append(last, functions.size() == 0 && i == statements.size() - 1), concrete);
			}
			for (size_t i = 0; i < functions.size(); i++) {
				if (!concrete) {
					print(ast, functions[i], indent_level + 1, append(last, i == functions.size() - 1), concrete);
				}
				else {
					auto& specializations = std::get<FunctionNode>(ast.nodes[functions[i]]).specializations;
					for (size_t j = 0; j < specializations.size(); j++) {
						bool is_last = i == functions.size() - 1 && j == specializations.size() - 1;
						print(ast, specializations[j], indent_level + 1, append(last, is_last), concrete);
					}
				}
			}
			break;
		}
			
		case Function: {
			auto& identifier = std::get<FunctionNode>(ast.nodes[node]).identifier;
			auto& args = std::get<FunctionNode>(ast.nodes[node]).args;
			auto& return_type = std::get<FunctionNode>(ast.nodes[node]).return_type;
			auto body = std::get<FunctionNode>(ast.nodes[node]).body;

			put_indent_level(indent_level, last);
			std::cout << "function " << std::get<IdentifierNode>(ast.nodes[identifier]).value << '(';
			for (size_t i = 0; i < args.size(); i++) {
				auto& arg_name = std::get<IdentifierNode>(ast.nodes[args[i]]).value;
				std::cout << arg_name;

				auto& arg_type = std::get<IdentifierNode>(ast.nodes[args[i]]).type;
				
				if (arg_type != Type("")) {
					std::cout << ": " << arg_type.to_str();
				}

				if (i != args.size() - 1) std::cout << ", ";
			}
			std::cout << ")";
			if (return_type != Type("")) {
				std::cout << ": " << return_type.to_str();
			}
			std::cout << "\n";
			print(ast, body, indent_level, last, concrete);
			break;
		}
		
		case FunctionSpecialization: {
			auto& identifier = std::get<FunctionSpecializationNode>(ast.nodes[node]).identifier;
			auto& args = std::get<FunctionSpecializationNode>(ast.nodes[node]).args;
			auto& return_type = std::get<FunctionSpecializationNode>(ast.nodes[node]).return_type;
			auto body = std::get<FunctionSpecializationNode>(ast.nodes[node]).body;

			put_indent_level(indent_level, last);
			std::cout << "function " << identifier << '(';
			for (size_t i = 0; i < args.size(); i++) {
				auto& arg_name = std::get<IdentifierNode>(ast.nodes[args[i]]).value;
				std::cout << arg_name;

				auto& arg_type = std::get<IdentifierNode>(ast.nodes[args[i]]).type;
				
				if (arg_type != Type("")) {
					std::cout << ": " << arg_type.to_str();
				}

				if (i != args.size() - 1) std::cout << ", ";
			}
			std::cout << ")";
			if (return_type != Type("")) {
				std::cout << ": " << return_type.to_str();
			}
			std::cout << "\n";
			print(ast, body, indent_level, last, concrete);
			break;
		}

		case Assignment: {
			auto& identifier = std::get<AssignmentNode>(ast.nodes[node]).identifier;
			auto expression = std::get<AssignmentNode>(ast.nodes[node]).expression;
			auto nonlocal = std::get<AssignmentNode>(ast.nodes[node]).nonlocal;
			auto is_mutable = std::get<AssignmentNode>(ast.nodes[node]).is_mutable;

			print(ast, identifier, indent_level, last);
			if (nonlocal) {
				put_indent_level(indent_level + 1, append(last, false));
				std::cout << "nonlocal\n";
			}
			put_indent_level(indent_level + 1, append(last, false));
			std::cout << (is_mutable ? "=" : "be") << '\n';
			
			print(ast, expression, indent_level + 1, append(last, true), concrete);
			break;
		}

		case Return: {
			auto expression = std::get<ReturnNode>(ast.nodes[node]).expression;
			put_indent_level(indent_level, last);
			std::cout << "return" << '\n';
			if (expression.has_value()) {
				print(ast, expression.value(), indent_level + 1, append(last, true), concrete);
			}
			break;
		}
		
		case Break: {
			put_indent_level(indent_level, last);
			std::cout << "break" << '\n';
			break;
		}

		case Continue: {
			put_indent_level(indent_level, last);
			std::cout << "continue" << '\n';
			break;
		}
		
		case IfElse: {
			auto condition = std::get<IfElseNode>(ast.nodes[node]).condition;
			auto if_branch = std::get<IfElseNode>(ast.nodes[node]).if_branch;
			auto else_branch = std::get<IfElseNode>(ast.nodes[node]).else_branch;

			bool is_last = last[last.size() - 1];
			last.pop_back();

			bool has_else_block = else_branch ? true : false;
			put_indent_level(indent_level, append(last, is_last && !has_else_block));
			std::cout << "if" << '\n';
			print(ast, condition, indent_level + 1, append(append(last, is_last && !has_else_block), false), concrete);
			print(ast, if_branch, indent_level, append(last, is_last && !has_else_block), concrete);
		
			if (else_branch.has_value()) {
				put_indent_level(indent_level, append(last, is_last));
				std::cout << "else" << "\n";
				print(ast, else_branch.value(), indent_level, append(last, is_last));
			}
			break;
		}

		case While: {
			auto condition = std::get<WhileNode>(ast.nodes[node]).condition;
			auto block = std::get<WhileNode>(ast.nodes[node]).block;

			put_indent_level(indent_level, last);
			std::cout << "while" << '\n';
			print(ast, condition, indent_level + 1, append(last, false), concrete);
			print(ast, block, indent_level, last, concrete);
			break;
		}

		case Use: {
			auto path = std::get<StringNode>(ast.nodes[std::get<UseNode>(ast.nodes[node]).path]);
			std::cout << "use \"" << path.value << "\"\n";
			break;
		}

		case Include: {
			auto path = std::get<StringNode>(ast.nodes[std::get<IncludeNode>(ast.nodes[node]).path]);
			std::cout << "include \"" << path.value << "\"\n";
			break;
		}

		case Call: {
			auto& call = std::get<CallNode>(ast.nodes[node]);
			auto& identifier = std::get<IdentifierNode>(ast.nodes[call.identifier]);
			auto& args = call.args;
			auto& type = call.type;

			put_indent_level(indent_level, last);
			std::cout << identifier.value;
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			for (size_t i = 0; i < args.size(); i++) {
				print(ast, args[i], indent_level + 1, append(last, i == args.size() - 1), concrete);
			}
			break;
		}

		case Float: {
			auto value = std::get<FloatNode>(ast.nodes[node]).value;
			auto& type = std::get<FloatNode>(ast.nodes[node]).type;

			put_indent_level(indent_level, last);
			std::cout << value;
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			break;
		}

		case Integer: {
			auto value = std::get<IntegerNode>(ast.nodes[node]).value;
			auto& type = std::get<IntegerNode>(ast.nodes[node]).type;

			put_indent_level(indent_level, last);
			std::cout << value;
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			break;
		}

		case Identifier: {
			auto value = std::get<IdentifierNode>(ast.nodes[node]).value;
			auto& type = std::get<IdentifierNode>(ast.nodes[node]).type;

			put_indent_level(indent_level, last);
			std::cout << value;
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			break;
		}

		case Boolean: {
			auto value = std::get<BooleanNode>(ast.nodes[node]).value;
			auto& type = std::get<BooleanNode>(ast.nodes[node]).type;

			put_indent_level(indent_level, last);
			std::cout << (value ? "true" : "false");
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			break;
		}

		case String: {
			auto value = std::get<StringNode>(ast.nodes[node]).value;
			auto& type = std::get<StringNode>(ast.nodes[node]).type;

			put_indent_level(indent_level, last);
			std::cout << "\"" << value << "\"";
			if (type != Type("")) std::cout << ": " << type.to_str();
			std::cout << "\n";
			break;
		}

		default: assert(false);
	}
}