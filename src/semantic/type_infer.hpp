#ifndef SEMANTIC_TYPE_INFER_HPP
#define SEMANTIC_TYPE_INFER_HPP

#include "context.hpp"

namespace semantic {
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::Node* node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::BlockNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::Type& type);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::TypeNode& node); 
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::AssignmentNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FieldAssignmentNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::DereferenceAssignmentNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::ReturnNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::BreakNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::ContinueNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::IfElseNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::WhileNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::UseNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::LinkWithNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::CallArgumentNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::CallNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::StructLiteralNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FloatNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::IntegerNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::IdentifierNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::BooleanNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::StringNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::ArrayNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FieldAccessNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::AddressOfNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::DereferenceNode& node);
}

#endif