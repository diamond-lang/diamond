#include <unordered_map>
#include <cassert>
#include <filesystem>
#include <set>
#include <iostream>
#include <cassert>
#include <bits/stdc++.h>

#include "ast.hpp"
#include "errors.hpp"
#include "semantic.hpp"
#include "utilities.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "intrinsics.hpp"

namespace semantic {
    // Set
    template <typename T>
    struct Set {
        std::vector<T> elements;
    };

    template <typename T>
    Set<T> make_Set(std::vector<T> elements);

    template <typename T>
    void insert(Set<T>& set, T element);
    template <typename T>
    bool contains(Set<T> set, T element);
    template <typename T>
    size_t size(Set<T> set);
    template <typename T>
    void merge(Set<T>& a, Set<T> b);
    template <typename T>
    Set<T> intersect(Set<T> a, Set<T> b);
    template <typename T>
    std::vector<Set<T>> merge_sets_with_shared_elements(std::vector<Set<T>> sets);

    // Binding
    enum BindingType {
        AssignmentBinding,
        FunctionArgumentBinding,
        OverloadedFunctionsBinding,
        GenericFunctionBinding,
        TypeBinding
    };

    struct Binding {
        BindingType type;
        std::vector<ast::Node*> value;
    };

    Binding make_Binding(ast::AssignmentNode* assignment);
    Binding make_Binding(ast::Node* function_argument);
    Binding make_Binding(ast::FunctionNode* function);
    Binding make_Binding(std::vector<ast::FunctionNode*> functions);
    Binding make_Binding(ast::TypeNode* type);

    ast::AssignmentNode* get_assignment(Binding binding);
    ast::Node* get_function_argument(Binding binding);
    ast::FunctionNode* get_generic_function(Binding binding);
    std::vector<ast::FunctionNode*> get_overloaded_functions(Binding binding);
    ast::TypeNode* get_type_definition(Binding binding);
    ast::Type get_binding_type(Binding& binding);
    std::string get_binding_identifier(Binding& binding);
    bool is_function(Binding& binding);

    // Scopes
    using Scopes = std::vector<std::unordered_map<std::string, Binding>>;

    // Context
    struct StructType {
        std::unordered_map<std::string, ast::Type> fields;
    };

    struct FunctionConstraint {
        Binding binding;
        ast::FunctionConstraint constraint; 
        ast::CallNode* call = nullptr;   
    };

    struct TypeInference {
        size_t current_type_variable_number = 1;
        std::vector<Set<ast::Type>> type_constraints;
        std::unordered_map<ast::Type, Set<ast::Type>> labeled_type_constraints;
        std::unordered_map<ast::Type, StructType> struct_types;
        std::vector<semantic::FunctionConstraint> function_constraints;
    };

    void print(std::unordered_map<ast::Type, StructType> struct_types);
    void print(Set<ast::Type> set);
    void print(std::vector<Set<ast::Type>> sets);
    void print(std::unordered_map<ast::Type, Set<ast::Type>> labeled_sets);

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
    bool in_generic_function(Context& context);
    void add_definitions_to_current_scope(Context& context, ast::BlockNode& block);
    void add_module_functions(Context& context, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules);
    std::vector<std::unordered_map<std::string, Binding>> get_definitions(Context& context);
    Result<ast::Type, Error> get_type_of_generic_function(Context& context, std::vector<ast::Type> args, ast::FunctionNode* function, std::vector<ast::FunctionPrototype> call_stack = {});
    Result<Ok, Error> check_constraint_of_generic_function(Context& context, std::unordered_map<std::string, ast::Type>& type_bindings, ast::FunctionConstraint constraint, std::vector<ast::FunctionPrototype> call_stack = {});
    // bool depends_on_binding_with_concrete_type(ast::Node* node);
    void add_constraint(Context& context, Set<ast::Type> constraint);
    ast::Type new_type_variable(Context& context);
    ast::Type get_unified_type(Context& context, ast::Type type_var);
    ast::Type new_final_type_variable(Context& context);
    bool has_type_variables(std::vector<ast::Type> types);
    Result<Ok, Error> check_function_constraint(Context& context, semantic::FunctionConstraint constraint);
    void relabel(Context& context, ast::Type type_variable, ast::Type new_type);

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
    Result<Ok, Error> type_infer_and_analyze(Context& context, ast::FieldAccessNode& node);

    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::Node* node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::BlockNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FunctionNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::TypeNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::AssignmentNode& node);
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FieldAssignmentNode& node);
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
    Result<Ok, Error> unify_types_and_type_check(Context& context, ast::FieldAccessNode& node);
}

// Set
// ---
template <typename T>
semantic::Set<T> semantic::make_Set(std::vector<T> elements) {
    Set<T> set;
    for (size_t i = 0; i < elements.size(); i++) {
        semantic::insert(set, elements[i]);
    }
    return set;
}

template <typename T>
void semantic::insert(semantic::Set<T>& set, T element) {
    for (size_t i = 0; i < set.elements.size(); i++) {
        if (set.elements[i] == element) {
            return;
        }
    }
    set.elements.push_back(element);
}

template <typename T>
bool semantic::contains(semantic::Set<T> set, T element) {
    for (size_t i = 0; i < set.elements.size(); i++) {
        if (set.elements[i] == element) return true;
    }
    return false;
}

template <typename T>
size_t semantic::size(semantic::Set<T> set) {
    return set.elements.size();
}

