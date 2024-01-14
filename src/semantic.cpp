#include <unordered_map>
#include <cassert>
#include <filesystem>
#include <set>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "ast.hpp"
#include "errors.hpp"
#include "semantic.hpp"
#include "utilities.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "intrinsics.hpp"
#include "data_structures.hpp"

namespace semantic {
    // Bindings
    enum BindingType {
        AssignmentBinding,
        FunctionArgumentBinding,
        FunctionBinding,
        TypeBinding
    };

    struct Binding {
        BindingType type;
        std::vector<ast::Node*> value;
    };

    Binding make_Binding(ast::AssignmentNode* assignment);
    Binding make_Binding(ast::Node* function_argument);
    Binding make_Binding(std::vector<ast::FunctionNode*> functions);
    Binding make_Binding(ast::TypeNode* type);

    ast::AssignmentNode* get_assignment(Binding binding);
    ast::Node* get_function_argument(Binding binding);
    std::vector<ast::FunctionNode*> get_functions(Binding binding);
    ast::TypeNode* get_type_definition(Binding binding);
    ast::Type get_binding_type(Binding& binding);
    std::string get_binding_identifier(Binding& binding);
    bool is_function(Binding& binding);

    // Scopes
    using Scopes = std::vector<std::unordered_map<std::string, Binding>>;

    // Type inference
    struct TypeInference {
        size_t current_type_variable_number = 1;
        std::vector<Set<ast::Type>> type_constraints;
        std::unordered_map<ast::Type, Set<ast::Type>> labeled_type_constraints;
        std::unordered_map<ast::Type, Set<ast::Type>> overload_constraints;
        std::unordered_map<ast::Type, std::unordered_map<std::string, ast::Type>> field_constraints;
        std::unordered_map<size_t, ast::Type> type_bindings;
    };

    // Context
    struct Context {
        ast::Ast* ast;
        Errors errors;
        std::filesystem::path current_module;
        Scopes scopes;
        TypeInference type_inference;
        std::optional<ast::FunctionNode*> current_function = std::nullopt;
    };

    void init_Context(Context& context, ast::Ast* ast);
    void add_scope(Context& context);
    void remove_scope(Context& context);
    std::unordered_map<std::string, Binding>& current_scope(Context& context);
    Binding* get_binding(Context& context, std::string identifier);
    Result<Ok, Error> add_definitions_to_current_scope(Context& context, ast::BlockNode& block);
    Result<Ok, Error> add_module_functions(Context& context, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules);
    std::vector<std::unordered_map<std::string, Binding>> get_definitions(Context& context);
    Result<Ok, Error> add_type_binding(Context& context, std::unordered_map<size_t, ast::Type>& type_bindings, ast::Type binding, ast::Type type);
    ast::Type new_type_variable(Context& context);
    ast::Type new_final_type_variable(Context& context);
    void add_constraint(Context& context, Set<ast::Type> constraint);
    ast::Type get_unified_type(Context& context, ast::Type type_var);
    bool is_type_concrete(Context& context, ast::Type type);
    ast::Type get_type(Context& context, ast::Type type);
    bool are_types_compatible(ast::Type function_type, ast::Type argument_type);
    std::vector<ast::FunctionNode*> remove_incompatible_functions_with_argument_type(std::vector<ast::FunctionNode*> functions, size_t argument_position, ast::Type argument_type);
    std::vector<ast::FunctionNode*> remove_incompatible_functions_with_return_type(std::vector<ast::FunctionNode*> functions, ast::Type return_type);
    std::vector<ast::Type> get_possible_types_for_argument(std::vector<ast::FunctionNode*> functions, size_t argument_position);
    std::vector<ast::Type> get_possible_types_for_return_type(std::vector<ast::FunctionNode*> functions);
    Result<Ok, Error> set_expected_types_of_arguments_and_check(Context& context, ast::CallNode* call, ast::FunctionNode* called_function, size_t from_argument, std::vector<ast::CallInCallStack> call_stack);

    // Semantic analysis
    Result<Ok, Error> analyze_block_or_expression(Context& context, ast::Node* node);
    Result<Ok, Error> analyze(Context& context, ast::BlockNode& node);
    Result<Ok, Error> analyze(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> analyze(Context& context, ast::Type& type);
    Result<Ok, Error> analyze(Context& context, ast::TypeNode& node);

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
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FloatNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::IntegerNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::IdentifierNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::BooleanNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::StringNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::ArrayNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FieldAccessNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::AddressOfNode& node);
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::DereferenceNode& node);

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
    
    Result<ast::Type, Error> get_function_type(Context& context, ast::CallNode* call, ast::FunctionNode* function, std::vector<ast::Type> args, std::vector<ast::CallInCallStack> call_stack);
    
    void make_concrete(Context& context, ast::Node* node);
}

// Binding
// -------
semantic::Binding semantic::make_Binding(ast::AssignmentNode* assignment) {
    semantic::Binding binding;
    binding.type = semantic::AssignmentBinding;
    binding.value.push_back((ast::Node*) assignment);
    return binding;
}

semantic::Binding semantic::make_Binding(ast::Node* function_argument) {
    semantic::Binding binding;
    binding.type = semantic::FunctionArgumentBinding;
    binding.value.push_back((ast::Node*) function_argument);
    return binding;
}

semantic::Binding semantic::make_Binding(std::vector<ast::FunctionNode*> functions) {
    semantic::Binding binding;
    binding.type = semantic::FunctionBinding;
    for (size_t i = 0; i < functions.size(); i++) {
        binding.value.push_back((ast::Node*) functions[i]);
    }
    return binding;
}

semantic::Binding semantic::make_Binding(ast::TypeNode* type) {
    semantic::Binding binding;
    binding.type = semantic::TypeBinding;
    binding.value.push_back((ast::Node*) type);
    return binding;
}

ast::AssignmentNode* semantic::get_assignment(semantic::Binding binding) {
    assert(binding.type == semantic::AssignmentBinding);
    return (ast::AssignmentNode*)binding.value[0];
}

ast::Node* semantic::get_function_argument(semantic::Binding binding) {
    assert(binding.type == semantic::FunctionArgumentBinding);
    return binding.value[0];
}

std::vector<ast::FunctionNode*> semantic::get_functions(semantic::Binding binding) {
    assert(binding.type == semantic::FunctionBinding);
    std::vector<ast::FunctionNode*> functions;
    for (auto& function: binding.value) {
        functions.push_back((ast::FunctionNode*)function);
    }
    return functions;
}

ast::TypeNode* semantic::get_type_definition(semantic::Binding binding) {
    assert(binding.type == semantic::TypeBinding);
    return (ast::TypeNode*) binding.value[0];
}

ast::Type semantic::get_binding_type(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return ast::get_type(((ast::AssignmentNode*)binding.value[0])->expression);
        case semantic::FunctionArgumentBinding: return ast::get_type(binding.value[0]);
        case semantic::FunctionBinding: return ast::Type("function");
        case semantic::TypeBinding: return ast::Type("type");
    }
    assert(false);
    return ast::Type(ast::NoType{});
}

std::string semantic::get_binding_identifier(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return ((ast::AssignmentNode*)binding.value[0])->identifier->value;
        case semantic::FunctionArgumentBinding: return ((ast::IdentifierNode*)binding.value[0])->value;
        case semantic::FunctionBinding: return ((ast::FunctionNode*)binding.value[0])->identifier->value;
        case semantic::TypeBinding: return ((ast::TypeNode*)binding.value[0])->identifier->value;
    }
    assert(false);
    return "";
}

bool semantic::is_function(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return false;
        case semantic::FunctionArgumentBinding: return false;
        case semantic::FunctionBinding: return true;
        case semantic::TypeBinding: return false;
    }
    assert(false);
    return false;
}

