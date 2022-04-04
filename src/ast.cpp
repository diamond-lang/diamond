#include "ast.hpp"

#include <cassert>
#include <iostream>

// Type
// ----
bool ast::Type::operator==(const Type &t) const {
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

bool ast::Type::operator!=(const Type &t) const {
	return !(t == *this);
}

std::string ast::Type::to_str(std::string output) const {
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

bool ast::Type::is_type_variable() const {
	std::string str = this->to_str();
	if (str.size() > 0 && str[0] == '$') return true;
	else                                 return false;
}

// Node
// ----
ast::Type ast::get_type(Node* node) {
	return std::visit([](const auto& variant) {return variant.type;}, *node);	
}

void ast::set_type(Node* node, Type type) {
	std::visit([type](auto& variant) {variant.type = type;}, *node);
}

std::vector<ast::Type> ast::get_types(std::vector<Node*> nodes) {
	std::vector<Type> types;
	for (size_t i = 0; i < nodes.size(); i++) {
		types.push_back(get_type(nodes[i]));
	}
	return types;
}

// Ast
// ---
size_t ast::Ast::capacity() {
	size_t result = 0;
	for (size_t i = 0; i < this->nodes.size(); i++) {
		result += this->initial_size * pow(this->growth_factor, i);
	}
	return result;
}

void ast::Ast::push_back(Node node) {
	if (this->size + 1 > this->capacity()) {
		Node* array = new Node[this->initial_size * static_cast<unsigned int>(pow(this->growth_factor, this->nodes.size()))];
		this->nodes.push_back(array);
	}

	this->size++;
	this->nodes[this->nodes.size() - 1][this->size - 1 - this->size_of_arrays_filled()] = node;
}

size_t ast::Ast::size_of_arrays_filled() {
	if (this->nodes.size() == 0) return 0;
	else {
		size_t size_of_arrays_filled = 0;
		for (size_t i = 0; i < this->nodes.size() - 1; i++) {
			size_of_arrays_filled += this->initial_size * static_cast<unsigned int>(pow(this->growth_factor, i));
		}
		return size_of_arrays_filled;
	}
}

ast::Node* ast::Ast::last_element() {
	if (this->size == 0) return nullptr;
	else                 return &this->nodes[this->nodes.size() - 1][this->size - 1 - this->size_of_arrays_filled()];
}

void ast::Ast::free() {
	for (size_t i = 0; i < this->nodes.size(); i++) {
		delete[] this->nodes[i];
	}
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


void ast::print(const Ast& ast, PrintContext context) {
	std::cout << "program\n";
	print(ast.program, context);
}

void ast::print_with_concrete_types(const Ast& ast, PrintContext context) {
	context.concrete = true;
	std::cout << "program\n";
	print_with_concrete_types(ast.program, context);
}

void ast::print_with_concrete_types(Node* node, PrintContext context) {
	context.concrete = true;
	print(node, context);
}

void ast::print(Node* node, PrintContext context) {
	switch (node->index()) {
		case Block: {
			auto& block = std::get<BlockNode>(*node);

			for (size_t i = 0; i < block.use_include_statements.size(); i++) {	
				bool is_last = block.statements.size() == 0 && block.functions.size() == 0 && i == block.use_include_statements.size() - 1;
				print(block.use_include_statements[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete});
			}
			for (size_t i = 0; i < block.statements.size(); i++) {
				bool is_last = block.functions.size() == 0 && i == block.statements.size() - 1;
				print(block.statements[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete});
			}
			for (size_t i = 0; i < block.functions.size(); i++) {
				if (!context.concrete) {
					bool is_last = i == block.functions.size() - 1;
					print(block.functions[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete});
				}
				else {
					std::cout << "To do\n";
				}
			}
			break;
		}
			
		case Function: {
			auto& function = std::get<FunctionNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "function " << std::get<IdentifierNode>(*function.identifier).value << '(';
			for (size_t i = 0; i < function.args.size(); i++) {
				auto& arg_name = std::get<IdentifierNode>(*function.args[i]).value;
				std::cout << arg_name;

				auto& arg_type = std::get<IdentifierNode>(*function.args[i]).type;
				
				if (arg_type != Type("")) {
					std::cout << ": " << arg_type.to_str();
				}

				if (i != function.args.size() - 1) std::cout << ", ";
			}
			std::cout << ")";
			if (function.return_type != Type("")) {
				std::cout << ": " << function.return_type.to_str();
			}
			std::cout << "\n";
			if (std::holds_alternative<BlockNode>(*function.body)) {
				print(function.body, context);
			}
			else {
				context.indent_level += 1;
				context.last.push_back(true);
				print(function.body, context);
			}
			break;
		}
		
		case Assignment: {
			auto& assignment = std::get<AssignmentNode>(*node);

			print(assignment.identifier, context);
			if (assignment.nonlocal) {
				put_indent_level(context.indent_level + 1, append(context.last, false));
				std::cout << "nonlocal\n";
			}
			put_indent_level(context.indent_level + 1, append(context.last, false));
			std::cout << (assignment.is_mutable ? "=" : "be") << '\n';
			
			context.indent_level += 1;
			context.last.push_back(true);
			print(assignment.expression, context);
			break;
		}

		case Return: {
			auto& return_node = std::get<ReturnNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "return" << '\n';
			if (return_node.expression.has_value()) {
				context.indent_level += 1;
				context.last.push_back(true);
				print(return_node.expression.value(), context);
			}
			break;
		}
		
		case Break: {
			put_indent_level(context.indent_level, context.last);
			std::cout << "break" << '\n';
			break;
		}

		case Continue: {
			put_indent_level(context.indent_level, context.last);
			std::cout << "continue" << '\n';
			break;
		}
		
		case IfElse: {
			auto& if_else = std::get<IfElseNode>(*node);

			bool is_last = context.last[context.last.size() - 1];
			context.last.pop_back();

			bool has_else_block = if_else.else_branch ? true : false;
			put_indent_level(context.indent_level, append(context.last, is_last && !has_else_block));
			std::cout << "if" << '\n';
			print(if_else.condition, PrintContext{context.indent_level + 1, append(append(context.last, is_last && !has_else_block), false), context.concrete});
			print(if_else.if_branch, PrintContext{context.indent_level, append(context.last, is_last && !has_else_block), context.concrete});
		
			if (if_else.else_branch.has_value()) {
				put_indent_level(context.indent_level, append(context.last, is_last));
				std::cout << "else" << "\n";
				print(if_else.else_branch.value(), PrintContext{context.indent_level, append(context.last, is_last), context.concrete});
			}
			break;
		}

		case While: {
			auto& while_node = std::get<WhileNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "while" << '\n';
			print(while_node.condition, PrintContext{context.indent_level + 1, append(context.last, false), context.concrete});
			print(while_node.block, PrintContext{context.indent_level, context.last, context.concrete});
			break;
		}

		case Use: {
			auto path = std::get<StringNode>(*std::get<UseNode>(*node).path);
			std::cout << "use \"" << path.value << "\"\n";
			break;
		}

		case Include: {
			auto path = std::get<StringNode>(*std::get<IncludeNode>(*node).path);
			std::cout << "include \"" << path.value << "\"\n";
			break;
		}

		case Call: {
			auto& call = std::get<CallNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << std::get<IdentifierNode>(*call.identifier).value;
			if (call.type != Type("")) std::cout << ": " << call.type.to_str();
			std::cout << "\n";
			for (size_t i = 0; i < call.args.size(); i++) {
				print(call.args[i], PrintContext{context.indent_level + 1, append(context.last, i == call.args.size() - 1), context.concrete});
			}
			break;
		}

		case Float: {
			auto& float_node = std::get<FloatNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << float_node.value;
			if (float_node.type != Type("")) std::cout << ": " << float_node.type.to_str();
			std::cout << "\n";
			break;
		}

		case Integer: {
			auto& integer = std::get<IntegerNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << integer.value;
			if (integer.type != Type("")) std::cout << ": " << integer.type.to_str();
			std::cout << "\n";
			break;
		}

		case Identifier: {
			auto& identifier = std::get<IdentifierNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << identifier.value;
			if (identifier.type != Type("")) std::cout << ": " << identifier.type.to_str();
			std::cout << "\n";
			break;
		}

		case Boolean: {
			auto& boolean = std::get<BooleanNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << (boolean.value ? "true" : "false");
			if (boolean.type != Type("")) std::cout << ": " << boolean.type.to_str();
			std::cout << "\n";
			break;
		}

		case String: {
			auto& string = std::get<StringNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "\"" << string.value << "\"";
			if (string.type != Type("")) std::cout << ": " << string.type.to_str();
			std::cout << "\n";
			break;
		}

		default: assert(false);
	}
}