#include "check_functions_used.hpp"
#include "semantic.hpp"

void semantic::check_functions_used(Context& context, ast::Node* node) {
    assert(node);
    std::visit([&context, node](auto& variant) {
        semantic::check_functions_used(context, variant);
    }, *node);
}

void semantic::check_functions_used(Context& context, ast::BlockNode& node) {
    // Add scope
    semantic::add_scope(context);

    // Add functions to the current scope
    semantic::add_definitions_from_block_to_scope(context, node);

    // Make statements concrete
    size_t number_of_errors = context.errors.size();
    for (auto statement: node.statements) {
        semantic::check_functions_used(context, statement);
    }

    // Remove scope
    semantic::remove_scope(context);
}

void semantic::check_functions_used(Context& context, ast::FunctionArgumentNode& node) {
    assert(false);
}

void semantic::check_functions_used(Context& context, ast::FunctionNode& node) {
    assert(false);
}

void semantic::check_functions_used(Context& context, ast::InterfaceNode& node) {
    assert(false);
}

void semantic::check_functions_used(Context& context, ast::TypeNode& node) {
}

void semantic::check_functions_used(Context& context, ast::DeclarationNode& node) {
    semantic::check_functions_used(context, node.expression);
}

void semantic::check_functions_used(Context& context, ast::AssignmentNode& node) {
    semantic::check_functions_used(context, node.assignable);
    semantic::check_functions_used(context, node.expression);
}

void semantic::check_functions_used(Context& context, ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        semantic::check_functions_used(context, node.expression.value());
    }

}

void semantic::check_functions_used(Context& context, ast::BreakNode& node) {
}

void semantic::check_functions_used(Context& context, ast::ContinueNode& node) {
}

void semantic::check_functions_used(Context& context, ast::IfElseNode& node) {
    semantic::check_functions_used(context, node.condition);

    semantic::check_functions_used(context, node.if_branch);
    
    if (node.else_branch.has_value()) {
        semantic::check_functions_used(context, node.else_branch.value());

    }

}

void semantic::check_functions_used(Context& context, ast::WhileNode& node) {
    semantic::check_functions_used(context, node.condition);
    semantic::check_functions_used(context, node.block);
}

void semantic::check_functions_used(Context& context, ast::UseNode& node) {
}

void semantic::check_functions_used(Context& context, ast::LinkWithNode& node) {
}

void semantic::check_functions_used(Context& context, ast::CallArgumentNode& node) {
    semantic::check_functions_used(context, node.expression);
}

void semantic::check_functions_used(Context& context, ast::CallNode& node) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);
    assert(binding->type == semantic::FunctionBinding || binding->type == semantic::InterfaceBinding); 
    for (auto arg: node.args) {
        semantic::check_functions_used(context, *arg);
    }

    auto args_types = ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings);
    auto call_type = ast::get_concrete_type(node.type, context.type_inference.type_bindings);
    auto function_type = semantic::get_function_type(context, binding->value, &node, args_types, call_type);
}


void semantic::check_functions_used(Context& context, ast::StructLiteralNode& node) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    for (auto field: node.fields) {
        semantic::check_functions_used(context, field.second);
    }
    node.type = ast::Type(node.identifier->value, semantic::get_type_definition(*binding));
}

void semantic::check_functions_used(Context& context, ast::FloatNode& node) {
}

void semantic::check_functions_used(Context& context, ast::IntegerNode& node) {
}

void semantic::check_functions_used(Context& context, ast::IdentifierNode& node) {
}

void semantic::check_functions_used(Context& context, ast::BooleanNode& node) {
}

void semantic::check_functions_used(Context& context, ast::StringNode& node) {
}

void semantic::check_functions_used(Context& context, ast::InterpolatedStringNode& node) {
    for (auto expression: node.expressions) {
        semantic::check_functions_used(context, expression);
    }
}

void semantic::check_functions_used(Context& context, ast::ArrayNode& node) {
    for (auto element: node.elements) {
        check_functions_used(context, element);
    }
}

void semantic::check_functions_used(Context& context, ast::FieldAccessNode& node) {
}

void semantic::check_functions_used(Context& context, ast::AddressOfNode& node) {
    semantic::check_functions_used(context, node.expression);

}

void semantic::check_functions_used(Context& context, ast::DereferenceNode& node) {
    check_functions_used(context, node.expression);
    
}

void semantic::check_functions_used(Context& context, ast::NewNode& node) {
    semantic::check_functions_used(context, node.expression);
}