// Context
// -------
void semantic::init_Context(semantic::Context& context, ast::Ast* ast) {
    context.current_module = ast->module_path;
    context.ast = ast;

    // Add intrinsic functions
    semantic::add_scope(context);

    for (auto it = intrinsic_functions.begin(); it != intrinsic_functions.end(); it++) {
        auto& identifier = it->first;
        auto& scope = current_scope(context);
        std::vector<ast::FunctionNode*> overloaded_functions;
        for (auto& prototype: it->second) {
            // Create function node
            auto function_node = ast::FunctionNode {};
            function_node.state = ast::FunctionCompletelyTyped;

            // Create identifier node
            auto identifier_node = ast::IdentifierNode {};
            identifier_node.value = identifier;
            ast->push_back(identifier_node);

            function_node.identifier = (ast::IdentifierNode*) ast->last_element();

            // Create args nodes
            for (auto& arg: prototype.first) {
                auto arg_node = ast::IdentifierNode {};
                arg_node.type = arg;

                ast->push_back(arg_node);
                function_node.args.push_back((ast::FunctionArgumentNode*) ast->last_element());
            }

            function_node.return_type = prototype.second;
            ast->push_back(function_node);
            overloaded_functions.push_back((ast::FunctionNode*) ast->last_element());
        }
        scope[identifier] = semantic::make_Binding(overloaded_functions);
    }
}

void semantic::add_scope(Context& context) {
    context.scopes.push_back(std::unordered_map<std::string, semantic::Binding>());
}

void semantic::remove_scope(Context& context) {
    context.scopes.pop_back();
}

std::unordered_map<std::string, semantic::Binding>& semantic::current_scope(Context& context) {
    return context.scopes[context.scopes.size() - 1];
}

semantic::Binding* semantic::get_binding(Context& context, std::string identifier) {
    for (auto scope = context.scopes.rbegin(); scope != context.scopes.rend(); scope++) {
        if (scope->find(identifier) != scope->end()) {
            return &(*scope)[identifier];
        }
    }
    return nullptr;
}

Result<Ok, Error> semantic::add_definitions_to_current_scope(Context& context, ast::BlockNode& block) {
    // Add functions from block to current scope
    for (auto& function: block.functions) {
        auto& identifier = function->identifier->value;
        auto& scope = semantic::current_scope(context);

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = semantic::make_Binding(std::vector{function});
        }
        else if (is_function(scope[identifier])) {
            scope[identifier].value.push_back((ast::Node*) function);
        }
        else {
            assert(false);
        }
    }

    // Add types from current block to current scope
    for (auto& type: block.types) {
        auto& identifier = type->identifier->value;
        auto& scope = semantic::current_scope(context);

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = make_Binding(type);
        }
        else {
            context.errors.push_back(errors::generic_error(Location{type->line, type->column, type->module_path}, "This type is already defined."));
            return Error {};
        }
    }

    // Add functions from modules
    auto current_directory = context.current_module.parent_path();
    std::set<std::filesystem::path> already_included_modules = {context.current_module};

    for (auto& use_stmt: block.use_statements) {
        auto module_path = std::filesystem::canonical(current_directory / (use_stmt->path->value + ".dmd"));
        assert(std::filesystem::exists(module_path));
        auto result = semantic::add_module_functions(context, module_path, already_included_modules);
        if (result.is_error()) return result;
    }

    return Ok {};
}

Result<Ok, Error> semantic::add_module_functions(Context& context, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules) {
    if (context.ast->modules.find(module_path.string()) == context.ast->modules.end()) {
        // Read file
        Result<std::string, Error> result = utilities::read_file(module_path.string());
        if (result.is_error()) {
            std::cout << result.get_error().value;
            exit(EXIT_FAILURE);
        }
        std::string file = result.get_value();

        // Lex
        auto tokens = lexer::lex(module_path);
        if (tokens.is_error()) {
            for (size_t i = 0; i < tokens.get_error().size(); i++) {
                std::cout << tokens.get_error()[i].value << "\n";
            }
            exit(EXIT_FAILURE);
        }

        // Parse module and add it to the ast
        auto parsing_result = parse::module(*context.ast, tokens.get_value(), module_path);
        if (parsing_result.is_error()) {
            std::vector<Error> errors = parsing_result.get_errors();
            for (size_t i = 0; i < errors.size(); i++) {
                std::cout << errors[i].value << '\n';
            }
            exit(EXIT_FAILURE);
        }
    }

    if (already_included_modules.find(module_path) == already_included_modules.end()) {
        // Add functions from current module to current context
        for (auto& function: context.ast->modules[module_path.string()]->functions) {
            auto& identifier = function->identifier->value;
            auto& scope = semantic::current_scope(context);

            if (scope.find(identifier) == scope.end()) {
                scope[identifier] = semantic::make_Binding(std::vector{function});
            }
            else if (is_function(scope[identifier])) {
                scope[identifier].value.push_back((ast::Node*) function);
            }
            else {
                assert(false);
            }
        }

        // Add types from current block to current scope
        for (auto& type: context.ast->modules[module_path.string()]->types) {
            auto& identifier = type->identifier->value;
            auto& scope = semantic::current_scope(context);

            if (scope.find(identifier) == scope.end()) {
                scope[identifier] = semantic::make_Binding(type);
            }
            else {
                context.errors.push_back(errors::generic_error(Location{type->line, type->column, type->module_path}, "This type is already defined in current parseModule."));
                return Error {};
            }
        }

        already_included_modules.insert(module_path);

        // Add includes
        for (auto& use_stmt: context.ast->modules[module_path.string()]->use_statements) {
            if (use_stmt->include) {
                auto include_path = std::filesystem::canonical(module_path.parent_path() / (use_stmt->path->value + ".dmd"));
                auto result = semantic::add_module_functions(context, include_path, already_included_modules);
                if (result.is_error()) return result;
            }
        }
    }

    return Ok {};
}

std::vector<std::unordered_map<std::string, semantic::Binding>> semantic::get_definitions(Context& context) {
    std::vector<std::unordered_map<std::string, Binding>> scopes;
    for (size_t i = 0; i < context.scopes.size(); i++) {
        scopes.push_back(std::unordered_map<std::string, Binding>());
        for (auto it = context.scopes[i].begin(); it != context.scopes[i].end(); it++) {
            if (semantic::is_function(it->second)) {
                scopes[i][it->first] = it->second;
            }
            else if (it->second.type == semantic::TypeBinding) {
                scopes[i][it->first] = it->second;
            }
        }
    }
    return scopes;
}

Result<Ok, Error> semantic::add_type_binding(Context& context, std::unordered_map<size_t, ast::Type>& type_bindings, ast::Type binding, ast::Type type) {
    assert(binding.is_type_variable());
    type_bindings[binding.as_type_variable().id] = type;
    return Ok {};
}

