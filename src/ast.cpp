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

ast::Type ast::get_concrete_type(Node* node, std::unordered_map<std::string, Type>& type_bindings) {
    Type type_variable = get_type(node);
    if (type_variable.is_type_variable()) {
        assert(type_bindings.find(type_variable.to_str()) != type_bindings.end());
        return type_bindings[type_variable.to_str()];
    }
    return type_variable;
}

ast::Type ast::get_concrete_type(Type type_variable, std::unordered_map<std::string, Type>& type_bindings) {
    if (type_variable.is_type_variable()) {
        assert(type_bindings.find(type_variable.to_str()) != type_bindings.end());
        return type_bindings[type_variable.to_str()];
    }
    return type_variable;
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

std::vector<ast::Type> ast::get_types(std::vector<CallArgumentNode*> nodes) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(get_type(nodes[i]->expression));
    }
    return types;
}

std::vector<ast::Type> ast::get_concrete_types(std::vector<Node*> nodes, std::unordered_map<std::string, Type>& type_bindings) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(get_concrete_type(nodes[i], type_bindings));
    }
    return types;
}


std::vector<ast::Type> ast::get_concrete_types(std::vector<ast::Type> type_variables, std::unordered_map<std::string, Type>& type_bindings) {
    std::vector<Type> types;
    for (size_t i = 0; i < type_variables.size(); i++) {
        types.push_back(get_concrete_type(type_variables[i], type_bindings));
    }
    return types;
}

