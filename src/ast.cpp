#include "ast.hpp"

#include <cassert>
#include <iostream>
#include <variant>

#include "semantic/intrinsics.hpp"
#include "shared.hpp"

// Interface
// ---------
bool ast::InterfaceType::operator==(const InterfaceType& interface) const {
    return interface.name == this->name;
}

bool ast::InterfaceType::operator!=(const InterfaceType& interface) const {
    return !(interface == *this);
}

ast::Type ast::InterfaceType::get_default_type() {
    if (this->name == "number") {
        return ast::Type("Int64");
    } else if (this->name == "float") {
        return ast::Type("Float64");
    } else {
        return ast::Type(ast::NoType{});
    }
}

ast::Type ast::get_default_type(Set<ast::InterfaceType> interface) {
    for (auto it : interface.elements) {
        if (!it.get_default_type().is_no_type()) {
            return it.get_default_type();
        }
    }
    assert(false);
    return ast::Type(ast::NoType{});
}

bool ast::InterfaceType::
    is_compatible_with(ast::Type type, ast::InterfaceNode* interface) {
    if (this->name == "number") {
        return type.is_integer() || type.is_float();
    } else if (this->name == "float") {
        return type.is_float();
    } else if (this->name == "pointer") {
        return type.is_pointer() || type.is_boxed();
    } else if (this->name == "type") {
        return type.is_struct_type();
    } else {
        assert(interface != nullptr);
        return interface->is_compatible_with(type);
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

std::size_t ast::FieldTypes::size() const { return this->fields.size(); }

std::vector<ast::FieldConstraint>::iterator ast::FieldTypes::find(
    std::string field_name
) {
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

    this->fields.push_back(
        ast::FieldConstraint{field_name, ast::Type(ast::NoType{})}
    );
    return this->fields[this->fields.size() - 1].type;
}

const ast::Type& ast::FieldTypes::operator[](const std::string& field_name
) const {
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

bool ast::Type::operator==(const Type& t) const {
    if (this->type.index() != t.type.index()) return false;

    if (this->is_no_type()) {
        return true;
    } else if (this->is_type_variable()) {
        return std::get<ast::TypeVariable>(this->type).id
               == std::get<ast::TypeVariable>(t.type).id;
    } else if (this->is_final_type_variable()) {
        return std::get<ast::FinalTypeVariable>(this->type).id
               == std::get<ast::FinalTypeVariable>(t.type).id;
    } else if (this->is_nominal_type()) {
        if (std::get<ast::NominalType>(this->type).name
            == std::get<ast::NominalType>(t.type).name) {
            if (std::get<ast::NominalType>(this->type).parameters.size()
                == std::get<ast::NominalType>(t.type).parameters.size()) {
                for (size_t i = 0;
                     i
                     < std::get<ast::NominalType>(this->type).parameters.size();
                     i++) {
                    if (std::get<ast::NominalType>(this->type).parameters[i]
                        != std::get<ast::NominalType>(t.type).parameters[i]) {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    } else if (this->is_struct_type()) {
        for (auto field : this->as_struct_type().fields) {
            if (t.as_struct_type().fields.find(field.name)
                == t.as_struct_type().fields.end()) {
                return false;
            }

            if (t.as_struct_type().fields[field.name] != field.type) {
                return false;
            }
        }
        return true;
    } else {
        assert(false);
        return false;
    }
}

bool ast::Type::operator!=(const Type& t) const { return !(t == *this); }

static std::string as_letter(size_t type_var) {
    char letters[] = "abcdefghijklmnopqrstuvwxyz";
    if (type_var > 25) {
        return letters[(type_var + 18) % 26]
               + std::to_string(1 + type_var / 26);
    } else {
        return std::string(1, letters[(type_var + 18) % 26]);
    }
}

std::string ast::Type::to_str() const {
    std::string output = "";
    if (this->is_no_type()) {
        output += "ast::Notype";
    } else if (this->is_type_variable()) {
        output += std::to_string(std::get<ast::TypeVariable>(this->type).id);

        return output;
    } else if (this->is_final_type_variable()) {
        output += std::get<ast::FinalTypeVariable>(this->type).id;
        if (this->as_final_type_variable().parameter_constraints.size() > 0) {
            output += "[";
            for (size_t i = 0;
                 i
                 < this->as_final_type_variable().parameter_constraints.size();
                 i++) {
                output += this->as_final_type_variable()
                              .parameter_constraints[i]
                              .to_str();
                if (i + 1
                    != this->as_final_type_variable()
                           .parameter_constraints.size()) {
                    output += ", ";
                }
                output += "]";
            }
        }
    } else if (this->is_nominal_type()) {
        if (std::get<ast::NominalType>(this->type).parameters.size() == 0) {
            output += std::get<ast::NominalType>(this->type).name;
        } else {
            output += std::get<ast::NominalType>(this->type).name;
            output += "[";
            output += std::get<ast::NominalType>(this->type)
                          .parameters[0]
                          .to_str();
            for (size_t i = 1;
                 i < std::get<ast::NominalType>(this->type).parameters.size();
                 i++) {
                output += ", ";
                output += std::get<ast::NominalType>(this->type)
                              .parameters[i]
                              .to_str();
            }
            output += "]";
        }
    } else if (this->is_struct_type()) {
        output += "struct {";

        auto fields = this->as_struct_type().fields;
        for (auto field = fields.begin(); field != fields.end(); field++) {
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

    if (this->interface.size() > 0
        || this->type.as_final_type_variable().field_constraints.size() > 0) {
        output += ": ";
    }

    if (this->interface.size() > 0) {
        for (size_t i = 0; i < this->interface.elements.size(); i++) {
            output += this->interface.elements[i].name;
            if (i + 1 != this->interface.elements.size()) {
                output += " and ";
            }
        }
    } else if (this->type.as_final_type_variable().field_constraints.size()
               > 0) {
        output += "{";
        auto fields = this->type.as_final_type_variable().field_constraints;
        for (auto field = fields.begin(); field != fields.end(); field++) {
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
    return this->type.index() == ast::NominalTypeVariant
           && this->as_nominal_type().type_definition;
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
        return ast::types_are_concrete(
            std::get<ast::NominalType>(this->type).parameters
        );
    }

    if (this->is_struct_type()) {
        for (auto field : this->as_struct_type().fields) {
            if (!field.type.is_concrete()) {
                return false;
            }
        }

        return true;
    }

    return true;
}

bool ast::Type::is_integer() const {
    if (*this == ast::Type("Int64"))
        return true;
    else if (*this == ast::Type("Int32"))
        return true;
    else if (*this == ast::Type("Int16"))
        return true;
    else if (*this == ast::Type("Int8"))
        return true;
    else
        return false;
}

bool ast::Type::is_float() const {
    if (*this == ast::Type("Float64"))
        return true;
    else if (*this == ast::Type("Float32"))
        return true;
    else
        return false;
}

bool ast::Type::is_pointer() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    if (std::get<ast::NominalType>(this->type).name != "Pointer") {
        return false;
    }

    return true;
}

bool ast::Type::is_boxed() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    if (std::get<ast::NominalType>(this->type).name != "Boxed") {
        return false;
    }

    return true;
}

bool ast::Type::has_boxed_elements() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    for (auto parameter : this->as_nominal_type().parameters) {
        return parameter.is_boxed() || parameter.has_boxed_elements();
    }

    if (this->as_nominal_type().type_definition != nullptr) {
        for (auto field : this->as_nominal_type().type_definition->fields) {
            if (field->type.is_boxed() || field->type.has_boxed_elements()) {
                return true;
            }
        }
    }

    return false;
}

bool ast::Type::is_array() const {
    if (!this->is_nominal_type()) {
        return false;
    }

    if (this->as_nominal_type().name.size() < 5
        || this->as_nominal_type().name.substr(0, 5) != "Array") {
        return false;
    }

    if (this->as_nominal_type().name.size() > 5
        && !is_number(this->as_nominal_type().name.substr(
            5,
            this->as_nominal_type().name.size()
        ))) {
        return false;
    }

    return true;
}

bool ast::Type::is_builtin_type() const {
    return this->is_pointer() || this->is_boxed() || this->is_array()
           || (this->is_nominal_type()
               && primitive_types.contains(this->as_nominal_type().name));
}

bool ast::Type::is_collection() const {
    return this->is_struct_type() || this->is_array();
}

size_t ast::Type::array_size_known() const {
    assert(this->is_array());
    return this->as_nominal_type().name.size() > 5;
}

size_t ast::Type::get_array_size() const {
    assert(this->is_array());
    assert(this->as_nominal_type().name.size() > 5);
    return stoi(std::get<ast::NominalType>(this->type)
                    .name.substr(5, this->as_nominal_type().name.size()));
}

// Add hash struct for ast::Type to be able to use ast::Type as keys of
// std::unordered_map
std::size_t std::hash<ast::Type>::operator()(const ast::Type& type) const {
    return std::hash<std::string>()(type.to_str());
}

// Node
// ----
ast::Type ast::get_type(Node* node) {
    return std::visit([](const auto& variant) { return variant.type; }, *node);
}

ast::Type ast::get_concrete_type(
    Node* node, std::unordered_map<std::string, Type>& type_bindings
) {
    return ast::get_concrete_type(ast::get_type(node), type_bindings);
}

ast::Type ast::get_concrete_type(
    Type type, std::unordered_map<std::string, Type>& type_bindings
) {
    if (type.is_final_type_variable()) {
        if (type_bindings.find(type.as_final_type_variable().id)
            != type_bindings.end()) {
            type = type_bindings[type.as_final_type_variable().id];
        } else {
            std::cout << "unknown type: " << type.to_str() << "\n";
            assert(false);
        }
    }
    if (!type.is_concrete()) {
        if (type.is_nominal_type()) {
            for (size_t i = 0; i < type.as_nominal_type().parameters.size();
                 i++) {
                type.as_nominal_type().parameters[i] = ast::get_concrete_type(
                    type.as_nominal_type().parameters[i],
                    type_bindings
                );
            }
        } else if (type.is_struct_type()) {
            std::cout << "WAHT ? " << type.to_str() << "\n";
            assert(false);
        }
    }
    return type;
}

ast::Type ast::try_to_get_concrete_type(
    Type type, std::unordered_map<std::string, Type>& type_bindings
) {
    if (type.is_final_type_variable()) {
        if (type_bindings.find(type.as_final_type_variable().id)
            != type_bindings.end()) {
            type = type_bindings[type.as_final_type_variable().id];
        }
    }
    if (!type.is_concrete()) {
        if (type.is_nominal_type()) {
            for (size_t i = 0; i < type.as_nominal_type().parameters.size();
                 i++) {
                type.as_nominal_type().parameters[i]
                    = ast::try_to_get_concrete_type(
                        type.as_nominal_type().parameters[i],
                        type_bindings
                    );
            }
        }
    }
    return type;
}

void ast::set_type(Node* node, Type type) {
    std::visit([type](auto& variant) { variant.type = type; }, *node);
}

std::vector<ast::Type> ast::get_types(std::vector<CallArgumentNode*> nodes) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(get_type(nodes[i]->expression));
    }
    return types;
}

std::vector<ast::Type> ast::get_types(std::vector<FunctionArgumentNode*> nodes
) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(nodes[i]->type);
    }
    return types;
}

std::vector<ast::Type> ast::get_concrete_types(
    std::vector<Node*> nodes,
    std::unordered_map<std::string, Type>& type_bindings
) {
    std::vector<Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(get_concrete_type(nodes[i], type_bindings));
    }
    return types;
}

std::vector<ast::Type> ast::get_concrete_types(
    std::vector<ast::Type> type_variables,
    std::unordered_map<std::string, Type>& type_bindings
) {
    std::vector<Type> types;
    for (size_t i = 0; i < type_variables.size(); i++) {
        types.push_back(get_concrete_type(type_variables[i], type_bindings));
    }
    return types;
}

namespace ast {
    bool could_be_expression(ast::BlockNode node) {
        if (node.statements.size() == 1
            && could_be_expression(node.statements[0])) {
            return true;
        }
        return false;
    }
    bool could_be_expression(FunctionArgumentNode node) { unreachable(); }
    bool could_be_expression(FunctionNode node) { return false; }
    bool could_be_expression(InterfaceNode node) { return false; }
    bool could_be_expression(TypeNode node) { return false; }
    bool could_be_expression(DeclarationNode node) { return false; }
    bool could_be_expression(AssignmentNode node) { return false; }
    bool could_be_expression(ReturnNode node) { return false; }
    bool could_be_expression(BreakNode node) { return false; }
    bool could_be_expression(ContinueNode node) { return false; }
    bool could_be_expression(IfElseNode node) {
        if (node.else_branch.has_value() && could_be_expression(node.if_branch)
            && could_be_expression(node.else_branch.value())) {
            return true;
        }
        return false;
    }
    bool could_be_expression(WhileNode node) { return false; }
    bool could_be_expression(UseNode node) { return false; }
    bool could_be_expression(LinkWithNode node) { return false; }
    bool could_be_expression(CallArgumentNode node) { unreachable(); }
    bool could_be_expression(CallNode node) { return true; }
    bool could_be_expression(StructLiteralNode node) { return true; }
    bool could_be_expression(FloatNode node) { return true; }
    bool could_be_expression(IntegerNode node) { return true; }
    bool could_be_expression(IdentifierNode node) { return true; }
    bool could_be_expression(BooleanNode node) { return true; }
    bool could_be_expression(StringNode node) { return true; }
    bool could_be_expression(InterpolatedStringNode node) { return true; }
    bool could_be_expression(ArrayNode node) { return true; }
    bool could_be_expression(FieldAccessNode node) { return true; }
    bool could_be_expression(AddressOfNode node) { return true; }
    bool could_be_expression(DereferenceNode node) { return true; }
    bool could_be_expression(NewNode node) { return true; }
}

bool ast::could_be_expression(ast::Node* node) {
    return std::visit(
        [](auto& variant) { return could_be_expression(variant); },
        *node
    );
}

void ast::transform_to_expression(ast::Node*& node) {
    assert(could_be_expression(node));
    if (std::holds_alternative<ast::BlockNode>(*node)) {
        auto& block = std::get<BlockNode>(*node);
        node = block.statements[0];
        transform_to_expression(node);
    } else if (std::holds_alternative<ast::IfElseNode>(*node)) {
        auto& if_else = std::get<IfElseNode>(*node);
        transform_to_expression(if_else.if_branch);
        transform_to_expression(if_else.else_branch.value());
    }
}

bool ast::has_type_variables(std::vector<ast::Type> types) {
    for (auto& type : types) {
        if (type.is_type_variable()) return true;
    }
    return false;
}

bool ast::types_are_concrete(std::vector<ast::Type> types) {
    for (auto& type : types) {
        if (!type.is_concrete()) return false;
    }
    return true;
}

namespace ast {
    bool is_expression(ast::BlockNode node) { return false; }
    bool is_expression(FunctionArgumentNode node) { return false; }
    bool is_expression(FunctionNode node) { return false; }
    bool is_expression(InterfaceNode node) { return false; }
    bool is_expression(TypeNode node) { return false; }
    bool is_expression(DeclarationNode node) { return false; }
    bool is_expression(AssignmentNode node) { return false; }
    bool is_expression(ReturnNode node) { return false; }
    bool is_expression(BreakNode node) { return false; }
    bool is_expression(ContinueNode node) { return false; }
    bool is_expression(IfElseNode node) {
        if (node.else_branch.has_value() && is_expression(node.if_branch)
            && is_expression(node.else_branch.value())) {
            return true;
        }
        return false;
    }
    bool is_expression(WhileNode node) { return false; }
    bool is_expression(UseNode node) { return false; }
    bool is_expression(LinkWithNode node) { return false; }
    bool is_expression(CallArgumentNode node) { unreachable(); }
    bool is_expression(CallNode node) { return true; }
    bool is_expression(StructLiteralNode node) { return true; }
    bool is_expression(FloatNode node) { return true; }
    bool is_expression(IntegerNode node) { return true; }
    bool is_expression(IdentifierNode node) { return true; }
    bool is_expression(BooleanNode node) { return true; }
    bool is_expression(StringNode node) { return true; }
    bool is_expression(InterpolatedStringNode node) { return true; }
    bool is_expression(ArrayNode node) { return true; }
    bool is_expression(FieldAccessNode node) { return true; }
    bool is_expression(AddressOfNode node) { return true; }
    bool is_expression(DereferenceNode node) { return true; }
    bool is_expression(NewNode node) { return true; }
}

bool ast::is_expression(ast::Node* node) {
    return std::visit(
        [](auto& variant) { return is_expression(variant); },
        *node
    );
}

size_t ast::TypeNode::get_index_of_field(std::string field_name) {
    for (size_t i = 0; i < this->fields.size(); i++) {
        if (this->fields[i]->value == field_name) {
            return i;
        }
    }

    assert(false);
    return 0;
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
        Node* array = new Node
            [this->initial_size
             * static_cast<unsigned int>(
                 pow(this->growth_factor, this->nodes.size())
             )];
        this->nodes.push_back(array);
    }

    this->size++;
    this->nodes[this->nodes.size() - 1]
               [this->size - 1 - this->size_of_arrays_filled()]
        = node;
}

size_t ast::Ast::size_of_arrays_filled() {
    if (this->nodes.size() == 0)
        return 0;
    else {
        size_t size_of_arrays_filled = 0;
        for (size_t i = 0; i < this->nodes.size() - 1; i++) {
            size_of_arrays_filled
                += this->initial_size
                   * static_cast<unsigned int>(pow(this->growth_factor, i));
        }
        return size_of_arrays_filled;
    }
}

ast::Node* ast::Ast::last_element() {
    if (this->size == 0)
        return nullptr;
    else
        return &this->nodes[this->nodes.size() - 1]
                           [this->size - 1 - this->size_of_arrays_filled()];
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

ast::Type ast::get_concrete_type_or_type_variable(
    ast::Type type, ast::PrintContext context
) {
    if (context.concrete) {
        return ast::get_concrete_type(type, context.type_bindings);
    } else {
        return type;
    }
}

void ast::print(const Ast& ast, PrintContext context) {
    std::cout << "program\n";
    print((ast::Node*)ast.program, context);
}

void ast::print_with_concrete_types(const Ast& ast, PrintContext context) {
    context.concrete = true;
    std::cout << "program\n";
    print_with_concrete_types((ast::Node*)ast.program, context);
}

void ast::print_with_concrete_types(Node* node, PrintContext context) {
    context.concrete = true;
    print(node, context);
}

namespace ast {
    void print(ast::BlockNode node, PrintContext context) {
        // Put all nodes of block in a vector
        std::vector<Node*> nodes = {};
        for (size_t i = 0; i < node.use_statements.size(); i++) {
            nodes.push_back((Node*)node.use_statements[i]);
        }
        for (size_t i = 0; i < node.statements.size(); i++) {
            nodes.push_back(node.statements[i]);
        }
        for (size_t i = 0; i < node.types.size(); i++) {
            nodes.push_back((Node*)node.types[i]);
        }
        for (size_t i = 0; i < node.interfaces.size(); i++) {
            nodes.push_back((Node*)node.interfaces[i]);
        }
        for (size_t i = 0; i < node.functions.size(); i++) {
            nodes.push_back((Node*)node.functions[i]);
        }

        // Print all nodes
        for (size_t i = 0; i < nodes.size(); i++) {
            if (!context.concrete
                || !std::holds_alternative<ast::FunctionNode>(*nodes[i])) {
                bool is_last = i == nodes.size() - 1;
                print(
                    nodes[i],
                    PrintContext{
                        context.indent_level + 1,
                        append(context.last, is_last),
                        context.concrete,
                        context.type_bindings
                    }
                );
            } else {
                auto function = (ast::FunctionNode*)nodes[i];
                for (size_t j = 0; j < function->specializations.size(); j++) {
                    // Check if is last specialization
                    bool is_last = j == function->specializations.size() - 1;
                    for (size_t k = i + 1; k < nodes.size(); k++) {
                        if (((FunctionNode*)nodes[k])->specializations.size()
                            != 0) {
                            is_last = false;
                            break;
                        }
                    }

                    context.type_bindings
                        = function->specializations[j].type_bindings;
                    print(
                        (ast::Node*)nodes[i],
                        PrintContext{
                            context.indent_level + 1,
                            append(context.last, is_last),
                            context.concrete,
                            context.type_bindings
                        }
                    );
                }
            }
        }
    }
    void print(FunctionArgumentNode node, PrintContext context) {
        unreachable();
    }
    void print(FunctionNode node, PrintContext context) {
        bool last = true;
        if (context.last.size() != 0) {
            last = context.last[context.last.size() - 1];
            context.last.pop_back();
        }

        put_indent_level(context.indent_level, append(context.last, last));
        if (node.is_extern) {
            std::cout << "extern " << node.identifier->value << '(';
        } else if (node.is_builtin) {
            std::cout << "builtin " << node.identifier->value << '(';
        } else {
            std::cout << "function " << node.identifier->value;

            if (node.type_parameters.size() > 0 && !context.concrete) {
                std::cout << '[';
                for (size_t i = 0; i < node.type_parameters.size(); i++) {
                    std::cout << node.type_parameters[i].to_str();
                    if (i + 1 != node.type_parameters.size()) std::cout << ", ";
                }
                std::cout << ']';
            }

            std::cout << '(';
        }
        for (size_t i = 0; i < node.args.size(); i++) {
            auto& arg_name = node.args[i]->identifier->value;
            if (node.args[i]->is_mutable) {
                std::cout << "mut ";
            }
            std::cout << arg_name;

            auto& arg_type = node.args[i]->type;

            if (arg_type != Type(ast::NoType{})) {
                std::cout
                    << ": "
                    << get_concrete_type_or_type_variable(arg_type, context)
                           .to_str();
            }

            if (i != node.args.size() - 1) std::cout << ", ";
        }

        if (node.is_extern_and_variadic) {
            std::cout << ", ...";
        }

        std::cout << ")";
        if (node.return_type != Type(ast::NoType{})) {
            std::cout << ": ";
            if (node.return_type_is_mutable) std::cout << "mut ";
            std::cout << get_concrete_type_or_type_variable(
                             node.return_type,
                             context
            )
                             .to_str();
        }
        std::cout << "\n";

        if (node.is_extern || node.is_builtin) {
            return;
        }

        if (!is_expression(node.body)) {
            context.last.push_back(last);
            print(node.body, context);
            context.last.pop_back();
        } else {
            context.indent_level += 1;
            context.last.push_back(last);
            context.last.push_back(true);
            print(node.body, context);
            context.indent_level -= 1;
            context.last.pop_back();
            context.last.pop_back();
        }
    }
    void print(InterfaceNode node, PrintContext context) {
        bool last = true;
        if (context.last.size() != 0) {
            last = context.last[context.last.size() - 1];
            context.last.pop_back();
        }

        put_indent_level(context.indent_level, append(context.last, last));
        std::cout << "interface " << node.identifier->value;

        if (node.type_parameters.size() > 0 && !context.concrete) {
            std::cout << '[';
            for (size_t i = 0; i < node.type_parameters.size(); i++) {
                std::cout << node.type_parameters[i].to_str();
                if (i + 1 != node.type_parameters.size()) std::cout << ", ";
            }
            std::cout << ']';
        }

        std::cout << '(';
        for (size_t i = 0; i < node.args.size(); i++) {
            auto& arg_name = node.args[i]->identifier->value;
            if (node.args[i]->is_mutable) {
                std::cout << "mut ";
            }
            std::cout << arg_name;

            auto& arg_type = node.args[i]->type;

            if (arg_type != Type(ast::NoType{})) {
                std::cout
                    << ": "
                    << get_concrete_type_or_type_variable(arg_type, context)
                           .to_str();
            }

            if (i != node.args.size() - 1) std::cout << ", ";
        }

        std::cout << ")";
        if (node.return_type != Type(ast::NoType{})) {
            std::cout << ": ";
            if (node.return_type_is_mutable) std::cout << "mut ";
            std::cout << get_concrete_type_or_type_variable(
                             node.return_type,
                             context
            )
                             .to_str();
        }
        std::cout << "\n";
    }
    void print(TypeNode node, PrintContext context) {
        put_indent_level(
            context.indent_level,
            append(
                context.last,
                node.fields.size() == 0 && node.cases.size() == 0
            )
        );
        std::cout << "type " << node.identifier->value << '\n';

        for (size_t i = 0; i < node.fields.size(); i++) {
            bool is_last
                = i == node.fields.size() - 1 && node.cases.size() == 0;
            print(
                (Node*)node.fields[i],
                PrintContext{
                    context.indent_level + 1,
                    append(context.last, is_last),
                    context.concrete,
                    context.type_bindings
                }
            );
        }

        for (size_t i = 0; i < node.cases.size(); i++) {
            bool is_last = i == node.cases.size() - 1;
            put_indent_level(
                context.indent_level + 1,
                append(context.last, is_last)
            );
            std::cout << "case " << node.cases[i]->identifier->value << '\n';
        }
    }
    void print(DeclarationNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << (node.is_mutable ? "=" : "be") << '\n';
        context.indent_level += 1;
        context.last.push_back(false);
        print((ast::Node*)node.identifier, context);
        context.last[context.last.size() - 1] = true;
        print(node.expression, context);
    }
    void print(AssignmentNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << ":=" << '\n';
        context.indent_level += 1;
        context.last.push_back(false);
        print(node.assignable, context);
        context.last[context.last.size() - 1] = true;
        print(node.expression, context);
    }
    void print(ReturnNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "return" << '\n';
        if (node.expression.has_value()) {
            context.indent_level += 1;
            context.last.push_back(true);
            print(node.expression.value(), context);
        }
    }
    void print(BreakNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "break" << '\n';
    }
    void print(ContinueNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "continue" << '\n';
    }
    void print(IfElseNode node, PrintContext context) {
        if (!is_expression(node)) {
            bool is_last = context.last[context.last.size() - 1];
            context.last.pop_back();

            bool has_else_block = node.else_branch ? true : false;
            put_indent_level(
                context.indent_level,
                append(context.last, is_last && !has_else_block)
            );
            std::cout << "if" << '\n';
            print(
                node.condition,
                PrintContext{
                    context.indent_level + 1,
                    append(
                        append(context.last, is_last && !has_else_block),
                        false
                    ),
                    context.concrete,
                    context.type_bindings
                }
            );

            print(
                node.if_branch,
                PrintContext{
                    context.indent_level,
                    append(context.last, is_last && !has_else_block),
                    context.concrete,
                    context.type_bindings
                }
            );

            if (node.else_branch.has_value()) {
                put_indent_level(
                    context.indent_level,
                    append(context.last, is_last)
                );
                std::cout << "else" << "\n";
                print(
                    node.else_branch.value(),
                    PrintContext{
                        context.indent_level,
                        append(context.last, is_last),
                        context.concrete,
                        context.type_bindings
                    }
                );
            }
        } else {
            bool is_last = true;
            if (context.last.size() > 0) {
                is_last = context.last[context.last.size() - 1];
                context.last.pop_back();
            }

            put_indent_level(context.indent_level, append(context.last, false));
            std::cout << "if";
            if (get_concrete_type_or_type_variable(node.type, context)
                != Type(ast::NoType{}))
                std::cout
                    << ": "
                    << get_concrete_type_or_type_variable(node.type, context)
                           .to_str();
            std::cout << "\n";
            print(
                node.condition,
                PrintContext{
                    context.indent_level + 1,
                    append(append(context.last, false), false),
                    context.concrete,
                    context.type_bindings
                }
            );
            print(
                node.if_branch,
                PrintContext{
                    context.indent_level + 1,
                    append(append(context.last, false), true),
                    context.concrete,
                    context.type_bindings
                }
            );

            assert(node.else_branch.has_value());
            put_indent_level(
                context.indent_level,
                append(context.last, is_last)
            );
            std::cout << "else" << "\n";
            print(
                node.else_branch.value(),
                PrintContext{
                    context.indent_level + 1,
                    append(append(context.last, is_last), true),
                    context.concrete,
                    context.type_bindings
                }
            );
        }
    }
    void print(WhileNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "while" << '\n';
        print(
            node.condition,
            PrintContext{
                context.indent_level + 1,
                append(context.last, false),
                context.concrete,
                context.type_bindings
            }
        );
        print(
            node.block,
            PrintContext{
                context.indent_level,
                context.last,
                context.concrete,
                context.type_bindings
            }
        );
    }
    void print(UseNode node, PrintContext context) {
        if (node.include) {
            std::cout << "include";
        } else {
            std::cout << "use";
        }
        std::cout << " \"" << node.path->value << "\"\n";
    }
    void print(LinkWithNode node, PrintContext context) { unreachable(); }
    void print(CallArgumentNode node, PrintContext context) {
        if (node.identifier.has_value()) {
            put_indent_level(context.indent_level, context.last);
            if (node.is_mutable) {
                std::cout << "mut ";
            }
            std::cout << node.identifier.value()->value << ":\n";
            print(
                node.expression,
                PrintContext{
                    context.indent_level + 1,
                    append(context.last, true),
                    context.concrete,
                    context.type_bindings
                }
            );
        } else {
            if (node.is_mutable) {
                put_indent_level(context.indent_level, context.last);
                std::cout << "mut " << "\n";
                print(
                    node.expression,
                    PrintContext{
                        context.indent_level + 1,
                        append(context.last, true),
                        context.concrete,
                        context.type_bindings
                    }
                );
            } else {
                print(
                    node.expression,
                    PrintContext{
                        context.indent_level,
                        context.last,
                        context.concrete,
                        context.type_bindings
                    }
                );
            }
        }
    }
    void print(CallNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << node.identifier->value;
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
        for (size_t i = 0; i < node.args.size(); i++) {
            print(
                (ast::Node*)node.args[i],
                PrintContext{
                    context.indent_level + 1,
                    append(context.last, i == node.args.size() - 1),
                    context.concrete,
                    context.type_bindings
                }
            );
        }
    }
    void print(StructLiteralNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << node.identifier->value << "{}";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
        for (auto it = node.fields.begin(); it != node.fields.end(); it++) {
            auto is_last = std::next(it) == node.fields.end();
            put_indent_level(
                context.indent_level + 1,
                append(context.last, is_last)
            );
            std::cout << it->first->value << ":\n";
            print(
                it->second,
                PrintContext{
                    context.indent_level + 2,
                    append(append(context.last, is_last), true),
                    context.concrete,
                    context.type_bindings
                }
            );
        }
    }
    void print(FloatNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << node.value;
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
    }
    void print(IntegerNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << node.value;
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
    }
    void print(IdentifierNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << node.value;
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
    }
    void print(BooleanNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << (node.value ? "true" : "false");
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
    }
    void print(StringNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "\"";
        for (size_t i = 0; i < node.value.size(); i++) {
            if (node.value[i] == '\n') {
                std::cout << "\\n";
            } else {
                std::cout << node.value[i];
            }
        }
        std::cout << "\"";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
    }
    void print(InterpolatedStringNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "InterpolatedString";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";

        for (size_t i = 0; i < node.strings.size(); i++) {
            bool is_last = i + 1 == node.strings.size();
            put_indent_level(
                context.indent_level + 1,
                append(context.last, is_last)
            );
            std::cout << "\"";
            for (size_t j = 0; j < node.strings[i].size(); j++) {
                if (node.strings[i][j] == '\n') {
                    std::cout << "\\n";
                } else {
                    std::cout << node.strings[i][j];
                }
            }
            std::cout << "\"\n";

            if (!is_last) {
                auto newContext = context;
                newContext.indent_level += 1;
                newContext.last = append(context.last, false);
                print(node.expressions[i], newContext);
            }
        }
    }
    void print(ArrayNode node, PrintContext context) {
        bool is_last = context.last[context.last.size()];

        put_indent_level(context.indent_level, context.last);
        std::cout << "[]";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";
        for (size_t i = 0; i < node.elements.size(); i++) {
            print(
                node.elements[i],
                PrintContext{
                    context.indent_level + 1,
                    append(context.last, i + 1 == node.elements.size()),
                    context.concrete,
                    context.type_bindings
                }
            );
        }
    }
    void print(FieldAccessNode node, PrintContext context) {
        // There should be at least 1 identifiers in fields accessed. eg: circle.radius
        assert(node.fields_accessed.size() >= 1);

        print(node.accessed, context);

        std::vector<bool> last = append(context.last, true);
        for (size_t i = 0; i < node.fields_accessed.size(); i++) {
            put_indent_level(context.indent_level + i + 1, last);
            std::cout << node.fields_accessed[i]->value;
            if (node.fields_accessed[i]->type != Type(ast::NoType{}))
                std::cout << ": "
                          << get_concrete_type_or_type_variable(
                                 node.fields_accessed[i]->type,
                                 context
                             )
                                 .to_str();
            std::cout << "\n";
            last.push_back(true);
        }
    }
    void print(AddressOfNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "&";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";

        context.indent_level += 1;
        context.last.push_back(true);
        ast::print(node.expression, context);
    }
    void print(DereferenceNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "*";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";

        context.indent_level += 1;
        context.last.push_back(true);
        ast::print(node.expression, context);
    }
    void print(NewNode node, PrintContext context) {
        put_indent_level(context.indent_level, context.last);
        std::cout << "new";
        if (node.type != Type(ast::NoType{}))
            std::cout << ": "
                      << get_concrete_type_or_type_variable(node.type, context)
                             .to_str();
        std::cout << "\n";

        context.indent_level += 1;
        context.last.push_back(true);
        ast::print(node.expression, context);
    }
}

void ast::print(ast::Node* node, PrintContext context) {
    std::visit([&context](auto& variant) { print(variant, context); }, *node);
}

bool ast::FunctionNode::typed_parameter_aready_added(ast::Type type) {
    for (auto type_parameter : this->type_parameters) {
        if (type_parameter.type == type) {
            return true;
        }
    }
    return false;
}

std::optional<ast::TypeParameter*> ast::FunctionNode::get_type_parameter(
    ast::Type type
) {
    for (auto& type_parameter : this->type_parameters) {
        if (type_parameter.type == type) {
            return &type_parameter;
        }
    }
    return std::nullopt;
}

static bool _is_in_type_parameter(ast::Type type_parameter, ast::Type type) {
    if (type_parameter == type) {
        return true;
    }

    for (auto field :
         type_parameter.as_final_type_variable().field_constraints) {
        if (_is_in_type_parameter(field.type, type)) {
            return true;
        }
    }

    for (auto parameter :
         type_parameter.as_final_type_variable().parameter_constraints) {
        if (_is_in_type_parameter(parameter, type)) {
            return true;
        }
    }

    return false;
}

bool ast::FunctionNode::is_in_type_parameter(ast::Type type) {
    for (auto type_parameter : this->type_parameters) {
        if (_is_in_type_parameter(type_parameter.type, type)) {
            return true;
        }
    }

    return false;
}

bool ast::InterfaceNode::typed_parameter_aready_added(ast::Type type) {
    for (auto type_parameter : this->type_parameters) {
        if (type_parameter.type == type) {
            return true;
        }
    }
    return false;
}

std::optional<ast::TypeParameter*> ast::InterfaceNode::get_type_parameter(
    ast::Type type
) {
    for (auto& type_parameter : this->type_parameters) {
        if (type_parameter.type == type) {
            return &type_parameter;
        }
    }
    return std::nullopt;
}

bool ast::InterfaceNode::is_in_type_parameter(ast::Type type) {
    for (auto type_parameter : this->type_parameters) {
        if (_is_in_type_parameter(type_parameter.type, type)) {
            return true;
        }
    }

    return false;
}

std::vector<ast::Type> ast::InterfaceNode::get_prototype() {
    std::vector<ast::Type> results;
    for (auto arg : this->args) {
        results.push_back(ast::get_type((ast::Node*)arg));
    }
    results.push_back(this->return_type);
    return results;
}

bool ast::InterfaceNode::is_compatible_with(ast::Type type) {
    for (auto function : this->functions) {
        for (size_t i = 0; i < function->args.size(); i++) {
            if (this->args[i]->type == this->type_parameters[0].type) {
                if (type == function->args[i]->type) {
                    return true;
                }
                break;
            }
        }
    }
    std::cout << "No implementation of " << this->identifier->value << " for "
              << type.to_str() << "\n";
    assert(false);
    return false;
}

std::optional<ast::TypeParameter*> ast::get_type_parameter(
    std::vector<ast::TypeParameter>& type_parameters, ast::Type type
) {
    for (auto& type_parameter : type_parameters) {
        if (type_parameter.type == type) {
            return &type_parameter;
        }
    }
    return std::nullopt;
}