ast::Type semantic::new_type_variable(Context& context) {
    ast::Type new_type = ast::Type(ast::TypeVariable(context.type_inference.current_type_variable_number));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

ast::Type semantic::new_final_type_variable(Context& context) {
    ast::Type new_type = ast::Type(ast::TypeVariable(context.type_inference.current_type_variable_number, true));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

void semantic::add_constraint(Context& context, Set<ast::Type> constraint) {
    context.type_inference.type_constraints.push_back(constraint);
}

ast::Type semantic::get_unified_type(Context& context, ast::Type type_var) {
    for (auto it = context.type_inference.labeled_type_constraints.begin(); it != context.type_inference.labeled_type_constraints.end(); it++) {
        if (it->second.contains(type_var)) {
            type_var = it->first;

            if (type_var.is_nominal_type()) {
                for (size_t i = 0; i < type_var.as_nominal_type().parameters.size(); i++) {
                    type_var.as_nominal_type().parameters[i] = semantic::get_unified_type(context, type_var.as_nominal_type().parameters[i]);
                }
            }
            else if (type_var.is_struct_type()) {
                for (auto field: type_var.as_struct_type().fields) {
                    type_var.as_struct_type().fields[field.first] = semantic::get_unified_type(context, field.second);
                } 
            }

            return type_var;
        }
    }

    if (type_var.is_type_variable() && !type_var.as_type_variable().is_final) {
        std::optional<ast::Interface> interface = std::nullopt;
        if (type_var.as_type_variable().interface.has_value()) {
            interface = type_var.as_type_variable().interface;
        }
        ast::Type new_type_var = semantic::new_final_type_variable(context);
        new_type_var.as_type_variable().interface = interface;
        context.type_inference.labeled_type_constraints[new_type_var] = Set<ast::Type>({type_var});
        new_type_var.as_type_variable().interface = type_var.as_type_variable().interface;
        return new_type_var;
    }
    else if (!type_var.is_type_variable()) {
        for (size_t i = 0; i < type_var.as_nominal_type().parameters.size(); i++) {
            type_var.as_nominal_type().parameters[i] = semantic::get_unified_type(context, type_var.as_nominal_type().parameters[i]);
        }
        return type_var;
    }
    
    return type_var;
}

bool semantic::is_type_concrete(Context& context, ast::Type type) {
    if (type.is_concrete()) {
        return true;
    }
    if (type.is_type_variable()
    &&  context.type_inference.type_bindings.find(type.as_type_variable().id) != context.type_inference.type_bindings.end()) {
        return true;
    }
    return false;
}

ast::Type semantic::get_type(Context& context, ast::Type type) {
    if (type.is_type_variable()) {
        if (context.type_inference.type_bindings.find(type.as_type_variable().id) != context.type_inference.type_bindings.end()) {
            type = context.type_inference.type_bindings[type.as_type_variable().id];
        }
        else {
            return type;
        }
    }
    if (!type.is_concrete()) {
        for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
            type.as_nominal_type().parameters[i] = ast::get_concrete_type(type.as_nominal_type().parameters[i], context.type_inference.type_bindings);
        }
    }
    return type;
}

bool semantic::are_types_compatible(ast::Type function_type, ast::Type argument_type) {
    if (!function_type.is_type_variable()) {
        return argument_type == function_type;
    }
    else if (function_type.is_type_variable()
    &&       function_type.as_type_variable().overload_constraints.size() > 0) {
        return function_type.as_type_variable().overload_constraints.contains(argument_type);
    }
    return true;
}

std::vector<ast::FunctionNode*> semantic::remove_incompatible_functions_with_argument_type(std::vector<ast::FunctionNode*> functions, size_t argument_position, ast::Type argument_type) {
    assert(argument_type.is_concrete());
    
    functions.erase(std::remove_if(functions.begin(), functions.end(), [&argument_position, &argument_type](ast::FunctionNode* function) {
        return semantic::are_types_compatible(function->args[argument_position]->type, argument_type) == false;
    }), functions.end());

    return functions;
}

std::vector<ast::FunctionNode*> semantic::remove_incompatible_functions_with_return_type(std::vector<ast::FunctionNode*> functions, ast::Type return_type) {
    assert(return_type.is_concrete());
    
    functions.erase(std::remove_if(functions.begin(), functions.end(), [&return_type](ast::FunctionNode* function) {
        return semantic::are_types_compatible(function->return_type, return_type) == false;
    }), functions.end());

    return functions;
}

std::vector<ast::Type> semantic::get_possible_types_for_argument(std::vector<ast::FunctionNode*> functions, size_t argument_position) {
    std::vector<ast::Type> possible_types;
    for (auto function: functions) {
        if (!function->args[argument_position]->type.is_type_variable()) {
            possible_types.push_back(function->args[argument_position]->type);
        }
        else if (function->args[argument_position]->type.is_type_variable()
        &&       function->args[argument_position]->type.as_type_variable().overload_constraints.size() > 0) {
            for (auto type: function->args[argument_position]->type.as_type_variable().overload_constraints.elements) {
                possible_types.push_back(type);
            }
        }
        else {
            assert(false);
        }
    }
    return possible_types;
}

std::vector<ast::Type> semantic::get_possible_types_for_return_type(std::vector<ast::FunctionNode*> functions) {
    std::vector<ast::Type> possible_types;
    for (auto function: functions) {
        if (!function->return_type.is_type_variable()) {
            possible_types.push_back(function->return_type);
        }
        else if (function->return_type.is_type_variable()
        &&       function->return_type.as_type_variable().overload_constraints.size() > 0) {
            for (auto type: function->return_type.as_type_variable().overload_constraints.elements) {
                possible_types.push_back(type);
            }
        }
        else {
            assert(false);
        }
    }
    return possible_types;
}

Result<Ok, Error> semantic::set_expected_types_of_arguments_and_check(Context& context, ast::CallNode* call, ast::FunctionNode* called_function, size_t from_argument, std::vector<ast::CallInCallStack> call_stack) {
    // Set and check expected types
    for (size_t i = from_argument; i < call->args.size(); i++) {
        auto arg_type = semantic::get_type(context, call->args[i]->type);
        if (!called_function->args[i]->type.is_concrete()) {
            if (arg_type.is_type_variable()
            &&  arg_type.as_type_variable().interface.has_value()) {
                arg_type = arg_type.as_type_variable().interface.value().get_default_type();
            } 
        }

        // If type is concrete
        if (arg_type.is_concrete()) {
            // If type dont match with expected type
            if (!called_function->args[i]->type.is_type_variable()
            &&  arg_type != called_function->args[i]->type) {
                arg_type = ast::get_concrete_type(call->args[i]->type, context.type_inference.type_bindings);
                context.errors.push_back(errors::unexpected_argument_type(*call, context.current_module, i, arg_type, {called_function->args[i]->type}, call_stack));
                return Error {};
            }
            else if (called_function->args[i]->type.is_type_variable()
            && !called_function->args[i]->type.as_type_variable().overload_constraints.contains(arg_type)) {
                arg_type = ast::get_concrete_type(call->args[i]->type, context.type_inference.type_bindings);
                context.errors.push_back(errors::unexpected_argument_type(*call, context.current_module, i, arg_type, called_function->args[i]->type.as_type_variable().overload_constraints.elements, call_stack));
                return Error {};
            }
            else {
                // Check if everything typechecks
                auto result = get_concrete_as_type_bindings(context, *call->args[i], call_stack);
                if (result.is_error()) return Error{};
            }
        }
        else {
            // Set expected type
            if (called_function->args[i]->type.is_concrete()) {
                context.type_inference.type_bindings[call->args[i]->type.as_type_variable().id] = called_function->args[i]->type;
            }

            // Check if it works
            auto result = get_concrete_as_type_bindings(context, *call->args[i], call_stack);
            if (result.is_error()) return Error{};
        }
    }

    return Ok {};
}

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
    semantic::Context context;
    init_Context(context, &ast);

    semantic::analyze(context, *ast.program);

    if (context.errors.size() > 0) return context.errors;
    else                           return Ok {};
}

