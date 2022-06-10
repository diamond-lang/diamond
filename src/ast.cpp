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

// FunctionPrototype
bool ast::FunctionPrototype::operator==(const FunctionPrototype &t) const {
	if (this->identifier == t.identifier && this->args.size() == t.args.size()) {
		for (size_t i = 0; i < this->args.size(); i++) {
			if (this->args[i] != t.args[i]) return false;
		}
		return this->return_type == t.return_type;
	}
	else {
		return false;
	}
}

bool ast::FunctionPrototype::operator!=(const FunctionPrototype &t) const {
	return !(t == *this);
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

bool ast::is_expression(Node* node) {
	switch (node->index()) {
		case Block: return false;
        case Function: return false;
        case Assignment: return false;
        case Return: return false;
        case Break: return false;
        case Continue: return false;
        case IfElse: {
			auto& if_else = std::get<IfElseNode>(*node);
			if (if_else.else_branch.has_value()
			&& is_expression(if_else.if_branch) && is_expression(if_else.else_branch.value())) {
				return true;
			}
			return false;
		}
        case While: return false;
        case Use: return false;
		case Include: return false;
        case Call: return true;
        case Float: return true;
        case Integer: return true;
        case Identifier: return true;
        case Boolean: return true;
        case String: return true;
		default: assert(false);
	}
	return false;
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

ast::Type get_concrete_type(ast::Type type, ast::PrintContext context) {
	if (context.concrete && type.is_type_variable()) {
		return context.type_bindings[type.to_str()];
	}
	else {
		return type;
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
				print(block.use_include_statements[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
			}
			for (size_t i = 0; i < block.statements.size(); i++) {
				bool is_last = block.functions.size() == 0 && i == block.statements.size() - 1;
				print(block.statements[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
			}
			for (size_t i = 0; i < block.functions.size(); i++) {
				if (!context.concrete) {
					bool is_last = i == block.functions.size() - 1;
					print((ast::Node*) block.functions[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
				}
				else {
					auto& function = *block.functions[i];
					for (size_t j = 0; j < function.specializations.size(); j++) {
						bool is_last = i == block.functions.size() - 1 && j == function.specializations.size() - 1;
						
						context.type_bindings = {};
						for (size_t k = 0; k < function.specializations[j].args.size(); k++) {
							auto type_variable = ast::get_type(function.args[k]).to_str();
							context.type_bindings[type_variable] = function.specializations[j].args[k];
						}
						context.type_bindings[function.return_type.to_str()] = function.specializations[j].return_type;
		
						print((ast::Node*) block.functions[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
					}
				}
			}
			break;
		}
			
		case Function: {
			auto& function = std::get<FunctionNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "function " << function.identifier->value << '(';
			for (size_t i = 0; i < function.args.size(); i++) {
				auto& arg_name = std::get<IdentifierNode>(*function.args[i]).value;
				std::cout << arg_name;

				auto& arg_type = std::get<IdentifierNode>(*function.args[i]).type;
				
				if (arg_type != Type("")) {
					std::cout << ": " << get_concrete_type(arg_type, context).to_str();
				}

				if (i != function.args.size() - 1) std::cout << ", ";
			}
			std::cout << ")";
			if (function.return_type != Type("")) {
				std::cout << ": " << get_concrete_type(function.return_type, context).to_str();
			}
			
			std::cout << "\n";
			if (!is_expression(function.body)) {
				print(function.body, context);
			}
			else {
				context.indent_level += 1;
				context.last.push_back(true);
				print(function.body, context);
				context.indent_level -= 1;
			}

			if (function.constraints.size() != 0 && !context.concrete) {
				put_indent_level(context.indent_level, context.last);
				std::cout << "where\n";
				for (size_t i = 0; i < function.constraints.size(); i++) {
					put_indent_level(context.indent_level + 1, i == (function.constraints.size() - 1) ? append(context.last, true) : append(context.last, false));
					std::cout << function.constraints[i].identifier << "(";
					for  (size_t j = 0; j < function.constraints[i].args.size(); j++) {
						std::cout << function.constraints[i].args[j].to_str();
						if (j != function.constraints[i].args.size() - 1) std::cout << ", ";
					}
					std::cout << "): " << function.constraints[i].return_type.to_str();
					std::cout << "\n";
				}
			}

			break;
		}
		
		case Assignment: {
			auto& assignment = std::get<AssignmentNode>(*node);

			print((ast::Node*) assignment.identifier, context);
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
			if (!is_expression(node)) {
				bool is_last = context.last[context.last.size() - 1];
				context.last.pop_back();

				bool has_else_block = if_else.else_branch ? true : false;
				put_indent_level(context.indent_level, append(context.last, is_last && !has_else_block));
				std::cout << "if" << '\n';
				print(if_else.condition, PrintContext{context.indent_level + 1, append(append(context.last, is_last && !has_else_block), false), context.concrete, context.type_bindings});

				print(if_else.if_branch, PrintContext{context.indent_level, append(context.last, is_last && !has_else_block), context.concrete, context.type_bindings});
			
				if (if_else.else_branch.has_value()) {
					put_indent_level(context.indent_level, append(context.last, is_last));
					std::cout << "else" << "\n";
					print(if_else.else_branch.value(), PrintContext{context.indent_level, append(context.last, is_last), context.concrete, context.type_bindings});
				}
			}
			else {
				bool is_last = true;
				if (context.last.size() > 0) {
					is_last = context.last[context.last.size() - 1];
					context.last.pop_back();
				}

				put_indent_level(context.indent_level, append(context.last, false));
				std::cout << "if";
				if (get_concrete_type(if_else.type, context) != Type("")) std::cout << ": " << get_concrete_type(if_else.type, context).to_str();
				std::cout << "\n";
				print(if_else.condition, PrintContext{context.indent_level + 1, append(append(context.last, false), false), context.concrete, context.type_bindings});
				print(if_else.if_branch, PrintContext{context.indent_level + 1, append(append(context.last, false), true), context.concrete, context.type_bindings});

				assert(if_else.else_branch.has_value());
				put_indent_level(context.indent_level, append(context.last, is_last));
				std::cout << "else" << "\n";
				print(if_else.else_branch.value(), PrintContext{context.indent_level + 1, append(append(context.last, is_last), true), context.concrete, context.type_bindings});
			}
				
			break;
		}

		case While: {
			auto& while_node = std::get<WhileNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "while" << '\n';
			print(while_node.condition, PrintContext{context.indent_level + 1, append(context.last, false), context.concrete, context.type_bindings});
			print(while_node.block, PrintContext{context.indent_level, context.last, context.concrete, context.type_bindings});
			break;
		}

		case Use: {
			auto path = std::get<UseNode>(*node).path;
			std::cout << "use \"" << path->value << "\"\n";
			break;
		}

		case Include: {
			auto path = std::get<IncludeNode>(*node).path;
			std::cout << "include \"" << path->value << "\"\n";
			break;
		}

		case Call: {
			auto& call = std::get<CallNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << call.identifier->value;
			if (call.type != Type("")) std::cout << ": " << get_concrete_type(call.type, context).to_str();
			std::cout << "\n";
			for (size_t i = 0; i < call.args.size(); i++) {
				print(call.args[i], PrintContext{context.indent_level + 1, append(context.last, i == call.args.size() - 1), context.concrete, context.type_bindings});
			}
			break;
		}

		case Float: {
			auto& float_node = std::get<FloatNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << float_node.value;
			if (float_node.type != Type("")) std::cout << ": " << get_concrete_type(float_node.type, context).to_str();
			std::cout << "\n";
			break;
		}

		case Integer: {
			auto& integer = std::get<IntegerNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << integer.value;
			if (integer.type != Type("")) std::cout << ": " << get_concrete_type(integer.type, context).to_str();
			std::cout << "\n";
			break;
		}

		case Identifier: {
			auto& identifier = std::get<IdentifierNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << identifier.value;
			if (identifier.type != Type("")) std::cout << ": " << get_concrete_type(identifier.type, context).to_str();
			std::cout << "\n";
			break;
		}

		case Boolean: {
			auto& boolean = std::get<BooleanNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << (boolean.value ? "true" : "false");
			if (boolean.type != Type("")) std::cout << ": " << get_concrete_type(boolean.type, context).to_str();
			std::cout << "\n";
			break;
		}

		case String: {
			auto& string = std::get<StringNode>(*node);

			put_indent_level(context.indent_level, context.last);
			std::cout << "\"" << string.value << "\"";
			if (string.type != Type("")) std::cout << ": " << get_concrete_type(string.type, context).to_str();
			std::cout << "\n";
			break;
		}

		default: assert(false);
	}
}