bool ast::could_be_expression(Node* node) {
    switch (node->index()) {
        case Block: {
            auto& block = std::get<BlockNode>(*node);
            if (block.statements.size() == 1
            && could_be_expression(block.statements[0])) return true;
            return false;
        }
        case Function: return false;
        case Assignment: return false;
        case Return: return false;
        case Break: return false;
        case Continue: return false;
        case IfElse: {
            auto& if_else = std::get<IfElseNode>(*node);
            if (if_else.else_branch.has_value()
            && could_be_expression(if_else.if_branch)
            && could_be_expression(if_else.else_branch.value())) {
                return true;
            }
            return false;
        }
        case While: return false;
        case Use: return false;
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

void ast::transform_to_expression(ast::Node*& node) {
    assert(could_be_expression(node));
    switch (node->index()) {
        case Block: {
            auto& block = std::get<BlockNode>(*node);
            node = block.statements[0];
            transform_to_expression(node);
            break;
        }
        case IfElse: {
            auto& if_else = std::get<IfElseNode>(*node);
            transform_to_expression(if_else.if_branch);
            transform_to_expression(if_else.else_branch.value());
            break;
        }
        case Call: break;
        case Float: break;
        case Integer: break;
        case Identifier: break;
        case Boolean: break;
        case String: break;
        default: assert(false);
    }
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

ast::Type ast::get_concrete_type(ast::Type type, ast::PrintContext context) {
    if (context.concrete && type.is_type_variable()) {
        return context.type_bindings[type.to_str()];
    }
    else {
        return type;
    }
}

void ast::print(const Ast& ast, PrintContext context) {
    std::cout << "program\n";
    print((ast::Node*) ast.program, context);
}

void ast::print_with_concrete_types(const Ast& ast, PrintContext context) {
    context.concrete = true;
    std::cout << "program\n";
    print_with_concrete_types((ast::Node*) ast.program, context);
}

void ast::print_with_concrete_types(Node* node, PrintContext context) {
    context.concrete = true;
    print(node, context);
}

void ast::print(Node* node, PrintContext context) {
    switch (node->index()) {
        case Block: {
            auto& block = std::get<BlockNode>(*node);

            // Put all nodes of block in a vector
            std::vector<Node*> nodes = {};
            for (size_t i = 0; i < block.use_statements.size(); i++) {
                nodes.push_back((Node*) block.use_statements[i]);
            }
            for (size_t i = 0; i < block.statements.size(); i++) {
                nodes.push_back(block.statements[i]);
            }
            for (size_t i = 0; i < block.types.size(); i++) {
                nodes.push_back((Node*) block.types[i]);
            }
            for (size_t i = 0; i < block.functions.size(); i++) {
                nodes.push_back((Node*) block.functions[i]);
            }

            // Print all nodes
            for (size_t i = 0; i < nodes.size(); i++) {
                if (!context.concrete || nodes[i]->index() != Function) {
                    bool is_last = i == nodes.size() - 1;
                    print(nodes[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
                }
                else {
                    auto function = (ast::FunctionNode*) nodes[i];
                    for (size_t j = 0; j < function->specializations.size(); j++) {

                        // Check if is last specialization
                        bool is_last = j == function->specializations.size() - 1;
                        for (size_t k = i + 1; k < nodes.size(); k++) {
                            if (((FunctionNode*)nodes[k])->specializations.size() != 0) {
                                is_last = false;
                                break;
                            }
                        }

                        context.type_bindings = function->specializations[j].type_bindings;
                        print((ast::Node*) nodes[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
                    }
                }
            }
            break;
        }

        case Function: {
            auto& function = std::get<FunctionNode>(*node);
            bool where_clause = !context.concrete && function.constraints.size() != 0;
            bool last = true;
            if (context.last.size() != 0) {
                last = context.last[context.last.size() - 1];
                context.last.pop_back();
            }

            put_indent_level(context.indent_level, append(context.last, last && !where_clause));
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
                context.last.push_back(last && !where_clause);
                print(function.body, context);
                context.last.pop_back();
            }
            else {
                context.indent_level += 1;
                context.last.push_back(last && !where_clause);
                context.last.push_back(true);
                print(function.body, context);
                context.indent_level -= 1;
                context.last.pop_back();
                context.last.pop_back();
            }

            if (function.constraints.size() != 0 && !context.concrete) {
                put_indent_level(context.indent_level, append(context.last, last));
                std::cout << "where\n";
                for (size_t i = 0; i < function.constraints.size(); i++) {
                    put_indent_level(context.indent_level + 1, i == (function.constraints.size() - 1) ? append(append(context.last, last), true) : append(append(context.last, last), false));
                    std::cout << function.constraints[i].identifier << "(";
                    for  (size_t j = 0; j < function.constraints[i].args.size(); j++) {
                        std::cout << function.constraints[i].args[j].to_str();
                        if (j != function.constraints[i].args.size() - 1) std::cout << ", ";
                    }
                    std::cout << ")";
                    if (function.constraints[i].return_type != ast::Type("")) {
                        std::cout << ": " << function.constraints[i].return_type.to_str();
                    }
                    std::cout << "\n";
                }
            }

            break;
        }

        case TypeDef: {
            auto& type = std::get<TypeNode>(*node);

            put_indent_level(context.indent_level, append(context.last, type.fields.size() == 0));
            std::cout << "type " << type.identifier->value << '\n';

            for (size_t i = 0; i < type.fields.size(); i++) {
                bool is_last = i == type.fields.size() - 1;
                print((Node*) type.fields[i], PrintContext{context.indent_level + 1, append(context.last, is_last), context.concrete, context.type_bindings});
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
            auto& use_node = std::get<UseNode>(*node);
            if (use_node.include) std::cout << "include";
            else                  std::cout << "use";
            std::cout << " \"" << use_node.path->value << "\"\n";
            break;
        }

        case CallArgument: {
            auto& call_argument = std::get<CallArgumentNode>(*node);
            if (call_argument.identifier.has_value()) {
                put_indent_level(context.indent_level, context.last);
                std::cout << call_argument.identifier.value()->value << ":\n";
                print(call_argument.expression, PrintContext{context.indent_level + 1, append(context.last, true), context.concrete, context.type_bindings});
            }
            else {
                print(call_argument.expression, PrintContext{context.indent_level, context.last, context.concrete, context.type_bindings});
            }

            break;
        }

        case Call: {
            auto& call = std::get<CallNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << call.identifier->value;
            if (call.type != Type("")) std::cout << ": " << get_concrete_type(call.type, context).to_str();
            std::cout << "\n";
            for (size_t i = 0; i < call.args.size(); i++) {
                print((ast::Node*) call.args[i], PrintContext{context.indent_level + 1, append(context.last, i == call.args.size() - 1), context.concrete, context.type_bindings});
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

        case FieldAccess: {
            auto& field_access = std::get<FieldAccessNode>(*node);

            // There should be at least 2 identifiers in fields accessed. eg: circle.radius
            assert(field_access.fields_accessed.size() >= 2);

            put_indent_level(context.indent_level, context.last);
            std::cout << field_access.fields_accessed[0]->value;
            for (size_t i = 1; i < field_access.fields_accessed.size(); i++) {
                std::cout << "." << field_access.fields_accessed[i]->value;
            }
            if (field_access.type != Type("")) std::cout << ": " << get_concrete_type(field_access.type, context).to_str();
            std::cout << "\n";
            break;
        }

        default: assert(false);
    }
}