Result<Ok, Error> semantic::analyze_block_or_expression(semantic::Context& context, ast::Node* node) {
    assert(node->index() == ast::Block || ast::is_expression(node));

    // Do type inference and semantic analysis
    // ---------------------------------------
    auto result = semantic::type_infer_and_analyze(context, node);
    if (result.is_error()) return Error {};

    // If we are in expression add constraint for expression with function return type
    if (node->index() != ast::Block) {
        assert(context.current_function.has_value());
        semantic::add_constraint(context, Set<ast::Type>({ast::get_type(node), context.current_function.value()->return_type}));
    }

    // Merge type constraints that share elements
    context.type_inference.type_constraints = merge_sets_with_shared_elements<ast::Type>(context.type_inference.type_constraints);

    // If we are in a function check if return type is alone, if is alone it means the function is void
    if (context.current_function.has_value()
    && context.current_function.value()->return_type.is_type_variable()) {
        for (size_t i = 0; i < context.type_inference.type_constraints.size(); i++) {
            if (context.type_inference.type_constraints[i].contains(context.current_function.value()->return_type)) {
                if (context.type_inference.type_constraints[i].size() == 1) {
                    context.type_inference.type_constraints[i].insert(ast::Type("void"));
                }
            }
        }
    }

    // Label type constraints
    context.type_inference.current_type_variable_number = 1;
    for (size_t i = 0; i < context.type_inference.type_constraints.size(); i++) {
        std::vector<ast::Type> representatives;
        
        for (auto& type: context.type_inference.type_constraints[i].elements) {
            if (!type.is_type_variable()) {
                representatives.push_back(type);
            }
        }

        if (representatives.size() == 0) {
            context.type_inference.labeled_type_constraints[semantic::new_final_type_variable(context)] = context.type_inference.type_constraints[i];
        }
        else {
            ast::Type representative = representatives[0];
            for (size_t i = 1; i < representatives.size(); i++) {
                if (representative.is_nominal_type() && representatives[i].is_nominal_type()) {
                    if (representative.as_nominal_type().name != representatives[i].as_nominal_type().name) {
                        std::cout << representative.to_str() << " <> " << representatives[i].to_str() << "\n";
                        assert(false);
                    }
                }
                else {
                    std::cout << representative.to_str() << " <> " << representatives[i].to_str() << "\n";
                    assert(false);
                }
            }

            if (context.type_inference.labeled_type_constraints.find(representative) == context.type_inference.labeled_type_constraints.end()) {
                context.type_inference.labeled_type_constraints[representative] = context.type_inference.type_constraints[i];
            }
            else {
                context.type_inference.labeled_type_constraints[representative].merge(context.type_inference.type_constraints[i]);
            }
        }
    }

    // Unify of domains of type variables
    auto labeled_type_constraints = context.type_inference.labeled_type_constraints;
    for (auto type_constraint: labeled_type_constraints) {
        if (type_constraint.first.is_type_variable()) {
            auto interface = type_constraint.second.elements[0].as_type_variable().interface;
            for (size_t i = 0; i < type_constraint.second.elements.size(); i++) {
                auto current = type_constraint.second.elements[i].as_type_variable().interface;
                if (interface == current) {
                    continue;
                }
                else if (!interface.has_value()) {
                    interface = current;
                }
                else if (!current.has_value()) {
                    continue;
                }
                else if (interface == ast::Interface("Number") && current == ast::Interface("Float")) {
                    interface = current;
                }
                else if (interface == ast::Interface("Float") && current == ast::Interface("Number")) {
                    continue;
                }
                else {
                    std::cout << type_constraint.second.elements[i].to_str() << "\n";
                    std::cout << interface.value().name << " : " << current.value().name << "\n";
                    assert(false);
                }
            }

            // Update domain of type variables
            for (size_t i = 0; i < type_constraint.second.elements.size(); i++) {
                type_constraint.second.elements[i].as_type_variable().interface = interface;
            }

            // Update key
            auto key = type_constraint.first;
            key.as_type_variable().interface = interface;
            context.type_inference.labeled_type_constraints.erase(type_constraint.first);
            context.type_inference.labeled_type_constraints[key] = type_constraint.second;
        }
    }

    // Unify
    // -----
    result = semantic::unify_types_and_type_check(context, node);
    if(result.is_error()) return Error {};

    // Make concrete
    // -------------
    if (!context.current_function.has_value()
    ||   context.current_function.value()->state == ast::FunctionCompletelyTyped) {
        result = semantic::get_concrete_as_type_bindings(context, node, {});
        if (result.is_error()) return result;
        make_concrete(context, node);
    }

    return Ok {};
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::BlockNode& node) {
    return semantic::analyze_block_or_expression(context, (ast::Node*) &node);
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::FunctionNode& node) {
    if (node.is_extern) {
        for (auto arg: node.args) {
            auto result = semantic::analyze(context, arg->type);
            if (result.is_error()) return Error {};
        }
        return semantic::analyze(context, node.return_type);
    }

    if (node.state == ast::FunctionBeingAnalyzed) {
        return Ok {};
    }
    if (node.state == ast::FunctionNotAnalyzed) {
        node.state = ast::FunctionBeingAnalyzed;
    }

    // Create new context
    Context new_context;
    semantic::init_Context(new_context, context.ast);
    new_context.current_function = &node;

    if (context.current_module == node.module_path) {
        new_context.scopes = semantic::get_definitions(context);
    }
    else {
        new_context.current_module = node.module_path;
        auto result = semantic::add_definitions_to_current_scope(new_context, *(context.ast->modules[node.module_path.string()]));
        assert(result.is_ok());
    }

    semantic::add_scope(new_context);

    // Analyze function
    if (node.state == ast::FunctionBeingAnalyzed) {
        // For every argument
        for (auto arg: node.args) {
            auto identifier = arg->value;
            if (arg->type == ast::Type(ast::NoType{})) {
                // Create new type variable if it doesn't have a type
                arg->type = semantic::new_type_variable(new_context);
                semantic::add_constraint(new_context, Set<ast::Type>({arg->type}));
            }
            else {
                // Analyze type if it has
                auto result = semantic::analyze(new_context, arg->type);
                if (result.is_error()) return Error {};
            }

            // Create binding
            semantic::current_scope(new_context)[identifier] = semantic::make_Binding((ast::Node*) arg);
        }

        // For return type
        if (node.return_type == ast::Type(ast::NoType{})) {
            // Create new type variable if it doesn't have a type
            node.return_type = semantic::new_type_variable(new_context);
            semantic::add_constraint(new_context, Set<ast::Type>({node.return_type}));
        }
        else {
            // Analyze type if it has
            auto result = semantic::analyze(new_context, node.return_type);
            if (result.is_error()) return Error {};
        }

        // Analyze function body
        auto result = semantic::analyze_block_or_expression(new_context, node.body);
        if (result.is_error()) {
            context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
            return Error {};
        }

        // Unify args and return type
        for (size_t i = 0; i < node.args.size(); i++) {
            node.args[i]->type = semantic::get_unified_type(new_context, node.args[i]->type);

            if (new_context.type_inference.overload_constraints.find(node.args[i]->type) != new_context.type_inference.overload_constraints.end()) {
                node.args[i]->type.as_type_variable().overload_constraints = new_context.type_inference.overload_constraints[node.args[i]->type];
            }
        }
        node.return_type = semantic::get_unified_type(new_context, node.return_type);
        if (new_context.type_inference.overload_constraints.find(node.return_type) != new_context.type_inference.overload_constraints.end()) {
            node.return_type.as_type_variable().overload_constraints = new_context.type_inference.overload_constraints[node.return_type];
        }

        // Set function as analyzed
        node.state = ast::FunctionAnalyzed;
    }
    else if (node.state == ast::FunctionCompletelyTyped) {
        for (auto arg: node.args) {
            auto identifier = arg->value;
            auto result = semantic::analyze(new_context, arg->type);
            if (result.is_error()) {
                context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
                return Error {};
            }
            semantic::current_scope(new_context)[identifier] = semantic::make_Binding((ast::Node*) arg);
        }

        auto result = semantic::analyze(new_context, node.return_type);
        if (result.is_error()) {
            context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
            return Error {};
        }
        
        result = semantic::analyze_block_or_expression(new_context, node.body);
        if (result.is_error()) {
            context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
            return Error {};
        }
    }
    else {
        assert(false);
    }
    return Ok {};
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::Type& type) {
    if      (type.is_type_variable()) return Ok {};
    else if (type == ast::Type("int64")) return Ok {};
    else if (type == ast::Type("int32")) return Ok {};
    else if (type == ast::Type("int8")) return Ok {};
    else if (type == ast::Type("float64")) return Ok {};
    else if (type == ast::Type("bool")) return Ok {};
    else if (type == ast::Type("void")) return Ok {};
    else if (type == ast::Type("string")) return Ok {};
    else if (type.is_pointer()) return semantic::analyze(context, type.as_nominal_type().parameters[0]);
    else if (type.is_array()) return semantic::analyze(context, type.as_nominal_type().parameters[0]);
    else {
        Binding* type_binding = semantic::get_binding(context, type.to_str());
        if (!type_binding) {
            context.errors.push_back(Error{"Errors: Undefined type"});
            return Error {};
        }
        if (type_binding->type != semantic::TypeBinding) {
            context.errors.push_back(Error{"Error: Binding is not a type\n"});
            return Error {};
        }

        type.as_nominal_type().type_definition = semantic::get_type_definition(*type_binding);
        return Ok {};
    }
    assert(false);
    return Error {};
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::TypeNode& node) {
    for (auto field: node.fields) {
        auto result = semantic::analyze(context, field->type);
        if (result.is_error()) return Error {};
    }

    return Ok {};
}