template <typename T>
void semantic::merge(semantic::Set<T>& a, semantic::Set<T> b) {
    for (size_t i = 0; i < b.elements.size(); i++) {
        insert(a, b.elements[i]);
    }
}

template <typename T>
semantic::Set<T> semantic::intersect(semantic::Set<T> a, semantic::Set<T> b) {
    semantic::Set<T> intersection;
    for (auto& element: a.elements) {
        if (contains(b, element)) {
            intersection.elements.push_back(element);
        }
    }
    return intersection;
}

template <typename T>
std::vector<semantic::Set<T>> semantic::merge_sets_with_shared_elements(std::vector<Set<T>> sets) {
    size_t i = 0;
    while (i < sets.size()) {
        bool merged = false;

        for (size_t j = i + 1; j < sets.size(); j++) {
            Set<ast::Type> intersection = semantic::intersect<T>(sets[i], sets[j]);
            if (semantic::size<T>(intersection) != 0) {
                // Merge sets
                semantic::merge<T>(sets[i], sets[j]);

                // Erase duplicated set
                sets.erase(sets.begin() + j);
                merged = true;
                break;
            }
        }

        if (!merged) i++;
    }

    return sets;
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

semantic::Binding semantic::make_Binding(ast::FunctionNode* function) {
    semantic::Binding binding;
    binding.type = semantic::GenericFunctionBinding;
    binding.value.push_back((ast::Node*) function);
    return binding;
}

semantic::Binding semantic::make_Binding(std::vector<ast::FunctionNode*> functions) {
    semantic::Binding binding;
    binding.type = semantic::OverloadedFunctionsBinding;
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

ast::FunctionNode* semantic::get_generic_function(semantic::Binding binding) {
    assert(binding.type == semantic::GenericFunctionBinding);
    return (ast::FunctionNode*) binding.value[0];
}

std::vector<ast::FunctionNode*> semantic::get_overloaded_functions(semantic::Binding binding) {
    assert(binding.type == semantic::OverloadedFunctionsBinding);
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
        case semantic::OverloadedFunctionsBinding: return ast::Type("function");
        case semantic::GenericFunctionBinding: return ast::Type("function");
        case semantic::TypeBinding: return ast::Type("type");
    }
    assert(false);
    return ast::Type("");
}

std::string semantic::get_binding_identifier(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return ((ast::AssignmentNode*)binding.value[0])->identifier->value;
        case semantic::FunctionArgumentBinding: return ((ast::IdentifierNode*)binding.value[0])->value;
        case semantic::OverloadedFunctionsBinding: return ((ast::FunctionNode*)binding.value[0])->identifier->value;
        case semantic::GenericFunctionBinding: return ((ast::FunctionNode*)binding.value[0])->identifier->value;
        case semantic::TypeBinding: return ((ast::TypeNode*)binding.value[0])->identifier->value;
    }
    assert(false);
    return "";
}

bool semantic::is_function(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::AssignmentBinding: return false;
        case semantic::FunctionArgumentBinding: return false;
        case semantic::OverloadedFunctionsBinding: return true;
        case semantic::GenericFunctionBinding: return true;
        case semantic::TypeBinding: return false;
    }
    assert(false);
    return false;
}

