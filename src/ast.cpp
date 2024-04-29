#include "ast.hpp"
#include "semantic/intrinsics.hpp"

#include <cassert>
#include <iostream>

// Interface
// ---------
bool ast::Interface::operator==(const Interface &interface) const {
    return interface.name == this->name;
}

bool ast::Interface::operator!=(const Interface &interface) const {
    return !(interface == *this);
}

ast::Type ast::Interface::get_default_type() {
    if (this->name== "Number") {
        return ast::Type("int64");
    }
    else if (this->name == "Float") {
        return ast::Type("float64");
    }
    else {
        assert(false);
        return ast::Type(ast::NoType{});
    }
}

bool ast::Interface::is_compatible_with(ast::Type type) {
    if (this->name== "Number") {
        return type.is_integer() || type.is_float();
    }
    else if (this->name == "Float") {
        return type.is_float();
    }
    else {
        assert(false);
        return false;
    }
}

// Field types
// -----------
std::vector<ast::FieldConstraint>::iterator ast::FieldTypes::begin() {
    return this->fields.begin();
}

std::vector<ast::FieldConstraint>::iterator ast::FieldTypes::end() {
    return this->fields.end();
}

std::size_t ast::FieldTypes::size() const {
    return this->fields.size();
}

std::vector<ast::FieldConstraint>::iterator ast::FieldTypes::find(std::string field_name) {
    for (auto field = this->begin(); field != this->fields.end(); field++) {
        if (field_name == field->name) {
            return field;
        }
    }
    return this->end();
}

ast::Type& ast::FieldTypes::operator[](const std::string& field_name) {
    for (size_t i = 0; i < this->fields.size(); i++) {
        if (field_name == this->fields[i].name) {
            return this->fields[i].type;
        }
    }
    
    this->fields.push_back(ast::FieldConstraint{field_name, ast::Type(ast::NoType{})});
    return this->fields[this->fields.size() - 1].type;
}

const ast::Type& ast::FieldTypes::operator[](const std::string& field_name) const {
    for (size_t i = 0; i < this->fields.size(); i++) {
        if (field_name == this->fields[i].name) {
            return this->fields[i].type;
        }
    }
    assert(false);
    return this->fields[0].type;
}

std::string ast::FieldTypes::to_str() const {
    std::string result = "{";
    for (size_t i = 0; i < this->fields.size(); i++) {
        result += this->fields[i].name + ": " + this->fields[i].type.to_str();
        if (i + 1 != this->fields.size()) result += ", ";
    }
    result += "}";
    return result;
}

// Type
// ----
ast::NoType& ast::Type::as_no_type() {
    return std::get<ast::NoType>(this->type);
}

ast::TypeVariable& ast::Type::as_type_variable() {
    return std::get<ast::TypeVariable>(this->type);
}

ast::FinalTypeVariable& ast::Type::as_final_type_variable() {
    return std::get<ast::FinalTypeVariable>(this->type);
}

ast::NominalType& ast::Type::as_nominal_type() {
    return std::get<ast::NominalType>(this->type);
}

ast::StructType& ast::Type::as_struct_type() {
    return std::get<ast::StructType>(this->type);
}

ast::NoType ast::Type::as_no_type() const {
    return std::get<ast::NoType>(this->type);
}

ast::TypeVariable ast::Type::as_type_variable() const {
    return std::get<ast::TypeVariable>(this->type);
}

ast::FinalTypeVariable ast::Type::as_final_type_variable() const {
    return std::get<ast::FinalTypeVariable>(this->type);
}

ast::NominalType ast::Type::as_nominal_type() const {
    return std::get<ast::NominalType>(this->type);
}

ast::StructType ast::Type::as_struct_type() const {
    return std::get<ast::StructType>(this->type);
}

