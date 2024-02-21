#ifndef SEMANTIC_UNIFY_HPP
#define SEMANTIC_UNIFY_HPP

#include "context.hpp"

namespace semantic {
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::Node* node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::BlockNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::TypeNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::AssignmentNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FieldAssignmentNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::DereferenceAssignmentNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::ReturnNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::BreakNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::ContinueNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::IfElseNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::WhileNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::UseNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::LinkWithNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::CallArgumentNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::CallNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FloatNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::IntegerNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::IdentifierNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::BooleanNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::StringNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::ArrayNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FieldAccessNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::AddressOfNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::DereferenceNode& node);
}

#endif