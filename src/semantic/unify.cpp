#include "unify.hpp"
#include "semantic.hpp"
#include "../semantic.hpp"

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::Node* node) {
    return std::visit([&context, node](auto& variant) {return semantic::unify_types_and_type_check(context, variant);}, *node);
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BlockNode& block) {
    size_t errors = context.errors.size();

    // Add scope
    auto result = semantic::add_scope(context, block);
    assert(result.is_ok());

    for (auto statement: block.statements) {
        auto result = semantic::unify_types_and_type_check(context, statement);
        if (result.is_error()) return result;

        if (statement->index() == ast::Call
        && !ast::get_type(statement).is_final_type_variable()) {
            if (ast::get_type(statement) != ast::Type("None")) {
                context.errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*statement), context.current_module));
                return Error{};
            }
        }
    }

    // Remove scope
    semantic::remove_scope(context);

    if (errors != context.errors.size()) {
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FunctionArgumentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, *node.identifier);
    if (result.is_error()) return result;
    node.type = node.identifier->type;

    return Ok{};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FunctionNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::InterfaceNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::TypeNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::DeclarationNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return Error {};

    semantic::current_scope(context).variables_scope[node.identifier->value] = semantic::Binding(&node);

    return Ok{};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::AssignmentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.assignable);
    if (result.is_error()) return result;
    
    result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::ReturnNode& node) {
    if (node.expression.has_value()) return semantic::unify_types_and_type_check(context, node.expression.value());

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BreakNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::ContinueNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::IfElseNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.condition);
    if (result.is_error()) return result;

    result = semantic::unify_types_and_type_check(context, node.if_branch);
    if (result.is_error()) return result;

    if (node.else_branch.has_value()) {
        result = semantic::unify_types_and_type_check(context, node.else_branch.value());
        if (result.is_error()) return result;
    }

    if (ast::is_expression((ast::Node*) &node)) {
        node.type = semantic::get_unified_type(context, node.type);
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::WhileNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.condition);
    if (result.is_error()) return result;
    
    result = semantic::unify_types_and_type_check(context, node.block);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::UseNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::LinkWithNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::CallArgumentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return Error{};
    node.type = ast::get_type(node.expression);
    return Ok{};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::IdentifierNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BooleanNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::StringNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::InterpolatedStringNode& node) {
    for (auto expression: node.expressions) {
        auto result = semantic::unify_types_and_type_check(context, expression);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::ArrayNode& node) {
    for (auto element: node.elements) {
        auto result = semantic::unify_types_and_type_check(context, element);
        if (result.is_error()) return result;
    }
    
    node.type = semantic::get_unified_type(context, node.type);
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::IntegerNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    if (node.type.is_final_type_variable() && (
       !context.current_function.has_value()
    || !context.current_function.value()->is_in_type_parameter(node.type))) {
        semantic::set_unified_type(context, node.type, ast::Type("Int64"));
        node.type = ast::Type("Int64");
    }
    

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FloatNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    if (node.type.is_final_type_variable() && (
       !context.current_function.has_value()
    || !context.current_function.value()->is_in_type_parameter(node.type))) {
        semantic::set_unified_type(context, node.type, ast::Type("Float64"));
        node.type = ast::Type("Float64");
    }
    
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::CallNode& node) {
    bool all_args_typed = true;
    for (size_t i = 0; i < node.args.size(); i++) {
        auto result = semantic::unify_types_and_type_check(context, *node.args[i]); 
        if (result.is_error()) return result;

        if (!node.args[i]->type.is_concrete()) {
            all_args_typed = false;
        }
    }

    node.type = semantic::get_unified_type(context, node.type);

    // Get binding
    std::optional<semantic::Binding> binding = semantic::get_binding(context, node.identifier->value);
    assert(binding.has_value());
    
    if (all_args_typed) {
        auto call_type = get_function_type(context, binding.value().value, node.get_args_mutability(), ast::get_types(node.args), node.type).get_value();
        node.type = call_type;

        if (node.type.is_final_type_variable() && (
           !context.current_function.has_value()
        || !context.current_function.value()->is_in_type_parameter(node.type))) {
            assert(context.type_inference.interface_constraints.find(node.type) != context.type_inference.interface_constraints.end());
            semantic::set_unified_type(context, node.type, call_type);
            node.type = call_type;
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::StructLiteralNode& node) {
    for (auto field: node.fields) {
        auto result = semantic::unify_types_and_type_check(context, field.second); 
        if (result.is_error()) return result;
    }

    node.type = semantic::get_unified_type(context, node.type);

    // Get binding
    std::optional<semantic::Binding> binding = semantic::get_binding(context, node.identifier->value);
    assert(binding.has_value());

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 1);

    if (!node.type.is_type_variable()) {
        return Ok {};
    }
    else {
        ast::FieldTypes* field_constraints = &context.type_inference.field_constraints[ast::get_type(node.accessed)];

        auto result = semantic::unify_types_and_type_check(context, node.accessed);
        if (result.is_error()) return result;

        if (ast::get_type(node.accessed).is_concrete()) {
            ast::Type struct_type = ast::get_type(node.accessed);

            for (size_t i = 0; i < node.fields_accessed.size(); i++) {
                auto field = node.fields_accessed[i]->value;
                assert(struct_type.is_nominal_type());

                bool field_founded = false;
                for (auto field_in_definition: struct_type.as_nominal_type().type_definition->fields) {
                    if (field == field_in_definition->value) {
                        field_founded = true;
                        node.fields_accessed[i]->type = semantic::get_unified_type(context, (*field_constraints)[field]);
                        
                        if (node.fields_accessed[i]->type.is_final_type_variable() && (
                          !context.current_function.has_value()
                        || context.current_function.value()->typed_parameter_aready_added(node.type))) {
                            semantic::set_unified_type(context, node.type, field_in_definition->type);
                            node.fields_accessed[i]->type = field_in_definition->type;
                        }
                        break;
                    }
                }
                assert(field_founded);

                // Iterate over struct type
                if (i + 1 != node.fields_accessed.size()) {
                    struct_type = node.fields_accessed[i]->type;
                }
            }
        }
        else {
            for (size_t i = 0; i < node.fields_accessed.size(); i++) {
                std::string field = node.fields_accessed[i]->value;
                ast::Type current_type = node.fields_accessed[i]->type;

                // Get unified type
                node.fields_accessed[i]->type = semantic::get_unified_type(context, (*field_constraints)[field]);

                // Iterate
                if (i != node.fields_accessed.size() - 1) {
                    field_constraints = &context.type_inference.field_constraints[current_type];
                }
            }
        }

        // Unify overall node type
        node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;
    }

    return Ok {};
}


Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::AddressOfNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    node.type = semantic::get_unified_type(context, node.type);
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::DereferenceNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    if (ast::get_type(node.expression).is_concrete()) {
        assert(ast::get_type(node.expression).is_pointer() || ast::get_type(node.expression).is_boxed());
        node.type = ast::get_type(node.expression).as_nominal_type().parameters[0];
    }
    else {
        node.type = semantic::get_unified_type(context, node.type);
    }
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::NewNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    node.type = semantic::get_unified_type(context, node.type);
    return Ok {};
}