bool ast::Type::operator==(const Type &t) const {
    if (this->type.index() != t.type.index()) return false;

    if (this->is_no_type()) {
        return true;
    }
    else if (this->is_type_variable()) {
        return std::get<ast::TypeVariable>(this->type).id == std::get<ast::TypeVariable>(t.type).id;
    }
    else if (this->is_final_type_variable()) {
        return std::get<ast::FinalTypeVariable>(this->type).id == std::get<ast::FinalTypeVariable>(t.type).id;
    }
    else if (this->is_nominal_type()) {
        if (std::get<ast::NominalType>(this->type).name == std::get<ast::NominalType>(t.type).name) {
            if (std::get<ast::NominalType>(this->type).parameters.size() == std::get<ast::NominalType>(t.type).parameters.size()) {
                for (size_t i = 0; i < std::get<ast::NominalType>(this->type).parameters.size(); i++) {
                    if (std::get<ast::NominalType>(this->type).parameters[i] != std::get<ast::NominalType>(t.type).parameters[i]) {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }
    else if (this->is_struct_type()) {
        for (auto field: this->as_struct_type().fields) {
            if (t.as_struct_type().fields.find(field.name) == t.as_struct_type().fields.end()) {
                return false;
            }

            if (t.as_struct_type().fields[field.name] != field.type) {
                return false;
            }
        }
        return true;
    }
    else {
        assert(false);
        return false;
    }
}

bool ast::Type::operator!=(const Type &t) const {
    return !(t == *this);
}

static std::string as_letter(size_t type_var) {
    char letters[] = "abcdefghijklmnopqrstuvwxyz";
    if (type_var > 25) {
        return letters[(type_var + 18) % 26] + std::to_string(1 + type_var / 26);
    }
    else {
        return std::string(1, letters[(type_var + 18) % 26]);
    }
}

std::string ast::Type::to_str() const {
    std::string output = "";
    if (this->is_no_type()) {
        output += "ast::Notype";
    }
    else if (this->is_type_variable()) {
        output += std::to_string(std::get<ast::TypeVariable>(this->type).id);

        return output;
    }
    else if (this->is_final_type_variable()) {
        output += std::get<ast::FinalTypeVariable>(this->type).id;
    }
    else if (this->is_nominal_type()) {
        if (std::get<ast::NominalType>(this->type).parameters.size() == 0) {
            output += std::get<ast::NominalType>(this->type).name;
        }
        else {
            output += std::get<ast::NominalType>(this->type).name;
            output += "[";
            output += std::get<ast::NominalType>(this->type).parameters[0].to_str();
            for (size_t i = 1; i < std::get<ast::NominalType>(this->type).parameters.size(); i++) {
                output += ", ";
                output += std::get<ast::NominalType>(this->type).parameters[i].to_str();
            }
            output += "]";
        }
    }
    else if (this->is_struct_type()) {
        output += "struct {";
        
        auto fields = this->as_struct_type().fields;
        for(auto field = fields.begin(); field != fields.end(); field++) {
            output += field->name + ": " + field->type.to_str();

            if (std::next(field) != this->as_struct_type().fields.end()
            || this->as_struct_type().open) {
                output += ", ";
            }
        }

        if (this->as_struct_type().open) {
            output += "...";
        }

        output += "}";
    }
    return output;
}

std::string ast::TypeParameter::to_str() {
    assert(this->type.is_final_type_variable());
    std::string output = this->type.to_str();

    if (this->interface.has_value()
    ||  this->field_constraints.size() > 0) {
        output += ": ";
    }

    if (this->interface.has_value()) {
        output += this->interface.value().name;
    }
    else if (this->field_constraints.size() > 0) {
        output += "{";
        auto fields = this->field_constraints;
        for(auto field = fields.begin(); field != fields.end(); field++) {
            output += field->name + ": " + field->type.to_str() + ", ";

        }

        output += "...}";
    }

    return output;
}

static bool is_number(std::string str) {
    for (size_t i = 0; i < str.size(); i++) {
        if (!isdigit((int)str[i])) return false;
    }
    return true;
}

bool ast::Type::is_no_type() const {
    return this->type.index() == ast::NoTypeVariant;
}

bool ast::Type::is_type_variable() const {
    return this->type.index() == ast::TypeVariableVariant;
}

bool ast::Type::is_final_type_variable() const {
    return this->type.index() == ast::FinalTypeVariableVariant;
}

bool ast::Type::is_nominal_type() const {
    return this->type.index() == ast::NominalTypeVariant;
}

bool ast::Type::is_struct_type() const {
    return this->type.index() == ast::NominalTypeVariant && this->as_nominal_type().type_definition;
}

bool ast::Type::is_structural_struct_type() const {
    return this->type.index() == ast::StructTypeVariant;
}

bool ast::Type::is_concrete() const {
    if (this->is_no_type()) {
        return false;
    }

    if (this->is_type_variable()) {
        return false;
    }

    if (this->is_final_type_variable()) {
        return false;
    }

    if (this->is_nominal_type()) {
        return ast::types_are_concrete(std::get<ast::NominalType>(this->type).parameters);
    }

    if (this->is_struct_type()) {
        for (auto field: this->as_struct_type().fields) {
            if (!field.type.is_concrete()) {
                return false;
            }
        }

        return true;
    }

    return true;
}

bool ast::Type::is_integer() const {
    if (*this == ast::Type("int64"))      return true;
    else if (*this == ast::Type("int32")) return true;
    else if (*this == ast::Type("int16")) return true;
    else if (*this == ast::Type("int8"))  return true;
    else                                  return false;
}

bool ast::Type::is_float() const {
    if (*this == ast::Type("float64"))      return true;
    else if (*this == ast::Type("float32")) return true;
    else                                    return false;
}

bool ast::Type::is_pointer() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    if (std::get<ast::NominalType>(this->type).name != "pointer") {
        return false;
    }
    
    return true;
}

bool ast::Type::is_array() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    if (this->as_nominal_type().name.size() < 5
    || this->as_nominal_type().name.substr(0, 5) != "array") {
        return false;
    }

    if (this->as_nominal_type().name.size() > 5
    && !is_number(this->as_nominal_type().name.substr(5, this->as_nominal_type().name.size()))) {
        return false;
    }
    
    return true;
}

bool ast::Type::is_builtin_type() const {
    return this->is_pointer()
        || this->is_array()
        || (this->is_nominal_type() && primitive_types.contains(this->as_nominal_type().name));
}

size_t ast::Type::get_array_size() const {
    assert(this->is_array());
    assert(this->as_nominal_type().name.size() > 5);
    return stoi(std::get<ast::NominalType>(this->type).name.substr(5, this->as_nominal_type().name.size()));
}

// Add hash struct for ast::Type to be able to use ast::Type as keys of std::unordered_map
std::size_t std::hash<ast::Type>::operator()(const ast::Type& type) const {
    return std::hash<std::string>()(type.to_str());
}

// CallInCallStack
bool ast::CallInCallStack::operator==(const CallInCallStack &t) const {
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

bool ast::CallInCallStack::operator!=(const CallInCallStack &t) const {
    return !(t == *this);
}

// Node
// ----
ast::Type ast::get_type(Node* node) {
    return std::visit([](const auto& variant) {
        return variant.type;
    }, *node);
}

ast::Type ast::get_concrete_type(Node* node, std::unordered_map<std::string, Type>& type_bindings) {
    return ast::get_concrete_type(ast::get_type(node), type_bindings);
}

ast::Type ast::get_concrete_type(Type type, std::unordered_map<std::string, Type>& type_bindings) {
    if (type.is_final_type_variable()) {
        if (type_bindings.find(type.as_final_type_variable().id) != type_bindings.end()) {
            type = type_bindings[type.as_final_type_variable().id];
        }
        else {
            std::cout << "unknown type: " << type.to_str() << "\n";
            assert(false);
        }
    }
    if (!type.is_concrete()) {
        if (type.is_nominal_type()) {
            for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
                type.as_nominal_type().parameters[i] = ast::get_concrete_type(type.as_nominal_type().parameters[i], type_bindings);
            }
        }
        else if (type.is_struct_type()) {
            std::cout << "WAHT ? " << type.to_str() << "\n";
            assert(false);
        }
    }
    return type;
}

void ast::set_type(Node* node, Type type) {
    std::visit([type](auto& variant) {variant.type = type;}, *node);
}

std::vector<ast::Type> ast::get_types(std::vector<CallArgumentNode*> nodes) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(get_type(nodes[i]->expression));
    }
    return types;
}

