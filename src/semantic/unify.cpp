#include "unify.hpp"
#include "semantic.hpp"

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::Node* node) {
    return std::visit([&context, node](auto& variant) {return semantic::unify_types_and_type_check(context, variant);}, *node);
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BlockNode& block) {
    size_t errors = context.errors.size();

    // Add scope
    semantic::add_scope(context);

    // Add functions and types to the current scope
    auto result = semantic::add_definitions_to_current_scope(context, block);
    assert(result.is_ok());

    for (auto statement: block.statements) {
        auto result = semantic::unify_types_and_type_check(context, statement);
        if (result.is_error()) return result;
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
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return Error {};
    
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FieldAssignmentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, *node.identifier);
    if (result.is_error()) return result;
    
    result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::DereferenceAssignmentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, *node.identifier);
    if (result.is_error()) return result;

    result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::IndexAssignmentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, *node.index_access);
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
    if (ast::is_expression((ast::Node*) &node)) {
        node.type = semantic::get_unified_type(context, node.type);
    }
    auto result = semantic::unify_types_and_type_check(context, node.condition);
    if (result.is_error()) return result;

    result = semantic::unify_types_and_type_check(context, node.if_branch);
    if (result.is_error()) return result;

    if (node.else_branch.has_value()) return semantic::unify_types_and_type_check(context, node.else_branch.value());

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

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FloatNode& node) {
    node.type = semantic::get_unified_type(context, node.type);
    
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::CallNode& node) {
    for (size_t i = 0; i < node.args.size(); i++) {
        auto result = semantic::unify_types_and_type_check(context, *node.args[i]); 
        if (result.is_error()) return result;
    }

    node.type = semantic::get_unified_type(context, node.type);

    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);
    assert(binding->type == semantic::FunctionBinding);

    // Get overload constraints

    // Find functions that can be called
    // ---------------------------------
    std::vector<ast::FunctionNode*> functions_that_can_be_called = semantic::get_functions(*binding);

    // Remove functions whose number of arguments dont match with the call
    functions_that_can_be_called.erase(std::remove_if(functions_that_can_be_called.begin(), functions_that_can_be_called.end(), [&node](ast::FunctionNode* function) {
        return node.args.size() != function->args.size();
    }), functions_that_can_be_called.end());

    // Analyze functions that still have not been analyzed
    // and check for recursion
    for (auto function: functions_that_can_be_called) {
        if (function->state == ast::FunctionNotAnalyzed) {
            auto result = semantic::analyze(context, *function);
            if (result.is_error()) return result;
        }
        else if (function->state == ast::FunctionBeingAnalyzed) {
            node.functions = functions_that_can_be_called;
            return Ok {};
        }
    }

    // For each arg
    for (size_t i = 0; i < node.args.size(); i++) {
        if (node.args[i]->type.is_concrete()) {
            // Remove functions that can't be called
            auto backup = functions_that_can_be_called;
            functions_that_can_be_called = semantic::remove_incompatible_functions_with_argument_type(functions_that_can_be_called, i, node.args[i]->type, node.args[i]->is_mutable);

            // If there is no function that can be called
            if (functions_that_can_be_called.size() == 0) {
                std::vector<ast::Type> possible_types = semantic::get_possible_types_for_argument(backup, i);
                context.errors.push_back(errors::unexpected_argument_type(node, context.current_module, i,  node.args[i]->type, possible_types, {}));
                return Error{}; 
            }
        }
    }

    // For return type
    if (node.type.is_concrete()) {
        auto backup = functions_that_can_be_called;
        functions_that_can_be_called = semantic::remove_incompatible_functions_with_return_type(functions_that_can_be_called, node.type);

        if (functions_that_can_be_called.size() == 0) {
            std::vector<ast::Type> possible_types = semantic::get_possible_types_for_return_type(backup);
            context.errors.push_back(errors::unexpected_return_type(node, context.current_module, node.type, possible_types, {}));
            return Error{}; 
        }
    }
    node.functions = functions_that_can_be_called;
    assert(node.functions.size() > 0);
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

    node.type = ast::get_type(node.expression).as_nominal_type().parameters[0];
    return Ok {};
}
