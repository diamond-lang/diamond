#ifndef SEMANTIC_MAKE_CONCRETE_HPP
#define SEMANTIC_MAKE_CONCRETE_HPP

#include "context.hpp"

namespace semantic {
    bool is_type_concrete(Context& context, ast::Type type);
    ast::Type get_type(Context& context, ast::Type type);

    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::Node* node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::BlockNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::FunctionNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::TypeNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::AssignmentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::FieldAssignmentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::DereferenceAssignmentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::ReturnNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::BreakNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::ContinueNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::IfElseNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::WhileNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::UseNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::LinkWithNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::CallArgumentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::FloatNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::IntegerNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::IdentifierNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::BooleanNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::StringNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::ArrayNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::FieldAccessNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::AddressOfNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> get_concrete_as_type_bindings(Context& context, ast::DereferenceNode& node, std::vector<ast::CallInCallStack> call_stack);
    
    Result<ast::Type, Error> get_type_completly_typed_function(ast::FunctionNode* function, std::vector<ast::Type> args);
    Result<ast::Type, Error> get_function_type(Context& context, ast::CallNode* call, ast::FunctionNode* function, std::vector<ast::Type> args, std::vector<ast::CallInCallStack> call_stack);
    
    void make_concrete(Context& context, ast::Node* node);
}

#endif