std::vector<ast::Type> ast::get_types(std::vector<FunctionArgumentNode*> nodes) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(nodes[i]->type);
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
        case FieldAssignment: return false;
        case DereferenceAssignment: return false;
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

bool ast::has_type_variables(std::vector<ast::Type> types) {
    for (auto& type: types) {
        if (type.is_type_variable()) return true;
    }
    return false;
}

bool ast::types_are_concrete(std::vector<ast::Type> types) {
    for (auto& type: types) {
        if (!type.is_concrete()) return false;
    }
    return true;
}

bool ast::is_expression(Node* node) {
    switch (node->index()) {
        case Block: return false;
        case Function: return false;
        case Assignment: return false;
        case FieldAssignment: return false;
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

ast::Type ast::get_concrete_type_or_type_variable(ast::Type type, ast::PrintContext context) {
    if (context.concrete) {
        return ast::get_concrete_type(type, context.type_bindings);
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

        case FunctionArgument: {
            assert(false);
            break;
        }

        case Function: {
            auto& function = std::get<FunctionNode>(*node);
            bool last = true;
            if (context.last.size() != 0) {
                last = context.last[context.last.size() - 1];
                context.last.pop_back();
            }

            put_indent_level(context.indent_level, append(context.last, last));
            if (function.is_extern) {
                std::cout << "extern " << function.identifier->value << '(';
            }
            else {
                std::cout << "function " << function.identifier->value;

                if (function.type_parameters.size() > 0 && !context.concrete) {
                    std::cout << '[';
                    for (size_t i = 0; i < function.type_parameters.size(); i++) {
                        std::cout << function.type_parameters[i].to_str();
                        if (i + 1 != function.type_parameters.size()) std::cout << ", ";
                    }
                    std::cout << ']';
                }

                std::cout << '(';
            }
            for (size_t i = 0; i < function.args.size(); i++) {
                auto& arg_name = function.args[i]->identifier->value;
                if (function.args[i]->is_mutable) {
                    std::cout << "mut ";
                }
                std::cout << arg_name;

                auto& arg_type = function.args[i]->type;

                if (arg_type != Type(ast::NoType{})) {
                    std::cout << ": " << get_concrete_type_or_type_variable(arg_type, context).to_str();
                }

                if (i != function.args.size() - 1) std::cout << ", ";
            }
            std::cout << ")";
            if (function.return_type != Type(ast::NoType{})) {
                std::cout << ": " << get_concrete_type_or_type_variable(function.return_type, context).to_str();
            }
            std::cout << "\n";

            if (function.is_extern) {
                return;
            }

            if (!is_expression(function.body)) {
                context.last.push_back(last);
                print(function.body, context);
                context.last.pop_back();
            }
            else {
                context.indent_level += 1;
                context.last.push_back(last);
                context.last.push_back(true);
                print(function.body, context);
                context.indent_level -= 1;
                context.last.pop_back();
                context.last.pop_back();
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

        case Declaration: {
            auto& declaration = std::get<Declaration>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << (declaration.is_mutable ? "=" : "be") << '\n';
            context.indent_level += 1;
            context.last.push_back(false);
            print((ast::Node*) declaration.identifier, context);
            context.last[context.last.size() - 1] = true;
            print(declaration.expression, context);
            break;
        }

        case Assignment: {
            auto& assignment = std::get<AssignmentNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << ":=" << '\n';
            context.indent_level += 1;
            context.last.push_back(false);
            print((ast::Node*) assignment.identifier, context);
            context.last[context.last.size() - 1] = true;
            print(assignment.expression, context);
            break;
        }

        case FieldAssignment: {
            auto& assignment = std::get<FieldAssignmentNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "=\n";
            context.indent_level += 1;
            context.last.push_back(false);
            print((ast::Node*) assignment.identifier, context);
            context.last[context.last.size() - 1] = true;
            print(assignment.expression, context);
            break;
        }

        case DereferenceAssignment: {
            auto& assignment = std::get<DereferenceAssignmentNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "=\n";
            context.indent_level += 1;
            context.last.push_back(false);
            print((ast::Node*) assignment.identifier, context);
            context.last[context.last.size() - 1] = true;
            print(assignment.expression, context);
            break;
        }

        case IndexAssignment: {
            auto& assignment = std::get<IndexAssignmentNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "=\n";
            context.indent_level += 1;
            context.last.push_back(false);
            print((ast::Node*) assignment.index_access, context);
            context.last[context.last.size() - 1] = true;
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
                if (get_concrete_type_or_type_variable(if_else.type, context) != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(if_else.type, context).to_str();
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

        case LinkWith: {
            assert(false);
            break;
        }

        case CallArgument: {
            auto& call_argument = std::get<CallArgumentNode>(*node);
            if (call_argument.identifier.has_value()) {
                put_indent_level(context.indent_level, context.last);
                if (call_argument.is_mutable) {
                    std::cout << "mut ";
                }
                std::cout << call_argument.identifier.value()->value << ":\n";
                print(call_argument.expression, PrintContext{context.indent_level + 1, append(context.last, true), context.concrete, context.type_bindings});
            }
            else {
                if (call_argument.is_mutable) {
                    put_indent_level(context.indent_level, context.last);
                    std::cout << "mut " << "\n";
                    print(call_argument.expression, PrintContext{context.indent_level + 1, append(context.last, true), context.concrete, context.type_bindings});
                }
                else {
                    print(call_argument.expression, PrintContext{context.indent_level, context.last, context.concrete, context.type_bindings});
                }
            }

            break;
        }

        case Call: {
            auto& call = std::get<CallNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << call.identifier->value;
            if (call.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(call.type, context).to_str();
            std::cout << "\n";
            for (size_t i = 0; i < call.args.size(); i++) {
                print((ast::Node*) call.args[i], PrintContext{context.indent_level + 1, append(context.last, i == call.args.size() - 1), context.concrete, context.type_bindings});
            }
            break;
        }

        case StructLiteral: {
            auto& struct_literal = std::get<StructLiteralNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << struct_literal.identifier->value << "{}";
            if (struct_literal.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(struct_literal.type, context).to_str();
            std::cout << "\n";
            for (auto it = struct_literal.fields.begin(); it != struct_literal.fields.end(); it++) {
                auto is_last = std::next(it) == struct_literal.fields.end();
                put_indent_level(context.indent_level + 1, append(context.last, is_last));
                std::cout << it->first->value << ":\n";
                print(it->second, PrintContext{context.indent_level + 2, append(append(context.last, is_last), true), context.concrete, context.type_bindings});
            }
            break;
        }

        case Float: {
            auto& float_node = std::get<FloatNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << float_node.value;
            if (float_node.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(float_node.type, context).to_str();
            std::cout << "\n";
            break;
        }

        case Integer: {
            auto& integer = std::get<IntegerNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << integer.value;
            if (integer.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(integer.type, context).to_str();
            std::cout << "\n";
            break;
        }

        case Identifier: {
            auto& identifier = std::get<IdentifierNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << identifier.value;
            if (identifier.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(identifier.type, context).to_str();
            std::cout << "\n";
            break;
        }

        case Boolean: {
            auto& boolean = std::get<BooleanNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << (boolean.value ? "true" : "false");
            if (boolean.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(boolean.type, context).to_str();
            std::cout << "\n";
            break;
        }

        case String: {
            auto& string = std::get<StringNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "\"";
            for (size_t i = 0; i < string.value.size(); i++) {
                if (string.value[i] == '\n') {
                    std::cout << "\\n";
                }
                else {
                    std::cout << string.value[i];
                }
            }
            std::cout << "\"";
            if (string.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(string.type, context).to_str();
            std::cout << "\n";
            break;
        }

        case Array: {
            auto& array = std::get<ArrayNode>(*node);
            bool is_last = context.last[context.last.size()];

            put_indent_level(context.indent_level, context.last);
            std::cout << "[]";
            if (array.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(array.type, context).to_str();
            std::cout << "\n";
            for (size_t i = 0; i < array.elements.size(); i++) {
                print(array.elements[i], PrintContext{context.indent_level + 1, append(context.last, i + 1 == array.elements.size()), context.concrete, context.type_bindings});
            }
            break;
        }

        case FieldAccess: {
            auto& field_access = std::get<FieldAccessNode>(*node);

            // There should be at least 1 identifiers in fields accessed. eg: circle.radius
            assert(field_access.fields_accessed.size() >= 1);

            print(field_access.accessed, context);

            std::vector<bool> last = append(context.last, true);
            for (size_t i = 0; i < field_access.fields_accessed.size(); i++) {
                put_indent_level(context.indent_level + i + 1, last);
                std::cout << field_access.fields_accessed[i]->value;
                if (field_access.fields_accessed[i]->type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(field_access.fields_accessed[i]->type, context).to_str();
                std::cout << "\n";
                last.push_back(true);
            }

            break;
        }

        case AddressOf: {
            auto& address_of = std::get<AddressOfNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "&";
            if (address_of.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(address_of.type, context).to_str();
            std::cout << "\n";

            context.indent_level += 1;
            context.last.push_back(true);
            ast::print(address_of.expression, context);

            break;
        }

        case Dereference: {
            auto& dereference = std::get<DereferenceNode>(*node);

            put_indent_level(context.indent_level, context.last);
            std::cout << "*";
            if (dereference.type != Type(ast::NoType{})) std::cout << ": " << get_concrete_type_or_type_variable(dereference.type, context).to_str();
            std::cout << "\n";

            context.indent_level += 1;
            context.last.push_back(true);
            ast::print(dereference.expression, context);

            break;
        }

        default: {
            std::cout << node->index() << "\n";
            assert(false);
        }
    }
}

bool ast::FunctionNode::typed_parameter_aready_added(ast::Type type) {
    for (auto type_parameter: this->type_parameters) {
        if (type_parameter.type == type) {
            return true;
        }
    }
    return false;
}

std::optional<ast::TypeParameter*> ast::FunctionNode::get_type_parameter(ast::Type type) {
    for (auto& type_parameter: this->type_parameters) {
        if (type_parameter.type == type) {
            return &type_parameter;
        }
    }
    return std::nullopt;
}