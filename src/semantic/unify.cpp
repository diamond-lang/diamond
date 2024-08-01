#include "unify.hpp"
#include "semantic.hpp"
#include "check_functions_used.hpp"

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::Node* node) {
    return std::visit([&context, node](auto& variant) {return semantic::unify_types_and_type_check(context, variant);}, *node);
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BlockNode& block) {
    size_t errors = context.errors.size();

    // Add scope
    semantic::add_scope(context);

    // Add functions and types to the current scope
    auto result = semantic::add_definitions_from_block_to_scope(context, block);
    assert(result.is_ok());

    for (auto statement: block.statements) {
        auto result = semantic::unify_types_and_type_check(context, statement);
        if (result.is_error()) return result;

        if (statement->index() == ast::Call
        && !ast::get_type(statement).is_final_type_variable()) {
            if (ast::get_type(statement) != ast::Type("void")) {
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

    semantic::current_scope(context)[node.identifier->value] = semantic::Binding(&node);

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
        semantic::set_unified_type(context, node.type, ast::Type("int64"));
        node.type = ast::Type("int64");
    }
    

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FloatNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    if (node.type.is_final_type_variable() && (
       !context.current_function.has_value()
    || !context.current_function.value()->is_in_type_parameter(node.type))) {
        semantic::set_unified_type(context, node.type, ast::Type("float64"));
        node.type = ast::Type("float64");
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
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);
    
    if (all_args_typed) {
        auto call_type = get_function_type(context, binding->value, &node, ast::get_types(node.args), node.type).get_value();
        node.type = call_type;

        if (node.type.is_final_type_variable() && (
           !context.current_function.has_value()
        || !context.current_function.value()->is_in_type_parameter(node.type))) {
            assert(context.type_inference.interface_constraints.find(node.type) != context.type_inference.interface_constraints.end());
            semantic::set_unified_type(context, node.type, call_type);
            node.type = call_type;
        }
    }

    // Set functions that can be called
    std::vector<ast::FunctionNode*> functions_that_can_be_called;
    if (binding->type == FunctionBinding) {
        node.functions.push_back(semantic::get_function(*binding));
    }
    else if (binding->type == InterfaceBinding) {
        node.functions = semantic::get_interface(*binding)->functions;
    }

    if (node.identifier->value == "print" && node.args[0]->expression->index() == ast::InterpolatedString) {
        ast::InterpolatedStringNode& interpolated_string = std::get<ast::InterpolatedStringNode>(*node.args[0]->expression);
        for (auto expression: interpolated_string.expressions) {
            if (!ast::get_type(expression).is_final_type_variable()) {
                auto binding = semantic::get_binding(context, "printWithoutLineEnding");
                assert(binding);

                auto interface = semantic::get_interface(*binding);
                for (auto function: interface->functions) {
                    if (function->args[0]->type == ast::get_type(expression)) {
                        semantic::mark_function_as_used(context, function);
                    }
                }
            }
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
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

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
