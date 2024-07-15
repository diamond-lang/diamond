#include "make_concrete.hpp"
#include "intrinsics.hpp"
#include "semantic.hpp"

// Helper functions
// ----------------
static void print_bindings(std::unordered_map<std::string, ast::Type>& type_bindings) {
    std::cout << "bindings\n";
    for (auto binding: type_bindings) {
        std::cout << "    " << binding.first << ": " << binding.second.to_str() << "\n";
    }
}

bool semantic::is_type_concrete(Context& context, ast::Type type) {
    if (type.is_concrete()) {
        return true;
    }
    if (type.is_final_type_variable()
    &&  context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
        return true;
    }
    return false;
}

ast::Type semantic::get_type(Context& context, ast::Type type) {
    if (type.is_final_type_variable()) {
        if (context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
            type = context.type_inference.type_bindings[type.as_final_type_variable().id];
        }
        else {
            return type;
        }
    }
    if (!type.is_concrete()) {
        for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
            type.as_nominal_type().parameters[i] = semantic::get_type_or_default(context, type.as_nominal_type().parameters[i]);
        }
    }
    return type;
}

ast::Type semantic::get_type_or_default(Context& context, ast::Type type) {
    assert(!type.is_type_variable());

    if (type.is_final_type_variable()) {
        if (context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
            type = context.type_inference.type_bindings[type.as_final_type_variable().id];
        }
        else if (context.type_inference.interface_constraints.find(type) != context.type_inference.interface_constraints.end()) {
            std::cout << "assert: default types\n";
            assert(false);
            //type = context.type_inference.interface_constraints[type].get_default_type();
        }
        else {
            std::cout << "unknown type: " << type.to_str() << "\n";
            assert(false);
        }
    }
    if (!type.is_concrete()) {
        if (type.is_nominal_type()) {
            for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
                type.as_nominal_type().parameters[i] = semantic::get_type_or_default(context, type.as_nominal_type().parameters[i]);
            }
        }
        else if (type.is_struct_type()) {
            assert(false);
        }
    }
    return type;
}

ast::Type semantic::get_type_or_default(Context& context, ast::Node* node) {
    return semantic::get_type_or_default(context, ast::get_type(node));
}

std::vector<ast::Type> semantic::get_types_or_default(Context& context, std::vector<ast::Type> type_variables) {
    std::vector<ast::Type> types;
    for (size_t i = 0; i < type_variables.size(); i++) {
        types.push_back(semantic::get_type_or_default(context, type_variables[i]));
    }
    return types;
}

std::vector<ast::Type> semantic::get_types_or_default(Context& context, std::vector<ast::Node*> nodes) {
    std::vector<ast::Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(semantic::get_type_or_default(context, nodes[i]));
    }
    return types;
}