// Type inference
// --------------
void semantic::print(std::unordered_map<ast::Type, semantic::StructType> struct_types) {
    for (auto it = struct_types.begin(); it != struct_types.end(); it++) {
        std::cout << it->first.to_str() << ": {\n";
        for (auto it2 = it->second.fields.begin(); it2 != it->second.fields.end(); it2++) {
            std::cout << "    " << it2->first << ": " << it2->second.to_str() << "\n";
        }
        std::cout << "}";
        if (std::distance(it,struct_types.end()) > 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
}

void semantic::print(Set<ast::Type> type_constraint) {
    std::cout << "{";
    for (size_t i = 0; i < type_constraint.elements.size(); i++) {
        std::cout << type_constraint.elements[i].to_str();
        if (i != type_constraint.elements.size() - 1) std::cout << ", ";
    }
    std::cout << "}\n";
}

void semantic::print(std::vector<Set<ast::Type>> type_constraints) {
    for (auto& set: type_constraints) {
        semantic::print(set);
    }
}

void semantic::print(std::unordered_map<ast::Type, Set<ast::Type>> labeled_type_constraints) {
    for (auto it: labeled_type_constraints) {
        std::cout << it.first.to_str() << ": ";
        semantic::print(it.second);
    }
}

// Context
// -------
void semantic::init_Context(semantic::Context& context, ast::Ast* ast) {
    // Add intrinsic functions
    semantic::add_scope(context);

    for (auto it = intrinsics.begin(); it != intrinsics.end(); it++) {
        auto& identifier = it->first;
        auto& scope = current_scope(context);
        std::vector<ast::FunctionNode*> overloaded_functions;
        for (auto& prototype: it->second) {
            // Create function node
            auto function_node = ast::FunctionNode {};
            function_node.generic = false;

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

bool semantic::in_generic_function(Context& context) {
    return context.current_function.has_value() && context.current_function.value()->generic;
}

void semantic::add_definitions_to_current_scope(Context& context, ast::BlockNode& block) {
    // Add functions from block to current scope
    for (auto& function: block.functions) {
        auto& identifier = function->identifier->value;
        auto& scope = semantic::current_scope(context);

        if (scope.find(identifier) == scope.end()) {
            if (function->generic) {
                scope[identifier] = semantic::make_Binding(function);

            }
            else {
                scope[identifier] = semantic::make_Binding(std::vector{function});
            }
        }
        else if (is_function(scope[identifier])) {
            if (scope[identifier].type == GenericFunctionBinding) {
                context.errors.push_back(Error{"Error: Trying to overload generic function!"});
            }
            else {
                scope[identifier].value.push_back((ast::Node*) function);
            }
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
            context.errors.push_back(Error{"Error: Type defined multiple times"});
        }
    }

    // Add functions from modules
    auto current_directory = context.current_module.parent_path();
    std::set<std::filesystem::path> already_included_modules = {context.current_module};

    for (auto& use_stmt: block.use_statements) {
        auto module_path = std::filesystem::canonical(current_directory / (use_stmt->path->value + ".dmd"));
        assert(std::filesystem::exists(module_path));
        semantic::add_module_functions(context, module_path, already_included_modules);
    }
}

void semantic::add_module_functions(Context& context, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules) {
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
                if (function->generic) {
                    scope[identifier] = semantic::make_Binding(function);

                }
                else {
                    scope[identifier] = semantic::make_Binding(std::vector{function});
                }
            }
            else if (is_function(scope[identifier])) {
                if (scope[identifier].type == GenericFunctionBinding) {
                    context.errors.push_back(Error{"Error: Trying to overload generic function!"});
                }
                else {
                    scope[identifier].value.push_back((ast::Node*) function);
                }
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
                context.errors.push_back(Error{"Error: Type defined multiple times"});
            }
        }

        already_included_modules.insert(module_path);

        // Add includes
        for (auto& use_stmt: context.ast->modules[module_path.string()]->use_statements) {
            if (use_stmt->include) {
                auto include_path = std::filesystem::canonical(module_path.parent_path() / (use_stmt->path->value + ".dmd"));
                semantic::add_module_functions(context, include_path, already_included_modules);
            }
        }
    }
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

Result<ast::Type, Error> semantic::get_type_of_generic_function(Context& context, std::vector<ast::Type> args, ast::FunctionNode* function, std::vector<ast::FunctionPrototype> call_stack) {
    // Check specializations
    for (auto& specialization: function->specializations) {
        if (specialization.args == args) {
            return specialization.return_type;
        }
    }

    // Else check constraints and add specialization
    ast::FunctionSpecialization specialization;

    // Add arguments
    for (size_t i = 0; i < args.size(); i++) {
        ast::Type type = args[i];
        ast::Type type_variable = function->args[i]->type;

        if (type_variable.is_type_variable()) {
            // If it was no already included
            if (specialization.type_bindings.find(type_variable.to_str()) == specialization.type_bindings.end()) {
                specialization.type_bindings[type_variable.to_str()] = type;
            }
            // Else compare with previous type founded for her
            else if (specialization.type_bindings[type_variable.to_str()] != type) {
                context.errors.push_back(Error{"Error: Incompatible types in function arguments"});
                return Error {};
            }
        }
        else if (type != type_variable) {
            context.errors.push_back(Error{"Error: Incompatible types in function arguments"});
            return Error {};
        }

        specialization.args.push_back(type);
    }

    // Check constraints
    call_stack.push_back(ast::FunctionPrototype{function->identifier->value, args, ast::Type("")});
    for (auto& constraint: function->constraints) {
        semantic::check_constraint_of_generic_function(context, specialization.type_bindings, constraint, call_stack);
    }

    // Add return type to specialization
    if (function->return_type.is_type_variable()) {
        specialization.return_type = specialization.type_bindings[function->return_type.to_str()];
    }
    else {
        specialization.return_type = function->return_type;
    }

    // Add specialization to function
    function->specializations.push_back(specialization);

    return specialization.return_type;
}

Result<Ok, Error> semantic::check_constraint_of_generic_function(Context& context, std::unordered_map<std::string, ast::Type>& type_bindings, ast::FunctionConstraint constraint, std::vector<ast::FunctionPrototype> call_stack) {
     if (constraint.identifier == "Number") {
        ast::Type type_variable = constraint.args[0];

        // If type variable was not already included
        if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
            type_bindings[type_variable.to_str()] = ast::Type("int64");
        }
        // Else compare with previous type founded for her
        else if (type_bindings[type_variable.to_str()] != ast::Type("int64")
        &&       type_bindings[type_variable.to_str()] != ast::Type("int32")
        &&       type_bindings[type_variable.to_str()] != ast::Type("int8")
        &&       type_bindings[type_variable.to_str()] != ast::Type("float64")) {
            assert(false);
            return Error {};
        }

        return Ok {};
    }
    else if (constraint.identifier == "Float") {
        ast::Type type_variable = constraint.args[0];

        // If return type was not already included
        if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
            type_bindings[type_variable.to_str()] = ast::Type("float64");
        }
        // Else compare with previous type founded for her
        else if (type_bindings[type_variable.to_str()] != ast::Type("float64")) {
            assert(false);
            return Error {};
        }

        return Ok {};
    }
    else if (constraint.identifier[0] == '.') {
        std::string type_name = type_bindings[constraint.args[0].to_str()].to_str();
        Binding* type_binding = semantic::get_binding(context, type_name);
        if (!type_binding) {
            context.errors.push_back(Error{"Errors: Undefined type"});
            return Error {};
        }
        if (type_binding->type != semantic::TypeBinding) {
            context.errors.push_back(Error{"Error: Binding is not a type\n"});
            return Error {};
        }
        ast::TypeNode* type_definition = semantic::get_type_definition(*type_binding);
        
        // Search for field
        for (auto field: type_definition->fields) {
            if ("." + field->value == constraint.identifier) {
                type_bindings[constraint.return_type.to_str()] = field->type;
                return Ok {};
            }
        }

        context.errors.push_back(Error{"Error: Struct type doesn't have that field"});
        return Error {};
    }
    else {
        semantic::Binding* binding = semantic::get_binding(context, constraint.identifier);
        if (!binding || !is_function(*binding)) {
            context.errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
            return Error {};
        }
        else if (binding->type == semantic::OverloadedFunctionsBinding) {
            for (auto function: semantic::get_overloaded_functions(*binding)) {
                // Check arguments types match
                if (ast::get_types(function->args) == ast::get_concrete_types(constraint.args, type_bindings)) {
                    ast::Type type_variable = constraint.return_type;

                    // If return type was not already included
                    if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
                        type_bindings[type_variable.to_str()] = function->return_type;
                    }
                    // Else compare with previous type founded for her
                    else if (type_bindings[type_variable.to_str()] != function->return_type) {
                        context.errors.push_back(Error{"Error: Incompatible types in function constraints"});
                        return Error {};
                    }
                }
            }
            return Error {};
        }
        else if (binding->type == semantic::GenericFunctionBinding) {
            // Check if constraints are already being checked
            for (auto i = call_stack.rbegin(); i != call_stack.rend(); i++) {
                if (*i == ast::FunctionPrototype{constraint.identifier, ast::get_concrete_types(constraint.args, type_bindings), ast::Type("")}) {
                    return Ok {};
                }
            }

            // Check constraints otherwise
            auto result = semantic::get_type_of_generic_function(context, ast::get_concrete_types(constraint.args, type_bindings), semantic::get_generic_function(*binding), call_stack);
            if (result.is_error()) return Error {};
        }
        else {
            context.errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
            return Error {};
        }

        return Ok {};
    }
}

