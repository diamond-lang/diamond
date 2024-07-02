#ifndef SEMANTIC_MAKE_CONCRETE_HPP
#define SEMANTIC_MAKE_CONCRETE_HPP

#include "context.hpp"

namespace semantic {
    bool is_type_concrete(Context& context, ast::Type type);
    ast::Type get_type(Context& context, ast::Type type);
    ast::Type get_type_or_default(Context& context, ast::Type type);
    ast::Type get_type_or_default(Context& context, ast::Node* node);;
    std::vector<ast::Type> get_types_or_default(Context& context, std::vector<ast::Type> type_variables);
    std::vector<ast::Type> get_types_or_default(Context& context, std::vector<ast::Node*> nodes);

    Result<Ok, Error> make_concrete(Context& context, ast::Node* node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::BlockNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::FunctionArgumentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::FunctionNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::TypeNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::DeclarationNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::AssignmentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::ReturnNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::BreakNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::ContinueNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::IfElseNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::WhileNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::UseNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::LinkWithNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::CallArgumentNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::StructLiteralNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::FloatNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::IntegerNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::IdentifierNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::BooleanNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::StringNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::InterpolatedStringNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::ArrayNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::FieldAccessNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::AddressOfNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::DereferenceNode& node, std::vector<ast::CallInCallStack> call_stack);
    Result<Ok, Error> make_concrete(Context& context, ast::NewNode& node, std::vector<ast::CallInCallStack> call_stack);
    
   Result<Ok, Error> unify_types(std::unordered_map<std::string, ast::Type>& type_bindings, ast::Type function_type, ast::Type argument_type);
    Result<ast::Type, Error> get_type_completely_typed_function(Context& context, ast::FunctionNode* function, std::vector<ast::Type> args);
    Result<ast::Type, Error> get_function_type(Context& context, ast::CallNode* call, ast::FunctionNode* function, std::vector<ast::Type> args, std::vector<ast::CallInCallStack> call_stack);
    
    void set_concrete_types(Context& context, ast::Node* node);
}

#endif