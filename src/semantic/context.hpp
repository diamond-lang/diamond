#ifndef SEMANTIC_CONTEXT_HPP
#define SEMANTIC_CONTEXT_HPP

#include <cassert>
#include <filesystem>
#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>

#include "../ast.hpp"
#include "../errors.hpp"

namespace semantic {
    // Bindings
    enum BindingType {
        VariableBinding,
        FunctionArgumentBinding,
        FunctionBinding,
        InterfaceBinding,
        TypeBinding
    };

    struct Binding {
        BindingType type;
        ast::Node* value;

        Binding() {}
        ~Binding() {}
        Binding(ast::DeclarationNode* variable);
        Binding(ast::Node* function_argument);
        Binding(ast::FunctionNode* function);
        Binding(ast::InterfaceNode* interface);
        Binding(ast::TypeNode* type);
    };

    ast::DeclarationNode* get_variable(Binding binding);
    ast::Node* get_function_argument(Binding binding);
    ast::FunctionNode* get_function(Binding binding);
    ast::InterfaceNode* get_interface(Binding binding);
    ast::TypeNode* get_type_definition(Binding binding);
    ast::Type get_binding_type(Binding& binding);
    std::string get_binding_identifier(Binding& binding);

    // Scopes
    using Scopes = std::vector<std::unordered_map<std::string, Binding>>;

    // Type inference
    struct TypeInference {
        size_t current_type_variable_number = 1;
        std::vector<Set<ast::Type>> type_constraints;
        std::unordered_map<ast::Type, Set<ast::Type>> labeled_type_constraints;
        std::unordered_map<ast::Type, Set<ast::InterfaceType>> interface_constraints;
        std::unordered_map<ast::Type, ast::FieldTypes> field_constraints;
        std::unordered_map<ast::Type, std::vector<ast::Type>> parameter_constraints;
        std::unordered_map<std::string, ast::Type> type_bindings;
    };

    // Context
    struct Context {
        ast::Ast* ast;
        Errors errors;
        std::filesystem::path current_module;
        Scopes scopes;
        TypeInference type_inference;
        std::optional<ast::FunctionNode*> current_function = std::nullopt;

        void init_with(ast::Ast* ast);
    };

    // Manage scope
    void add_scope(Context& context);
    void remove_scope(Context& context);
    std::unordered_map<std::string, Binding>& current_scope(Context& context);
    Binding* get_binding(Context& context, std::string identifier);

    // Work with modules
    Result<Ok, Error> add_definitions_to_current_scope(Context& context, std::vector<ast::FunctionNode*>& functions, std::vector<ast::InterfaceNode*>& interfaces, std::vector<ast::TypeNode*>& types);
    Result<Ok, Error> add_definitions_from_block_to_scope(Context& context, ast::BlockNode& block);
    Result<Ok, Error> add_module_functions(Context& context, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules);
    std::vector<std::unordered_map<std::string, Binding>> get_definitions(Context& context);

    // For type infer and analyze
    ast::Type new_type_variable(Context& context);
    void add_constraint(Context& context, Set<ast::Type> constraint);
    void add_interface_constraint(Context& context, ast::Type type, ast::InterfaceType interface);
    void add_parameter_constraint(Context& context, ast::Type type, ast::Type parameter);

    // For unify and analyze
    ast::Type new_final_type_variable(Context& context);
    ast::Type get_unified_type(Context& context, ast::Type type_var);
    void set_unified_type(Context& context, ast::Type type_var, ast::Type new_type);

    // For unify and make concrete
    std::vector<ast::FunctionNode*> remove_incompatible_functions_with_argument_type(std::vector<ast::FunctionNode*> functions, size_t argument_position, ast::Type argument_type, bool is_mutable);
    std::vector<ast::FunctionNode*> remove_incompatible_functions_with_return_type(std::vector<ast::FunctionNode*> functions, ast::Type return_type);
    std::vector<ast::Type> get_possible_types_for_argument(std::vector<ast::FunctionNode*> functions, size_t argument_position);
    std::vector<ast::Type> get_possible_types_for_return_type(std::vector<ast::FunctionNode*> functions);
}

#endif