// Type inference and semantic analysis
// ------------------------------------
Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::Node* node) {
    return std::visit(
        [&context, node](auto& variant) {
            return semantic::type_infer_and_analyze(context, variant);
        },
        *node
    );
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::BlockNode& node) {
    semantic::add_scope(context);

    // Add functions to the current scope
    auto result = semantic::add_definitions_to_current_scope(context, node);
    if (result.is_error()) return result;

    // Analyze functions and types in current scope
    auto scope = semantic::current_scope(context);
    for (auto& it: scope) {
        auto& binding = it.second;
        if (binding.type == semantic::TypeBinding) {
            auto result = semantic::analyze(context, *semantic::get_type_definition(binding));
            if (result.is_error()) return result;
        }
    }
    for (auto& it: scope) {
        auto& binding = it.second;
        if (binding.type == semantic::FunctionBinding) {
            auto functions = semantic::get_functions(binding);
            for (size_t i = 0; i < functions.size(); i++) {
                if (functions[i]->state == ast::FunctionAnalyzed) continue;

                auto result = semantic::analyze(context, *functions[i]);
                if (result.is_error()) return result;
            }
        }
    }

    size_t number_of_errors = context.errors.size();
    for (size_t i = 0; i < node.statements.size(); i++) {
        auto result = semantic::type_infer_and_analyze(context, node.statements[i]);

        // Type checking
        switch (node.statements[i]->index()) {
            case ast::Assignment: break;
            case ast::FieldAssignment: break;
            case ast::DereferenceAssignment: break;
            case ast::Call: break;
            case ast::Return: {
                if (result.is_ok()) {
                    auto return_type = std::get<ast::ReturnNode>(*node.statements[i]).expression.has_value() ? ast::get_type(std::get<ast::ReturnNode>(*node.statements[i]).expression.value()) : ast::Type("void");
                    node.type = return_type;
                }
                break;
            }
            case ast::IfElse: {
                if (result.is_ok()) {
                    auto if_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).if_branch);
                    if (if_type != ast::Type(ast::NoType{})) {
                        node.type = if_type;
                    }

                    if (std::get<ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
                        auto else_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).else_branch.value());
                        if (else_type != ast::Type(ast::NoType{})) {
                            node.type = else_type;
                        }
                    }
                }
                break;
            }
            case ast::While: break;
            case ast::Break:
            case ast::Continue: {
                node.type = ast::Type("void");
                break;
            }
            default: assert(false);
        }
    }

    semantic::remove_scope(context);

    if (context.errors.size() > number_of_errors) return Error {};
    else                                          return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FunctionNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(Context& context, ast::Type& type) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(Context& context, ast::TypeNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::AssignmentNode& node) {
    // Type infer and analyze expression
    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};

    // Analyze type of expression
    if (!ast::get_type(node.expression).is_type_variable()) {
        auto type = ast::get_type(node.expression);
        auto result = semantic::analyze(context, type);
        ast::set_type(node.expression, type);
    }

    // Get identifier
    std::string identifier = node.identifier->value;

    // nonlocal assignment
    if (node.nonlocal) {
        semantic::Binding* binding = semantic::get_binding(context, identifier);
        if (!binding) {
            context.errors.push_back(errors::undefined_variable(*node.identifier, context.current_module));
            return Error {};
        }
        semantic::add_constraint(context, Set<ast::Type>({semantic::get_binding_type(*binding), ast::get_type(node.expression)}));
        *binding = semantic::make_Binding(&node);
    }

    // normal assignment
    else {
        if (semantic::current_scope(context).find(identifier) != semantic::current_scope(context).end()
        && semantic::current_scope(context)[identifier].type == AssignmentBinding) {
            auto assignment = get_assignment(semantic::current_scope(context)[identifier]);
            if (!assignment->is_mutable) {
                context.errors.push_back(errors::reassigning_immutable_variable(*node.identifier, *assignment, context.current_module));
                return Error {};
            }
        }
        semantic::current_scope(context)[identifier] = semantic::make_Binding(&node);
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FieldAssignmentNode& node) {
    auto identifier = semantic::type_infer_and_analyze(context, *node.identifier);
    if (identifier.is_error()) return Error {};

    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};
    
    semantic::add_constraint(context, Set<ast::Type>({node.identifier->type, ast::get_type(node.expression)}));
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::DereferenceAssignmentNode& node) {
    // Type infer and analyze expression
    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};

    // Analyze type of expression
    if (!ast::get_type(node.expression).is_type_variable()) {
        auto type = ast::get_type(node.expression);
        auto result = semantic::analyze(context, type);
        ast::set_type(node.expression, type);
    }

    // Analyze identifier
    auto identifier = semantic::type_infer_and_analyze(context, *node.identifier);
    if (identifier.is_error()) return Error {};

    // Add constraint
    semantic::add_constraint(context, Set<ast::Type>({node.identifier->type, ast::get_type(node.expression)}));

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        auto result = semantic::type_infer_and_analyze(context, node.expression.value());
        if (result.is_error()) return Error {};

        assert(context.current_function.has_value());
        semantic::add_constraint(context, Set<ast::Type>({context.current_function.value()->return_type, ast::get_type(node.expression.value())}));
    }
    else {
        assert(context.current_function.has_value());
        semantic::add_constraint(context, Set<ast::Type>({context.current_function.value()->return_type, ast::Type("void")}));
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::BreakNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::ContinueNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::IfElseNode& node) {
    auto result = semantic::type_infer_and_analyze(context, node.condition);
    if (result.is_error()) return Error {};

    result = semantic::type_infer_and_analyze(context, node.if_branch);
    if (result.is_error()) return Error {};

    if (node.else_branch.has_value()) {
        result = semantic::type_infer_and_analyze(context, node.else_branch.value());
        if (result.is_error()) return Error {};
    }

    if (ast::is_expression((ast::Node*) &node)) {
        if (node.type == ast::Type(ast::NoType{})) {
            node.type = semantic::new_type_variable(context);
        }

        // Add constraints
        semantic::add_constraint(context, Set<ast::Type>({
            ast::get_type(node.if_branch),
            ast::get_type(node.else_branch.value()),
            node.type
        }));
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::WhileNode& node) {
    auto result = semantic::type_infer_and_analyze(context, node.condition);
    if (result.is_error()) return Error {};

    result = semantic::type_infer_and_analyze(context, node.block);
    if (result.is_error()) return Error {};

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::UseNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::LinkWithNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::CallArgumentNode& node) {
    auto result = type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return result;
    node.type = ast::get_type(node.expression);
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::IdentifierNode& node) {
    semantic::Binding* binding = semantic::get_binding(context, node.value);
    if (!binding) {
        context.errors.push_back(errors::undefined_variable(node, context.current_module)); // tested in test/errors/undefined_variable.dm
        return Error {};
    }
    else {
        node.type = semantic::get_binding_type(*binding);
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::IntegerNode& node) {
    if (node.type == ast::Type(ast::NoType{})) {   
        node.type = semantic::new_type_variable(context);
        node.type.as_type_variable().interface = ast::Interface("Number");
    }
    else if (!node.type.is_type_variable() && !node.type.is_integer() && !node.type.is_float()) {
        context.errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FloatNode& node) {
    if (node.type == ast::Type(ast::NoType{})) {  
        node.type = semantic::new_type_variable(context);
        node.type.as_type_variable().interface = ast::Interface("Float");
    }
    else if (!node.type.is_type_variable() && !node.type.is_float()) {
        context.errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::BooleanNode& node) {
    node.type = ast::Type("bool");
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::StringNode& node) {
    node.type = ast::Type("string");
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::ArrayNode& node) {
    // Type infer and analyze elements
    for (size_t i = 0; i < node.elements.size(); i++) {
        auto result = semantic::type_infer_and_analyze(context, node.elements[i]);
        if (result.is_error()) return Error {};
    }

    assert(false);

    // Add constraint
    if (node.elements.size() > 0) {
        Set<ast::Type> constraint;
        for (auto element: node.elements) {
            constraint.insert(ast::get_type(element));
        }
        semantic::add_constraint(context, constraint);

    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::CallNode& node) {
    auto& identifier = node.identifier->value;

    // Analyze arguments
    for (auto arg: node.args) {
        auto result = semantic::type_infer_and_analyze(context, *arg);
        if (result.is_error()) return Error {};
    }
      
    // Check binding exists
    semantic::Binding* binding = semantic::get_binding(context, identifier);
    if (!binding) {
        context.errors.push_back(errors::undefined_function(node, ast::get_default_types(ast::get_types(node.args)), context.current_module));
        return Error {};
    }

    if (binding->type == semantic::TypeBinding) {
        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);
        ast::Type type = ast::Type(type_definition->identifier->value, type_definition);
        
        // Set type
        node.type = type;

        // Add types constraints on fields
        for (size_t i = 0; i < type_definition->fields.size(); i++) {
            std::string field = type_definition->fields[i]->value;
            bool founded = false;

            for (auto& arg: node.args) {
                assert(arg->identifier.has_value());

                if (arg->identifier.value()->value == field) {
                    founded = true;
                    
                    semantic::add_constraint(context, Set<ast::Type>({type_definition->fields[i]->type, ast::get_type(arg->expression)}));
                }
            }

            if (!founded) {
                context.errors.push_back(Error{"Error: Not all fields are initialized"});
                return Error {};
            }
        }

        return Ok {};
    }
    else if (is_function(*binding)) {
        if (!node.type.is_concrete()) {
            node.type = semantic::new_type_variable(context);
        }

        if (binding->type == semantic::FunctionBinding) {
           // do nothing
        }
        else {
            assert(false);
        }

        return Ok {};
    }

    assert(false);
    return Error {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 2);
    auto result = semantic::type_infer_and_analyze(context, *node.fields_accessed[0]);
    if (result.is_error()) return Error {};

    if (!node.fields_accessed[0]->type.is_type_variable()) {
        // Get binding
        semantic::Binding* binding = semantic::get_binding(context, node.fields_accessed[0]->type.as_nominal_type().name);
        assert(binding);
        assert(binding->type == semantic::TypeBinding);

        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);

        for (size_t i = 1; i < node.fields_accessed.size(); i++) {
            bool founded = false;
            for (auto field: type_definition->fields) {
                if (field->value == node.fields_accessed[i]->value) {
                    founded = true;
                    node.fields_accessed[i]->type = field->type;
                }
            }

            if (!founded) {
                std::cout << "Error: Field not founded\n";
                assert(false);
            }

            // Iterate on struct_type
            if (i != node.fields_accessed.size() - 1) {
                binding = semantic::get_binding(context, node.fields_accessed[i]->type.as_nominal_type().name);
                assert(binding);
                assert(binding->type == semantic::TypeBinding);

                type_definition = semantic::get_type_definition(*binding);
            }
        }
    }
    else {
        if (context.type_inference.field_constraints.find(node.fields_accessed[0]->type) == context.type_inference.field_constraints.end()) {
            context.type_inference.field_constraints[node.fields_accessed[0]->type] = {};
        }
        std::unordered_map<std::string, ast::Type>* field_constraints = &context.type_inference.field_constraints[node.fields_accessed[0]->type];

        for (size_t i = 1; i < node.fields_accessed.size(); i++) {
            std::string field = node.fields_accessed[i]->value;

            if (field_constraints->find(field) == field_constraints->end()) {
                (*field_constraints)[field] = semantic::new_type_variable(context);
            }

            node.fields_accessed[i]->type = (*field_constraints)[field];

            // Iterate on field constraints
            if (i != node.fields_accessed.size() - 1) {
                if (context.type_inference.field_constraints.find(node.fields_accessed[i]->type) == context.type_inference.field_constraints.end()) {
                    context.type_inference.field_constraints[node.fields_accessed[i]->type] = {};
                }

                field_constraints = &context.type_inference.field_constraints[node.fields_accessed[i]->type];
            }
        }
    }

    // Set overall node type
    node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::AddressOfNode& node) {
    // Type infer and analyze expression
    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};

    // Analyze type of expression
    if (!ast::get_type(node.expression).is_type_variable()) {
        auto type = ast::get_type(node.expression);
        auto result = semantic::analyze(context, type);
        ast::set_type(node.expression, type);
    }

    if (node.type.is_no_type()) {
        node.type = ast::Type(ast::NominalType("pointer"));
        node.type.as_nominal_type().parameters.push_back(ast::get_type(node.expression));
    }
    else if (!node.type.is_type_variable() && !node.type.is_pointer()) {
        context.errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::DereferenceNode& node) {
    // Type infer and analyze expression
    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};

    // Analyze type of expression
    if (!ast::get_type(node.expression).is_type_variable()) {
        auto type = ast::get_type(node.expression);
        auto result = semantic::analyze(context, type);
        ast::set_type(node.expression, type);
    }

    if (node.type.is_no_type()) {
        if (ast::get_type(node.expression).is_pointer()) {
            node.type = ast::get_type(node.expression).as_nominal_type().parameters[0];
        }
        else {
            node.type = semantic::new_type_variable(context);
            
            auto pointer_type = ast::Type(ast::NominalType("pointer"));
            pointer_type.as_nominal_type().parameters.push_back(node.type);
            semantic::add_constraint(context, Set<ast::Type>({ast::get_type(node.expression), pointer_type}));
        }
    }

    return Ok {};
}

// Unify types and type check
// --------------------------
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

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FunctionNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::TypeNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::AssignmentNode& node) {
    auto result = semantic::unify_types_and_type_check(context, node.expression);
    if (result.is_error()) return Error {};

    semantic::current_scope(context)[node.identifier->value] = semantic::make_Binding(&node);
    
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
    node.type = semantic::get_unified_type(context, node.type);
    return semantic::unify_types_and_type_check(context, node.expression);
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

    // Get overload constraints
    if (is_function(*binding)) {
        if (binding->type == semantic::FunctionBinding) {
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
                    functions_that_can_be_called = semantic::remove_incompatible_functions_with_argument_type(functions_that_can_be_called, i, node.args[i]->type);

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

            // Compare new overload constraints with previous
            // ----------------------------------------------
            auto& overload_constraints = context.type_inference.overload_constraints;

            // For each argument
            for (size_t i = 0; i < node.args.size(); i++) {
                auto arg = node.args[i];

                if (arg->type.is_concrete()) {
                    continue;
                }

                // Find new overload constraints
                Set<ast::Type> new_overload_constraints;
                for (auto function: functions_that_can_be_called) {
                    if (!function->args[i]->type.is_type_variable()) {
                        new_overload_constraints.insert(function->args[i]->type);
                    }
                    else if (function->args[i]->type.is_type_variable()
                        &&   function->args[i]->type.as_type_variable().overload_constraints.size() > 0) {
                        for (auto constraint: function->args[i]->type.as_type_variable().overload_constraints.elements) {
                            new_overload_constraints.insert(constraint);
                        }
                    }
                }

                // Add or intersect with previous overload constraints founded for argument type
                if (overload_constraints.find(arg->type) == overload_constraints.end()) {
                    overload_constraints[arg->type] = new_overload_constraints;
                }
                else {
                    auto intersection = overload_constraints[arg->type].intersect(new_overload_constraints);

                    if (intersection.size() == 0) {
                        context.errors.push_back(errors::unexpected_argument_types(node, context.current_module, i, overload_constraints[arg->type].elements, new_overload_constraints.elements, {}));
                        return Error {};
                    }

                    overload_constraints[arg->type] = intersection;
                }
            }
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 2);

    if (!node.type.is_type_variable()) {
        return Ok {};
    }
    else {
        std::unordered_map<std::string, ast::Type>* field_constraints = &context.type_inference.field_constraints[node.fields_accessed[0]->type];

        auto result = semantic::unify_types_and_type_check(context, *node.fields_accessed[0]);
        if (result.is_error()) return result;

        for (size_t i = 1; i < node.fields_accessed.size(); i++) {
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

    node.type = semantic::get_unified_type(context, node.type);
    return Ok {};
}

// Get concrete types as type bindings
// -----------------------------------
Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::Node* node, std::vector<ast::CallInCallStack> call_stack) {
    return std::visit([&context, node, &call_stack](auto& variant) {
        return semantic::get_concrete_as_type_bindings(context, variant, call_stack);
    }, *node);
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::BlockNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Add scope
     semantic::add_scope(context);

    // Add functions to the current scope
    auto result = semantic::add_definitions_to_current_scope(context, node);
    if (result.is_error()) return result;

    // Make statements concrete
    size_t number_of_errors = context.errors.size();
    for (auto statement: node.statements) {
        auto result = semantic::get_concrete_as_type_bindings(context, statement, call_stack);
        if (result.is_error()) return result;
    }

    // Remove scope
    semantic::remove_scope(context);

    if (context.errors.size() > number_of_errors) return Error {};
    else                                          return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::FunctionNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::TypeNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::AssignmentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return semantic::get_concrete_as_type_bindings(context, node.expression, call_stack);
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::FieldAssignmentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return semantic::get_concrete_as_type_bindings(context, node.expression, call_stack);
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::DereferenceAssignmentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::get_concrete_as_type_bindings(context, *node.identifier, call_stack);
    if (result.is_error()) return result;
    
    result = semantic::get_concrete_as_type_bindings(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::ReturnNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (node.expression.has_value()) {
        return semantic::get_concrete_as_type_bindings(context, node.expression.value(), call_stack);
    }

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::BreakNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::ContinueNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::IfElseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::get_concrete_as_type_bindings(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::get_concrete_as_type_bindings(context, node.if_branch, call_stack);
    if (result.is_error()) return result;
    
    if (node.else_branch.has_value()) {
        result = semantic::get_concrete_as_type_bindings(context, node.else_branch.value(), call_stack);
        if (result.is_error()) return result;
    }

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::WhileNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::get_concrete_as_type_bindings(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::get_concrete_as_type_bindings(context, node.block, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::UseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::LinkWithNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::CallArgumentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::get_concrete_as_type_bindings(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    if (binding->type == semantic::TypeBinding) {
        for (auto arg: node.args) {
            auto result = semantic::get_concrete_as_type_bindings(context, *arg, call_stack);
            if (result.is_error()) return result;
        }
        node.type = ast::Type(node.identifier->value, semantic::get_type_definition(*binding));
    }
    else if (is_function(*binding)) {
        if (binding->type == semantic::FunctionBinding) {
            assert(node.functions.size() != 0);
            auto functions_that_can_be_called = node.functions;

            ast::FunctionNode* called_function = nullptr;
            if (semantic::is_type_concrete(context, node.type)) {
                // Remove functions that aren't expected
                functions_that_can_be_called = remove_incompatible_functions_with_return_type(functions_that_can_be_called, ast::get_concrete_type(node.type, context.type_inference.type_bindings));

                // Check what function to call is not ambiguos
                if (functions_that_can_be_called.size() > 1) {
                    context.errors.push_back(errors::ambiguous_what_function_to_call(node, context.current_module, functions_that_can_be_called, call_stack));
                    return Error {};
                }

                // There should be exactly one function
                assert(functions_that_can_be_called.size() == 1);
                auto called_function = functions_that_can_be_called[0];

                // Set and check expected types of arguments
                auto result = semantic::set_expected_types_of_arguments_and_check(context, &node, called_function, 0, call_stack);
                if (result.is_error()) return Error{};
            }
            else {
                size_t i = 0;
                while (i < node.args.size()) {     
                    // Get concrete type for current argument
                    auto result = get_concrete_as_type_bindings(context, *node.args[i], call_stack);
                    if (result.is_error()) return result;

                    // Remove functions that don't match with the type founded for the argument
                    auto backup = functions_that_can_be_called;
                    functions_that_can_be_called = semantic::remove_incompatible_functions_with_argument_type(functions_that_can_be_called, i, ast::get_concrete_type(node.args[i]->type, context.type_inference.type_bindings));

                    // If only one function that can be called remains
                    if (functions_that_can_be_called.size() == 1) {
                        auto called_function = functions_that_can_be_called[0];

                        // Check types of remaining arguments
                        auto result = semantic::set_expected_types_of_arguments_and_check(context, &node, called_function, i + 1, call_stack);
                        if (result.is_error()) return Error{};
                        break;
                    }
                    else if (functions_that_can_be_called.size() == 0) {
                        std::vector<ast::Type> possible_types = semantic::get_possible_types_for_argument(backup, i);
                        context.errors.push_back(errors::unexpected_argument_type(node, context.current_module, i, ast::get_concrete_type(node.args[i]->type, context.type_inference.type_bindings), possible_types, call_stack));
                        return Error {};
                    }

                    i++;
                }

                // Check what function to call is not ambiguos
                if (functions_that_can_be_called.size() > 1) {
                    context.errors.push_back(errors::ambiguous_what_function_to_call(node, context.current_module, functions_that_can_be_called, call_stack));
                    return Error {};
                }
                auto& called_function = functions_that_can_be_called[0];

                // Get type of called function
                if (node.type.is_type_variable()
                &&  context.type_inference.type_bindings.find(node.type.as_type_variable().id) == context.type_inference.type_bindings.end()) {
                    if (called_function->state == ast::FunctionCompletelyTyped) {
                        context.type_inference.type_bindings[node.type.as_type_variable().id] = called_function->return_type;
                    }
                    else {
                        auto result = semantic::get_function_type(context, &node, called_function, ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings), call_stack);
                        if (result.is_error()) return Error {};
                        context.type_inference.type_bindings[node.type.as_type_variable().id] = result.get_value();
                    }
                }
            }
        }
        else {
            assert(false);
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::FloatNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::IntegerNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::IdentifierNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::BooleanNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::StringNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::ArrayNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::FieldAccessNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(node.fields_accessed.size() >= 2);

    if (!node.type.is_type_variable()) {
        return Ok {};
    }
    else {
        auto result = semantic::get_concrete_as_type_bindings(context, *node.fields_accessed[0], call_stack);
        if (result.is_error()) return Error {};

        assert(semantic::get_type(context, node.fields_accessed[0]->type).is_concrete());

        ast::Type struct_type = semantic::get_type(context, node.fields_accessed[0]->type);

        for (size_t i = 1; i < node.fields_accessed.size(); i++) {
            auto field = node.fields_accessed[i]->value;
            assert(struct_type.is_nominal_type());

            bool field_founded = false;
            for (auto field_in_definition: struct_type.as_nominal_type().type_definition->fields) {
                if (field == field_in_definition->value) {
                    field_founded = true;
                    context.type_inference.type_bindings[node.fields_accessed[i]->type.as_type_variable().id] = field_in_definition->type;
                    break;
                }
            }
            assert(field_founded);

            // Iterate over struct type
            if (i + 1 != node.fields_accessed.size()) {
                struct_type = semantic::get_type(context, node.fields_accessed[i]->type);
            }
        }

        // Set overall node type
        node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

        return Ok {};
    }
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::AddressOfNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::get_concrete_as_type_bindings(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::DereferenceNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(semantic::get_type(context, ast::get_type(node.expression)).is_pointer());

    auto result = get_concrete_as_type_bindings(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

// Get function type
// -----------------
Result<ast::Type, Error> semantic::get_function_type(Context& context, ast::CallNode* call, ast::FunctionNode* function, std::vector<ast::Type> args, std::vector<ast::CallInCallStack> call_stack) {
    // Check if specialization already exists
    for (auto& specialization: function->specializations) {
        if (specialization.args == args) {
            return specialization.return_type;
        }
    }

    // Add call to call stack
    for (auto& call_in_stack: call_stack) {
        if (call_in_stack.identifier == call->identifier->value
        && call_in_stack.args == args) {
            if (function->return_type.is_type_variable()
            &&  function->return_type.as_type_variable().interface.has_value()) {
                context.type_inference.type_bindings[function->return_type.as_type_variable().id] = function->return_type.as_type_variable().interface.value().get_default_type();
                return context.type_inference.type_bindings[function->return_type.as_type_variable().id];
            }
            else {
                std::cout << "RECURSION :O\n";
                assert(false);
            }
        }
    }
    call_stack.push_back(ast::CallInCallStack(function->identifier->value, args, call, function, context.current_module));

    // Else type check and create specialization type
    ast::FunctionSpecialization specialization;

    // Create new context
    Context new_context;
    semantic::init_Context(new_context, context.ast);
    new_context.current_function = function;

    if (context.current_module == function->module_path) {
        new_context.scopes = semantic::get_definitions(context);
    }
    else {
        new_context.current_module = function->module_path;
        auto result = semantic::add_definitions_to_current_scope(new_context, *(context.ast->modules[function->module_path.string()]));
        assert(result.is_ok());
    }

    semantic::add_scope(new_context);

    // Add arguments as type bindings
    for (size_t i = 0; i < args.size(); i++) {
        ast::Type call_type = args[i];
        ast::Type function_type = function->args[i]->type;

        if (function_type.is_type_variable() || !function_type.is_concrete()) {
            // If it was no already included
            if (new_context.type_inference.type_bindings.find(function_type.as_type_variable().id) == new_context.type_inference.type_bindings.end()) {
                new_context.type_inference.type_bindings[function_type.as_type_variable().id] = call_type;
            }
            // Else compare with previous type founded for it
            else if (new_context.type_inference.type_bindings[function_type.as_type_variable().id] != call_type) {
                assert(false);
            }
        }
        else if (call_type != function_type) {
            assert(false);
        }
    }

    // Analyze function with call argument types
    auto result = semantic::get_concrete_as_type_bindings(new_context, function->body, call_stack);
    if (result.is_error()) {
        context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
        return Error {};
    }

    // Add specialization
    specialization.type_bindings = new_context.type_inference.type_bindings;
    for (size_t i = 0; i < args.size(); i++) {
        specialization.args.push_back(args[i]);
    }

    if (function->return_type.is_type_variable() || !function->return_type.is_concrete()) {
        if (specialization.type_bindings.find(function->return_type.as_type_variable().id) != specialization.type_bindings.end()) {
            specialization.return_type = specialization.type_bindings[function->return_type.as_type_variable().id];
        }
        else if (function->return_type.is_type_variable()
        &&       function->return_type.as_type_variable().interface.has_value()) {
            specialization.type_bindings[function->return_type.as_type_variable().id] = function->return_type.as_type_variable().interface.value().get_default_type();
            specialization.return_type = specialization.type_bindings[function->return_type.as_type_variable().id];
        }
        else {
            assert(false);
        }
    }
    else {
        specialization.return_type = function->return_type;
    }

    function->specializations.push_back(specialization);

    return specialization.return_type;
}

// Make concrete
// -------------
void semantic::make_concrete(Context& context, ast::Node* node) {
    switch (node->index()) {
        case ast::Block: {
            auto& block = std::get<ast::BlockNode>(*node);

            // Add scope
            semantic::add_scope(context);

            // Add functions to the current scope
            auto result = semantic::add_definitions_to_current_scope(context, block);
            assert(result.is_ok());

            // Make statements concrete
            size_t number_of_errors = context.errors.size();
            for (auto statement: block.statements) {
                semantic::make_concrete(context, statement);
            }

            // Remove scope
            semantic::remove_scope(context);
            break;
        }

        case ast::Function: {
            break;
        }

        case ast::TypeDef: {
            break;
        }

        case ast::Assignment: {
            auto& assignment = std::get<ast::AssignmentNode>(*node);
            semantic::make_concrete(context, assignment.expression);
            break;
        }

        case ast::FieldAssignment: {
            auto& assignment = std::get<ast::FieldAssignmentNode>(*node);
            semantic::make_concrete(context, (ast::Node*) assignment.identifier);
            semantic::make_concrete(context, (ast::Node*) assignment.expression);
            break;
        }

        case ast::DereferenceAssignment: {
            auto& assignment = std::get<ast::DereferenceAssignmentNode>(*node);
            semantic::make_concrete(context, (ast::Node*) assignment.identifier);
            semantic::make_concrete(context, (ast::Node*) assignment.expression);
            break;
        }

        case ast::Return: {
            auto& return_node = std::get<ast::ReturnNode>(*node);
            if (return_node.expression.has_value()) {
                semantic::make_concrete(context, return_node.expression.value());
            }
            break;
        }

        case ast::Break: {
            break;
        }

        case ast::Continue: {
            break;
        }

        case ast::IfElse: {
            auto& if_else = std::get<ast::IfElseNode>(*node);
            semantic::make_concrete(context, if_else.condition);
            semantic::make_concrete(context, if_else.if_branch);
            if (if_else.else_branch.has_value()) {
                semantic::make_concrete(context, if_else.else_branch.value());
            }

            if (ast::is_expression(node)) {
                if_else.type = ast::get_concrete_type(if_else.type, context.type_inference.type_bindings);
            }
            break;
        }

        case ast::While: {
            auto& while_node = std::get<ast::WhileNode>(*node);
            semantic::make_concrete(context, while_node.condition);
            semantic::make_concrete(context, while_node.block);
            break;
        }

        case ast::Use: {
            assert(false);
            break;
        }

        case ast::LinkWith: {
            assert(false);
            break;
        }

        case ast::CallArgument: {
            auto& call_argument = std::get<ast::CallArgumentNode>(*node);
            semantic::make_concrete(context, call_argument.expression);
            call_argument.type = ast::get_type(call_argument.expression);
            break;
        }

        case ast::Call: {
            auto& call = std::get<ast::CallNode>(*node);
            for (auto arg: call.args) {
                semantic::make_concrete(context, (ast::Node*) arg);
            }
            
            call.type = ast::get_concrete_type(call.type, context.type_inference.type_bindings);
            break;
        }

        case ast::Float: {
            auto& float_node = std::get<ast::FloatNode>(*node);
            float_node.type = ast::get_concrete_type(float_node.type, context.type_inference.type_bindings);
            break;
        }

        case ast::Integer: {
            auto& integer = std::get<ast::IntegerNode>(*node);
            integer.type = ast::get_concrete_type(integer.type, context.type_inference.type_bindings);
            break;
        }

        case ast::Identifier: {
            auto& identifier = std::get<ast::IdentifierNode>(*node);
            identifier.type = ast::get_concrete_type(identifier.type, context.type_inference.type_bindings);
            break;
        }

        case ast::Boolean: {
            break;
        }

        case ast::String: {
            break;
        }

        case ast::Array: {
            auto& array = std::get<ast::ArrayNode>(*node);
            assert(false);
            break;
        }

        case ast::FieldAccess: {
            auto& field_access = std::get<ast::FieldAccessNode>(*node);
            for (auto field: field_access.fields_accessed) {
                field->type = ast::get_concrete_type(field->type, context.type_inference.type_bindings);
            }

            field_access.type = ast::get_concrete_type(field_access.type, context.type_inference.type_bindings);
            break;
        }

        case ast::AddressOf: {
            auto& address_of = std::get<ast::AddressOfNode>(*node);
            semantic::make_concrete(context, address_of.expression);
    
            assert(address_of.type.is_pointer());
            if (!address_of.type.is_concrete()) {
                address_of.type.as_nominal_type().parameters[0] = ast::get_concrete_type(address_of.expression, context.type_inference.type_bindings);
            }            
            break;
        }

        case ast::Dereference: {
            auto& dereference = std::get<ast::DereferenceNode>(*node);
            assert(ast::get_concrete_type(dereference.expression, context.type_inference.type_bindings).is_pointer());
            dereference.type = ast::get_concrete_type(dereference.type, context.type_inference.type_bindings);
            make_concrete(context, dereference.expression);
            break;
        }

        default: {
            assert(false);
        }
    }
}