void semantic::add_constraint(Context& context, Set<ast::Type> constraint) {
    context.type_inference.type_constraints.push_back(constraint);
}

ast::Type semantic::new_type_variable(Context& context) {
    ast::Type new_type = ast::Type(std::to_string(context.type_inference.current_type_variable_number));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

ast::Type semantic::get_unified_type(Context& context, ast::Type type_var) {
    if (type_var.is_type_variable()) {
        for (auto it = context.type_inference.labeled_type_constraints.begin(); it != context.type_inference.labeled_type_constraints.end(); it++) {
            if (semantic::contains(it->second, type_var)) {
                auto type = it->first;
                if (type.is_type_variable()) {
                    if (type.domain == "number") return ast::Type("int64");
                    else if (type.domain == "float") type = ast::Type("float64");
                }
                return type;
            }
        }

        ast::Type new_type_var = semantic::new_final_type_variable(context);
        context.type_inference.labeled_type_constraints[new_type_var] = semantic::make_Set<ast::Type>({type_var});
        return new_type_var;
    }
    return type_var;
}

ast::Type semantic::new_final_type_variable(Context& context) {
    ast::Type new_type = ast::Type("$" + std::to_string(context.type_inference.current_type_variable_number));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

bool semantic::has_type_variables(std::vector<ast::Type> types) {
    for (auto& type: types) {
        if (type.is_type_variable()) return true;
    }
    return false;
}

Result<Ok, Error> semantic::check_function_constraint(semantic::Context& context, semantic::FunctionConstraint constraint) {
    // Get unified and default types
    std::vector<ast::Type> args_types;
    std::vector<ast::Type> default_types;
    for (auto arg: constraint.constraint.args) {
        args_types.push_back(arg);
        auto unified_type = semantic::get_unified_type(context, arg);
        assert(!unified_type.is_type_variable());
        default_types.push_back(ast::Type(unified_type));
    }

    // If the function is concrete
    if (constraint.binding.type == semantic::OverloadedFunctionsBinding) {
        // Try default arguments types
        for (auto function: semantic::get_overloaded_functions(constraint.binding)) {
            if (ast::get_types(function->args) == default_types) {
                return Ok {};
            }
        }

        // Try everything else
        std::vector<ast::FunctionNode*> functions_that_can_be_called; 
        for (auto function: semantic::get_overloaded_functions(constraint.binding)) {
            if (function->args.size() == args_types.size()) {
                bool error = false;
                for (size_t i = 0; i < function->args.size(); i++) {
                    if (args_types[i].is_type_variable()) {
                        if (args_types[i].domain == "number" 
                        && !(ast::get_type((ast::Node*) function->args[i]).is_integer()
                        ||  ast::get_type((ast::Node*) function->args[i]).is_float())) {
                            error = true;
                            break;
                        }
                        else if (args_types[i].domain == "float" 
                        && !ast::get_type((ast::Node*) function->args[i]).is_float()) {
                            error = true;
                            break;
                        }
                        else {
                            continue;
                        }
                    }
                    else if (ast::get_type((ast::Node*) function->args[i]) != args_types[i]) {
                        error = true;
                        break;
                    }
                }

                if (!error) {
                    functions_that_can_be_called.push_back(function);
                }
            }
        }

        if (functions_that_can_be_called.size() == 0) {
            assert(constraint.call);
            ast::set_types(constraint.call->args, default_types);
            context.errors.push_back(errors::undefined_function(*constraint.call, context.current_module));
            return Error {};
        }
        else if (functions_that_can_be_called.size() > 1) {
            context.errors.push_back(Error{"Error: Is ambiguous what function to call"});
            return Error {};
        }

        constraint.call->function = functions_that_can_be_called[0];
        auto function = functions_that_can_be_called[0];
        for (size_t i = 0; i < function->args.size(); i++) {
            semantic::relabel(context, args_types[i], ast::get_type((ast::Node*) function->args[i]));
        }
        semantic::relabel(context, constraint.constraint.return_type, function->return_type);
    }
    else if (constraint.binding.type == semantic::GenericFunctionBinding) {
        // Get return type and analyze body
        Result<ast::Type, Error> result;
        auto function = semantic::get_generic_function(constraint.binding);
        if (context.current_module == function->module_path) {
            result = semantic::get_type_of_generic_function(context, default_types, function);
        }
        else {
            assert(false);
        }
    }
    return Ok {};
}

void semantic::relabel(Context& context, ast::Type type_variable, ast::Type new_type) {
    auto& labeled = context.type_inference.labeled_type_constraints;
    
    // Remove type variable from labeled type constraints
    for (auto it = labeled.begin(); it != labeled.end();) {
        if (semantic::contains(it->second, type_variable)) {
            if (semantic::size(it->second) == 1) {
                it = labeled.erase(it);
            }
            else {
                auto& set = it->second.elements;
                set.erase(std::find(set.begin(), set.end(), type_variable));
                it++;
            }
        }
        else {
            it++;
        }
    }

    // Re add type variable
    if (labeled.find(new_type) == labeled.end()) {
        labeled[new_type] = semantic::make_Set<ast::Type>({type_variable});
    }
    else {
        semantic::insert(labeled[new_type], type_variable);
    }
}

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
    semantic::Context context;
    init_Context(context, &ast);

    context.current_module = ast.module_path;
    semantic::analyze(context, *ast.program);

    if (context.errors.size() > 0) return context.errors;
    else                           return Ok {};
}

Result<Ok, Error> semantic::analyze_block_or_expression(semantic::Context& context, ast::Node* node) {
    assert(node->index() == ast::Block || ast::is_expression(node));

    // Do type inference and semantic analysis
    auto result = semantic::type_infer_and_analyze(context, node);
    if (result.is_error()) return Error {};

    // If we are in a function and the return type is not clear and the body of the function is an expression
    if (context.current_function.has_value()
    && context.current_function.value()->return_type.is_type_variable()
    && node->index() != ast::Block) {
        semantic::add_constraint(context, semantic::make_Set<ast::Type>({ast::get_type(node), context.current_function.value()->return_type}));
    }

    // Merge type constraints that share elements
    context.type_inference.type_constraints = semantic::merge_sets_with_shared_elements<ast::Type>(context.type_inference.type_constraints);

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
                if (representative != representatives[i]) {
                    assert(false);
                } 
            }
            context.type_inference.labeled_type_constraints[representative] = context.type_inference.type_constraints[i];
        }
    }

    // Unify of domains of type variables
    auto labeled_type_constraints = context.type_inference.labeled_type_constraints;
    for (auto it = labeled_type_constraints.begin(); it != labeled_type_constraints.end(); it++) {
        if (it->first.is_type_variable()) {
            std::string domain = it->second.elements[0].domain;
            for (size_t i = 0; i < it->second.elements.size(); i++) {
                std::string current = it->second.elements[i].domain;
                if (domain == current) {
                    continue;
                }
                else if (domain == "") {
                    domain = current;
                }
                else if (current == "") {
                    continue;
                }
                else if (domain == "number" && current == "float") {
                    domain = current;
                }
                else if (domain == "float" && current == "number") {
                    continue;
                }
                else {
                    std::cout << it->second.elements[i].to_str() << "\n";
                    std::cout << domain << " : " << current << "\n";
                    assert(false);
                }
            }

            // Update domain of type variables
            for (size_t i = 0; i < it->second.elements.size(); i++) {
                it->second.elements[i].domain = domain;
            }

            // Update key
            auto key = it->first;
            key.domain = domain;
            context.type_inference.labeled_type_constraints.erase(it->first);
            context.type_inference.labeled_type_constraints[key] = it->second;
        } 
    }

    // Check function constraints
    for (auto constraint: context.type_inference.function_constraints) {
        semantic::check_function_constraint(context, constraint);
    }

    // Unify
    return semantic::unify_types_and_type_check(context, node);
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::BlockNode& node) {
    return semantic::analyze_block_or_expression(context, (ast::Node*) &node);
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::FunctionNode& node) {
    if (node.is_extern) {
        assert(false);
    }
    if (node.generic) {
        Context new_context;
        new_context.ast = context.ast;
        new_context.current_function = &node;

        if (context.current_module == node.module_path) {
            new_context.scopes = semantic::get_definitions(context);
        }
        else {
            assert(false);
        }

        semantic::add_scope(new_context);

        // Assign a type variable to every argument and return type
        for (auto arg: node.args) {
            auto identifier = arg->value;
            if (arg->type == ast::Type("")) {
                arg->type = semantic::new_type_variable(new_context);
                semantic::add_constraint(new_context, semantic::make_Set<ast::Type>({arg->type}));
            }
            else {
                auto result = semantic::analyze(new_context, arg->type);
                if (result.is_error()) return Error {};
            }
            semantic::current_scope(new_context)[identifier] = semantic::make_Binding((ast::Node*) arg);
        }

        if (node.return_type == ast::Type("")) {
            node.return_type = semantic::new_type_variable(new_context);
            semantic::add_constraint(new_context, semantic::make_Set<ast::Type>({node.return_type}));
        }
        else {
            auto result = semantic::analyze(new_context, node.return_type);
            if (result.is_error()) return Error {};
        }

        auto result = semantic::analyze_block_or_expression(new_context, node.body);
        if (result.is_error()) {
            context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
            return Error {};
        }

        // Unify args and return type
        for (size_t i = 0; i < node.args.size(); i++) {
            node.args[i]->type = semantic::get_unified_type(new_context, node.args[i]->type);
        }
        node.return_type = semantic::get_unified_type(new_context, node.return_type);
    }
    else {
        Context new_context;
        new_context.ast = context.ast;
        new_context.current_function = &node;

        if (context.current_module == node.module_path) {
            new_context.scopes = semantic::get_definitions(context);
        }
        else {
            assert(false);
        }

        semantic::add_scope(new_context);
        for (auto arg: node.args) {
            auto identifier = arg->value;
            auto result = semantic::analyze(new_context, arg->type);
            if (result.is_error()) return Error {};
            semantic::current_scope(new_context)[identifier] = semantic::make_Binding((ast::Node*) arg);
        }

        auto result = semantic::analyze(new_context, node.return_type);
        if (result.is_error()) return Error {};
        
        result = semantic::analyze_block_or_expression(new_context, node.body);
        if (result.is_error()) {
            context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
            return Error {};
        }
    }
    return Ok {};
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::Type& type) {
    return Ok {};
}

