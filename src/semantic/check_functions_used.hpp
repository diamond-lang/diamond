#ifndef SEMANTIC_CHECK_FUNCTIONS_USED_HPP
#define SEMANTIC_CHECK_FUNCTIONS_USED_HPP

#include "context.hpp"

namespace semantic {
    void check_functions_used(Context& context, ast::Node* node);
    void check_functions_used(Context& context, ast::BlockNode& node);
    void check_functions_used(Context& context, ast::FunctionArgumentNode& node);
    void check_functions_used(Context& context, ast::FunctionNode& node);
    void check_functions_used(Context& context, ast::InterfaceNode& node);
    void check_functions_used(Context& context, ast::TypeNode& node);
    void check_functions_used(Context& context, ast::DeclarationNode& node);
    void check_functions_used(Context& context, ast::AssignmentNode& node);
    void check_functions_used(Context& context, ast::ReturnNode& node);
    void check_functions_used(Context& context, ast::BreakNode& node);
    void check_functions_used(Context& context, ast::ContinueNode& node);
    void check_functions_used(Context& context, ast::IfElseNode& node);
    void check_functions_used(Context& context, ast::WhileNode& node);
    void check_functions_used(Context& context, ast::UseNode& node);
    void check_functions_used(Context& context, ast::LinkWithNode& node);
    void check_functions_used(Context& context, ast::CallArgumentNode& node);
    void check_functions_used(Context& context, ast::CallNode& node);
    void check_functions_used(Context& context, ast::StructLiteralNode& node);
    void check_functions_used(Context& context, ast::FloatNode& node);
    void check_functions_used(Context& context, ast::IntegerNode& node);
    void check_functions_used(Context& context, ast::IdentifierNode& node);
    void check_functions_used(Context& context, ast::BooleanNode& node);
    void check_functions_used(Context& context, ast::StringNode& node);
    void check_functions_used(Context& context, ast::InterpolatedStringNode& node);
    void check_functions_used(Context& context, ast::ArrayNode& node);
    void check_functions_used(Context& context, ast::FieldAccessNode& node);
    void check_functions_used(Context& context, ast::AddressOfNode& node);
    void check_functions_used(Context& context, ast::DereferenceNode& node);
    void check_functions_used(Context& context, ast::NewNode& node);
}

#endif