// Make concrete
// -------------
Result<Ok, Error> semantic::make_concrete(Context& context, ast::Node* node, std::vector<ast::CallInCallStack> call_stack) {
    return std::visit([&context, node, &call_stack](auto& variant) {
        return semantic::make_concrete(context, variant, call_stack);
    }, *node);
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BlockNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Add scope
    semantic::add_scope(context);

    // Add functions to the current scope
    auto result = semantic::add_definitions_to_current_scope(context, node);
    if (result.is_error()) return result;

    // Make statements concrete
    size_t number_of_errors = context.errors.size();
    for (auto statement: node.statements) {
        auto result = semantic::make_concrete(context, statement, call_stack);
        if (result.is_error()) return result;

        if (statement->index() == ast::Call) {
            if (semantic::get_type(context, ast::get_type(statement)) != ast::Type("void")) {
                context.errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*statement), context.current_module));
                return Error{};
            }
        }
    }

    // Remove scope
    semantic::remove_scope(context);

    if (context.errors.size() > number_of_errors) return Error {};
    else                                          return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FunctionArgumentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FunctionNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::InterfaceNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::TypeNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::DeclarationNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return semantic::make_concrete(context, node.expression, call_stack);
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::AssignmentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.assignable, call_stack);
    if (result.is_error()) return result;
    
    result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ReturnNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (node.expression.has_value()) {
        return semantic::make_concrete(context, node.expression.value(), call_stack);
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BreakNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ContinueNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IfElseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::make_concrete(context, node.if_branch, call_stack);
    if (result.is_error()) return result;
    
    if (node.else_branch.has_value()) {
        result = semantic::make_concrete(context, node.else_branch.value(), call_stack);
        if (result.is_error()) return result;
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::WhileNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::make_concrete(context, node.block, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::UseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::LinkWithNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::CallArgumentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

static Result<Ok, Error> unify_function_type_with_call_type(semantic::Context& context, std::vector<ast::TypeParameter> type_parameters, ast::Type function_type, ast::Type call_type) {
    if (!semantic::is_type_concrete(context, call_type)) {
        if (call_type.is_final_type_variable()) {
            context.type_inference.type_bindings[call_type.as_final_type_variable().id] = ast::get_default_type(context.type_inference.interface_constraints[call_type]);
        }
        else {
            assert(false);
        }
    }
    else {
        //assert(false);
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    auto call_args = ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings);
    auto call_type = ast::get_concrete_type(node.type, context.type_inference.type_bindings);
    auto result = semantic::get_function_type(context, binding->value,  call_args, call_type);
    if (result.is_error()) return Error{};
    semantic::unify_types(context.type_inference.type_bindings, node.type, result.get_value());

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::StructLiteralNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    for (auto field: node.fields) {
        auto result = semantic::make_concrete(context, field.second, call_stack);
        if (result.is_error()) return result;
    }
    node.type = ast::Type(node.identifier->value, semantic::get_type_definition(*binding));

    return Ok{};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FloatNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (semantic::get_type(context, node.type).is_final_type_variable()) {
        context.type_inference.type_bindings[node.type.as_final_type_variable().id] = ast::Type("float64");
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IntegerNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (semantic::get_type(context, node.type).is_final_type_variable()) {
        context.type_inference.type_bindings[node.type.as_final_type_variable().id] = ast::Type("int64");
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IdentifierNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BooleanNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::StringNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::InterpolatedStringNode& node, std::vector<ast::CallInCallStack> call_stack) {
    for (auto expression: node.expressions) {
        auto result = semantic::make_concrete(context, expression, call_stack);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ArrayNode& node, std::vector<ast::CallInCallStack> call_stack) {
    for (auto element: node.elements) {
        auto result = make_concrete(context, element, call_stack);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FieldAccessNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(node.fields_accessed.size() >= 1);

    if (!node.type.is_final_type_variable()) {
        return Ok {};
    }
    else {
        auto result = semantic::make_concrete(context, node.accessed, call_stack);
        if (result.is_error()) return Error {};

        assert(semantic::get_type(context, ast::get_type(node.accessed)).is_concrete());
        ast::Type struct_type = semantic::get_type(context, ast::get_type(node.accessed));

        for (size_t i = 0; i < node.fields_accessed.size(); i++) {
            auto field = node.fields_accessed[i]->value;
            assert(struct_type.is_nominal_type());

            bool field_founded = false;
            for (auto field_in_definition: struct_type.as_nominal_type().type_definition->fields) {
                if (field == field_in_definition->value) {
                    field_founded = true;
                    context.type_inference.type_bindings[node.fields_accessed[i]->type.as_final_type_variable().id] = field_in_definition->type;
                    break;
                }
            }
            assert(field_founded);

            // Iterate over struct type
            if (i + 1 != node.fields_accessed.size()) {
                struct_type = semantic::get_type(context, node.fields_accessed[i]->type);
            }
        }

        // Set overall node type
        node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

        return Ok {};
    }
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::AddressOfNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::DereferenceNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    assert(semantic::get_type(context, ast::get_type(node.expression)).is_pointer()
    ||     semantic::get_type(context, ast::get_type(node.expression)).is_boxed());

    semantic::unify_types(context.type_inference.type_bindings, node.type, semantic::get_type(context, ast::get_type(node.expression)).as_nominal_type().parameters[0]);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::NewNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::unify_types(std::unordered_map<std::string, ast::Type>& type_bindings, ast::Type function_type, ast::Type argument_type) {
    if (function_type.is_final_type_variable()) {
        type_bindings[function_type.as_final_type_variable().id] = argument_type;
    }
    else if (function_type.is_nominal_type()) {
        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            auto result = unify_types(type_bindings, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i]);
            if (result.is_error()) return Error{};
        }
    }
    else {
        assert(false);
    }

    return Ok{};
}

// Set concrete types
// ------------------
void semantic::set_concrete_types(Context& context, ast::Node* node) {
    switch (node->index()) {
        case ast::Block: {
            auto& block = std::get<ast::BlockNode>(*node);

            // Add scope
            semantic::add_scope(context);

            // Add functions to the current scope
            auto result = semantic::add_definitions_to_current_scope(context, block);
            assert(result.is_ok());

            // Make statements concrete
            size_t number_of_errors = context.errors.size();
            for (auto statement: block.statements) {
                semantic::set_concrete_types(context, statement);
            }

            // Remove scope
            semantic::remove_scope(context);
            break;
        }

        case ast::Function: {
            break;
        }

        case ast::TypeDef: {
            break;
        }

        case ast::Declaration: {
            auto& declaration = std::get<ast::DeclarationNode>(*node);
            semantic::set_concrete_types(context, declaration.expression);
            break;
        }

        case ast::Assignment: {
            auto& assignment = std::get<ast::AssignmentNode>(*node);
            semantic::set_concrete_types(context, (ast::Node*) assignment.assignable);
            semantic::set_concrete_types(context, (ast::Node*) assignment.expression);
            break;
        }

        case ast::Return: {
            auto& return_node = std::get<ast::ReturnNode>(*node);
            if (return_node.expression.has_value()) {
                semantic::set_concrete_types(context, return_node.expression.value());
            }
            break;
        }

        case ast::Break: {
            break;
        }

        case ast::Continue: {
            break;
        }

        case ast::IfElse: {
            auto& if_else = std::get<ast::IfElseNode>(*node);
            semantic::set_concrete_types(context, if_else.condition);
            semantic::set_concrete_types(context, if_else.if_branch);
            if (if_else.else_branch.has_value()) {
                semantic::set_concrete_types(context, if_else.else_branch.value());
            }

            if (ast::is_expression(node)) {
                if_else.type = semantic::get_type_or_default(context, if_else.type);
            }
            break;
        }

        case ast::While: {
            auto& while_node = std::get<ast::WhileNode>(*node);
            semantic::set_concrete_types(context, while_node.condition);
            semantic::set_concrete_types(context, while_node.block);
            break;
        }

        case ast::Use: {
            assert(false);
            break;
        }

        case ast::LinkWith: {
            assert(false);
            break;
        }

        case ast::CallArgument: {
            auto& call_argument = std::get<ast::CallArgumentNode>(*node);
            semantic::set_concrete_types(context, call_argument.expression);
            call_argument.type = ast::get_type(call_argument.expression);
            break;
        }

        case ast::Call: {
            auto& call = std::get<ast::CallNode>(*node);
            for (auto arg: call.args) {
                semantic::set_concrete_types(context, (ast::Node*) arg);
            }
            
            call.type = semantic::get_type_or_default(context, call.type);
            break;
        }

        case ast::StructLiteral: {
            auto& struct_litereal = std::get<ast::StructLiteralNode>(*node);
            for (auto field: struct_litereal.fields) {
                semantic::set_concrete_types(context, field.second);
            }
            
            struct_litereal.type = semantic::get_type_or_default(context, struct_litereal.type);
            break;
        }

        case ast::Float: {
            auto& float_node = std::get<ast::FloatNode>(*node);
            float_node.type = semantic::get_type_or_default(context, float_node.type);
            break;
        }

        case ast::Integer: {
            auto& integer = std::get<ast::IntegerNode>(*node);
            integer.type = semantic::get_type_or_default(context, integer.type);
            break;
        }

        case ast::Identifier: {
            auto& identifier = std::get<ast::IdentifierNode>(*node);
            identifier.type = semantic::get_type_or_default(context, identifier.type);
            break;
        }

        case ast::Boolean: {
            break;
        }

        case ast::String: {
            break;
        }

        case ast::InterpolatedString: {
            auto& string = std::get<ast::InterpolatedStringNode>(*node);
            for (auto expression: string.expressions) {
                semantic::set_concrete_types(context, expression);
            }
            break;
        }

        case ast::Array: {
            auto& array =  std::get<ast::ArrayNode>(*node);
            for (auto element: array.elements) {
                semantic::set_concrete_types(context, element);
            }
            array.type = semantic::get_type_or_default(context, array.type);
            break;
        }

        case ast::FieldAccess: {
            auto& field_access = std::get<ast::FieldAccessNode>(*node);
            ast::set_type(field_access.accessed, semantic::get_type_or_default(context, ast::get_type(field_access.accessed)));
            for (auto field: field_access.fields_accessed) {
                field->type = semantic::get_type_or_default(context, field->type);
            }

            field_access.type = semantic::get_type_or_default(context, field_access.type);
            break;
        }

        case ast::AddressOf: {
            auto& address_of = std::get<ast::AddressOfNode>(*node);
            semantic::set_concrete_types(context, address_of.expression);
    
            assert(address_of.type.is_pointer());
            if (!address_of.type.is_concrete()) {
                address_of.type.as_nominal_type().parameters[0] = semantic::get_type_or_default(context, address_of.expression);
            }            
            break;
        }

        case ast::Dereference: {
            auto& dereference = std::get<ast::DereferenceNode>(*node);
            assert(semantic::get_type_or_default(context, dereference.expression).is_pointer()
            ||     semantic::get_type_or_default(context, dereference.expression).is_boxed());
            dereference.type = semantic::get_type_or_default(context, dereference.type);
            semantic::set_concrete_types(context, dereference.expression);
            break;
        }

        case ast::New: {
            auto& new_node = std::get<ast::NewNode>(*node);
            semantic::set_concrete_types(context, new_node.expression);
    
            assert(new_node.type.is_boxed());
            if (!new_node.type.is_concrete()) {
                new_node.type.as_nominal_type().parameters[0] = semantic::get_type_or_default(context, new_node.expression);
            }            
            break;
        }

        default: {
            assert(false);
        }
    }
}