Result<Ok, Error> semantic::analyze(semantic::Context& context, ast::TypeNode& node) {
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
    semantic::add_definitions_to_current_scope(context, node);

    // Analyze functions and types in current scope
    auto scope = semantic::current_scope(context);
    for (auto& it: scope) {
        auto& binding = it.second;
        if (binding.type == semantic::GenericFunctionBinding) {
            auto result = semantic::analyze(context, *semantic::get_generic_function(binding));
            if (result.is_error()) return result;
        }
        else if (binding.type == semantic::OverloadedFunctionsBinding) {
            auto functions = semantic::get_overloaded_functions(binding);
            for (size_t i = 0; i < functions.size(); i++) {
                auto result = semantic::analyze(context, *functions[i]);
                if (result.is_error()) return result;
            }
        }
        else if (binding.type == semantic::TypeBinding) {
            auto result = semantic::analyze(context, *semantic::get_type_definition(binding));
            if (result.is_error()) return result;
        }
    }

    size_t number_of_errors = context.errors.size();
    for (size_t i = 0; i < node.statements.size(); i++) {
        auto result = semantic::type_infer_and_analyze(context, node.statements[i]);

        // Type checking
        switch (node.statements[i]->index()) {
            case ast::Assignment: break;
            case ast::FieldAssignment: break;
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
                    if (if_type != ast::Type("")) {
                        node.type = if_type;
                    }

                    if (std::get<ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
                        auto else_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).else_branch.value());
                        if (else_type != ast::Type("")) {
                            node.type = else_type;
                        }
                    }
                }
                break;
            }
            case ast::While: break;
            case ast::Break:
            case ast::Continue: {
                if (node.type == ast::Type("")) {
                    node.type = ast::Type("void");
                }
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

    // Get identifier
    std::string identifier = node.identifier->value;

    // nonlocal assignment
    if (node.nonlocal) {
        semantic::Binding* binding = semantic::get_binding(context, identifier);
        if (!binding) {
            context.errors.push_back(errors::undefined_variable(*node.identifier, context.current_module));
            return Error {};
        }
        semantic::add_constraint(context, semantic::make_Set<ast::Type>({semantic::get_binding_type(*binding), ast::get_type(node.expression)}));
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
    
    semantic::add_constraint(context, make_Set<ast::Type>({node.identifier->type, ast::get_type(node.expression)}));
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        auto result = semantic::type_infer_and_analyze(context, node.expression.value());
        if (result.is_error()) return Error {};

        assert(context.current_function.has_value());
        semantic::add_constraint(context, make_Set<ast::Type>({context.current_function.value()->return_type, ast::get_type(node.expression.value())}));
    }
    else {
        assert(context.current_function.has_value());
        semantic::add_constraint(context, make_Set<ast::Type>({context.current_function.value()->return_type, ast::Type("void")}));
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
        node.type = semantic::new_type_variable(context);

        // Add constraints
        semantic::add_constraint(context, make_Set<ast::Type>({
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
    if (node.type == ast::Type("")) {
        node.type = semantic::new_type_variable(context);
        node.type.domain = "number";
        semantic::add_constraint(context, semantic::make_Set<ast::Type>({node.type}));
    }
    else if (!node.type.is_integer() && !node.type.is_float()) {
        context.errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FloatNode& node) {
    if (node.type == ast::Type("")) {
        node.type = semantic::new_type_variable(context);
        node.type.domain = "float";
        semantic::add_constraint(context, semantic::make_Set<ast::Type>({node.type}));
    }
    else if (!node.type.is_float()) {
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

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::CallNode& node) {
    auto& identifier = node.identifier->value;
      
    // Check binding exists
    semantic::Binding* binding = semantic::get_binding(context, identifier);
    if (!binding) {
        context.errors.push_back(errors::undefined_function(node, context.current_module));
        return Error {};
    }

    if (binding->type == semantic::TypeBinding) {
        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);
        ast::Type type = ast::Type(type_definition->identifier->value, type_definition);
        
        // Set type
        node.type = type;

        // Add type to struct_types if doesn't exists yet
        if (context.type_inference.struct_types.find(type) == context.type_inference.struct_types.end()) {
            context.type_inference.struct_types[type] = {};
        }
        semantic::StructType* struct_type = &context.type_inference.struct_types[type];

        // Add types constraints on fields
        for (size_t i = 0; i < type_definition->fields.size(); i++) {
            std::string field = type_definition->fields[i]->value;
            bool founded = false;

            for (auto& arg: node.args) {
                assert(arg->identifier.has_value());

                if (arg->identifier.value()->value == field) {
                    founded = true;

                    auto result = semantic::type_infer_and_analyze(context, node.args[i]->expression);
                    if (result.is_error()) return Error {};

                    if (struct_type->fields.find(field) == struct_type->fields.end()) {
                        struct_type->fields[field] = type_definition->fields[i]->type;
                    }
                    
                    semantic::add_constraint(context, make_Set<ast::Type>({struct_type->fields[field], ast::get_type(arg->expression)}));
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
        // Assign a type variable to every argument and return type
        for (size_t i = 0; i < node.args.size(); i++) {
            auto result = semantic::type_infer_and_analyze(context, node.args[i]->expression);
            if (result.is_error()) return Error {};
        }
        node.type = semantic::new_type_variable(context);

        // Infer things based on interface if exists
        if (interfaces.find(identifier) != interfaces.end()) {
            for (auto& interface: interfaces[identifier]) {
                if (interface.first.size() != node.args.size()) continue;

                // Create prototype type vector
                std::vector<ast::Type> prototype = ast::get_types(node.args);
                prototype.push_back(node.type);

                // Create interface prototype vector
                std::vector<ast::Type> interface_prototype = interface.first;
                interface_prototype.push_back(interface.second);

                // Infer stuff, basically is loop trough the each type of the interface prototype
                std::unordered_map<ast::Type, Set<ast::Type>> sets;
                for (size_t i = 0; i < interface_prototype.size(); i++) {
                    if (sets.find(interface_prototype[i]) == sets.end()) {
                        sets[interface_prototype[i]] = Set<ast::Type>();
                    }

                    semantic::insert(sets[interface_prototype[i]], prototype[i]);

                    if (!interface_prototype[i].is_type_variable()) {
                        semantic::insert(sets[interface_prototype[i]], interface_prototype[i]);
                    }
                }

                // Add constraints found
                for (auto it = sets.begin(); it != sets.end(); it++) {
                    semantic::add_constraint(context, it->second);
                }

                // Check binding exists
                semantic::Binding* binding = semantic::get_binding(context, identifier);
                if (!binding || !is_function(*binding)) {
                    context.errors.push_back(errors::undefined_function(node, context.current_module));
                    return Error {};
                }

                return Ok {};
            }
        }
        else if (binding->type == semantic::OverloadedFunctionsBinding) {
            // Add function constraint
            if (node.type.is_type_variable() || semantic::has_type_variables(ast::get_types(node.args))) {
                semantic::FunctionConstraint constraint = semantic::FunctionConstraint{*binding, ast::FunctionPrototype{node.identifier->value, ast::get_types(node.args), node.type}, &node};
                context.type_inference.function_constraints.push_back(constraint);
            }
        }
        else if (binding->type == semantic::GenericFunctionBinding) {
            node.function = semantic::get_generic_function(*binding);
            if (node.function->return_type == ast::Type("")) {
                semantic::Context new_context;
                semantic::init_Context(new_context, context.ast);
                new_context.scopes = semantic::get_definitions(context);
                semantic::analyze(new_context, *node.function);
            }

            // Create prototype type vector
            std::vector<ast::Type> prototype = ast::get_types(node.args);
            prototype.push_back(node.type);

            // Create interface prototype vector
            std::vector<ast::Type> function_prototype = ast::get_types(node.function->args);
            function_prototype.push_back(node.function->return_type);

            // Infer stuff, basically is loop trough the interface prototype that groups type variables
            std::unordered_map<ast::Type, Set<ast::Type>> sets;
            for (size_t i = 0; i < function_prototype.size(); i++) {
                if (sets.find(function_prototype[i]) == sets.end()) {
                    sets[function_prototype[i]] = Set<ast::Type>();
                }

                semantic::insert(sets[function_prototype[i]], prototype[i]);

                if (!function_prototype[i].is_type_variable()) {
                    semantic::insert(sets[function_prototype[i]], function_prototype[i]);
                }
            }

            // Save sets found
            for (auto it = sets.begin(); it != sets.end(); it++) {
                semantic::add_constraint(context, it->second);
            }

            // Add function constraint
            semantic::FunctionConstraint constraint = semantic::FunctionConstraint{*binding, ast::FunctionPrototype{node.identifier->value, ast::get_types(node.args), node.type}, &node};
            context.type_inference.function_constraints.push_back(constraint);
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

    if (context.type_inference.struct_types.find(node.fields_accessed[0]->type) == context.type_inference.struct_types.end()) {
        context.type_inference.struct_types[node.fields_accessed[0]->type] = {};
    }
    semantic::StructType* struct_type = &context.type_inference.struct_types[node.fields_accessed[0]->type];

    for (size_t i = 1; i < node.fields_accessed.size(); i++) {
        // Set type of field
        std::string field = node.fields_accessed[i]->value;
        if (struct_type->fields.find(field) == struct_type->fields.end()) {
            struct_type->fields[field] = semantic::new_type_variable(context);
        }
        node.fields_accessed[i]->type = struct_type->fields[field];

        // Iterate on struct_type
        if (i != node.fields_accessed.size() - 1) {
            if (context.type_inference.struct_types.find(node.fields_accessed[i]->type) == context.type_inference.struct_types.end()) {
                context.type_inference.struct_types[node.fields_accessed[i]->type] = {};
            }
            struct_type = &context.type_inference.struct_types[node.fields_accessed[i]->type];
        }
    }

    // Set overall node type
    node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

    return Ok {};
}

// Unify types and type check
// --------------------------
Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::Node* node) {
    return std::visit([&context, node](auto& variant) {return semantic::unify_types_and_type_check(context, variant);}, *node);
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::BlockNode& block) {
    for (size_t i = 0; i < block.statements.size(); i++) {
       semantic::unify_types_and_type_check(context, block.statements[i]);
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
    semantic::unify_types_and_type_check(context, node.expression);
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FieldAssignmentNode& node) {
    semantic::unify_types_and_type_check(context, *node.identifier);
    semantic::unify_types_and_type_check(context, node.expression);

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::ReturnNode& node) {
    if (node.expression.has_value()) semantic::unify_types_and_type_check(context, node.expression.value());

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
    semantic::unify_types_and_type_check(context, node.condition);
    semantic::unify_types_and_type_check(context, node.if_branch);
    if (node.else_branch.has_value()) semantic::unify_types_and_type_check(context, node.else_branch.value());

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::WhileNode& node) {
    semantic::unify_types_and_type_check(context, node.condition);
    semantic::unify_types_and_type_check(context, node.block);

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::UseNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::LinkWithNode& node) {
    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::CallArgumentNode& node) {
    return Ok {};
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

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::IntegerNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    // Add constraint
    if (semantic::in_generic_function(context)) {
        if (node.type.is_type_variable()) {
            auto function = context.current_function.value();
            auto constraint = ast::FunctionPrototype{"Number", std::vector{node.type}, ast::Type("")};
            if (std::find(function->constraints.begin(), function->constraints.end(), constraint) == function->constraints.end()) {
                function->constraints.push_back(constraint);
            }
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FloatNode& node) {
    node.type = semantic::get_unified_type(context, node.type);

    // Add constraint
    if (semantic::in_generic_function(context)) {
        if (node.type.is_type_variable()) {
            auto function = context.current_function.value();
            auto constraint = ast::FunctionPrototype{"Float", std::vector{node.type}, ast::Type("")};
            if (std::find(function->constraints.begin(), function->constraints.end(), constraint) == function->constraints.end()) {
                function->constraints.push_back(constraint);
            }
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::CallNode& node) {
    for (size_t i = 0; i < node.args.size(); i++) {
        semantic::unify_types_and_type_check(context, node.args[i]->expression);
    }

    node.type = semantic::get_unified_type(context, node.type);

    // Add constraint
    if (semantic::in_generic_function(context)) {
        if (node.type.is_type_variable() || has_type_variables(ast::get_types(node.args))) {
            auto function = context.current_function.value();
            auto constraint = ast::FunctionPrototype{node.identifier->value, ast::get_types(node.args), node.type};
            if (std::find(function->constraints.begin(), function->constraints.end(), constraint) == function->constraints.end()) {
                function->constraints.push_back(constraint);
            }
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::unify_types_and_type_check(Context& context, ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 2);

    // Get struct type
    semantic::StructType* struct_type = &context.type_inference.struct_types[node.fields_accessed[0]->type];
    
    // Unify
    semantic::unify_types_and_type_check(context, *node.fields_accessed[0]);

    for (size_t i = 1; i < node.fields_accessed.size(); i++) {
        std::string field = node.fields_accessed[i]->value;
        ast::Type type = node.fields_accessed[i]->type;

        // Get unified type
        node.fields_accessed[i]->type = semantic::get_unified_type(context, struct_type->fields[field]);
        
        // Add constraint
        if (semantic::in_generic_function(context)) {
            if (node.fields_accessed[0]->type.is_type_variable() || node.fields_accessed[i]->type.is_type_variable()) {
                auto function = context.current_function.value();
                auto constraint = ast::FunctionPrototype{"." + node.fields_accessed[i]->value, {node.fields_accessed[i - 1]->type}, node.fields_accessed[i]->type};
                if (std::find(function->constraints.begin(), function->constraints.end(), constraint) == function->constraints.end()) {
                    function->constraints.push_back(constraint);
                }
            }
        }
        
        // Iterate
        if (i != node.fields_accessed.size() - 1) {
            struct_type = &context.type_inference.struct_types[type];
        }
    }

    // Unify overall node type
    node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

    return Ok {};
}
