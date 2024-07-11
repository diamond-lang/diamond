#include "context.hpp"
#include "intrinsics.hpp"
#include "../utilities.hpp"
#include "../lexer.hpp"
#include "../parser.hpp"
#include  "../semantic.hpp"

// Bindings
// --------
semantic::Binding::Binding(ast::DeclarationNode* variable) {
    this->type = semantic::VariableBinding;
    this->value = (ast::Node*) variable;
}

semantic::Binding::Binding(ast::Node* function_argument) {
    this->type = semantic::FunctionArgumentBinding;
    this->value = (ast::Node*) function_argument;
}

semantic::Binding::Binding(ast::FunctionNode* function) {
    this->type = semantic::FunctionBinding;
    this->value = (ast::Node*) function;
}

semantic::Binding::Binding(ast::InterfaceNode* interface) {
    this->type = semantic::InterfaceBinding;
    this->value = (ast::Node*) interface;
}

semantic::Binding::Binding(ast::TypeNode* type) {
    this->type = semantic::TypeBinding;
    this->value = (ast::Node*) type;
}

ast::DeclarationNode* semantic::get_variable(semantic::Binding binding) {
    assert(binding.type == semantic::VariableBinding);
    return (ast::DeclarationNode*) binding.value;
}

ast::Node* semantic::get_function_argument(semantic::Binding binding) {
    assert(binding.type == semantic::FunctionArgumentBinding);
    return binding.value;
}

ast::FunctionNode* semantic::get_function(semantic::Binding binding) {
    assert(binding.type == semantic::FunctionBinding);
    return (ast::FunctionNode*) binding.value;
}

ast::InterfaceNode* semantic::get_interface(semantic::Binding binding) {
    assert(binding.type == semantic::InterfaceBinding);
    return (ast::InterfaceNode*) binding.value;
}

ast::TypeNode* semantic::get_type_definition(semantic::Binding binding) {
    assert(binding.type == semantic::TypeBinding);
    return (ast::TypeNode*) binding.value;
}

ast::Type semantic::get_binding_type(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::VariableBinding: return ast::get_type(((ast::DeclarationNode*) binding.value)->expression);
        case semantic::FunctionArgumentBinding: return ast::get_type(binding.value);
        case semantic::FunctionBinding: return ast::Type("function");
        case semantic::InterfaceBinding: return ast::Type("interface");
        case semantic::TypeBinding: return ast::Type("type");
    }
    assert(false);
    return ast::Type(ast::NoType{});
}

std::string semantic::get_binding_identifier(semantic::Binding& binding) {
    switch (binding.type) {
        case semantic::VariableBinding: return ((ast::DeclarationNode*) binding.value)->identifier->value;
        case semantic::FunctionArgumentBinding: return ((ast::IdentifierNode*) binding.value)->value;
        case semantic::FunctionBinding: return ((ast::FunctionNode*) binding.value)->identifier->value;
        case semantic::InterfaceBinding: return ((ast::InterfaceNode*) binding.value)->identifier->value;
        case semantic::TypeBinding: return ((ast::TypeNode*) binding.value)->identifier->value;
    }
    assert(false);
    return "";
}

// Context
// -------
void semantic::Context::init_with(ast::Ast* ast) {
    this->current_module = ast->module_path;
    this->ast = ast;

    semantic::add_scope(*this);
}

// Manage scopes
void semantic::add_scope(Context& context) {
    context.scopes.push_back(std::unordered_map<std::string, semantic::Binding>());
}

void semantic::remove_scope(Context& context) {
    for (auto binding: semantic::current_scope(context)) {
        if (binding.second.type == semantic::InterfaceBinding) {
            semantic::get_interface(binding.second)->functions = {};
        }
    }

    context.scopes.pop_back();
}

std::unordered_map<std::string, semantic::Binding>& semantic::current_scope(Context& context) {
    assert(context.scopes.size() != 0);
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

// Work with modules
Result<Ok, Error> semantic::add_definitions_to_current_scope(Context& context, std::vector<ast::FunctionNode*>& functions, std::vector<ast::InterfaceNode*>& interfaces, std::vector<ast::TypeNode*>& types) {
    auto& scope = semantic::current_scope(context);

    // Add interfaces from block to current scope
    for (auto& interface: interfaces) {
        auto& identifier = interface->identifier->value;

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = semantic::Binding(interface);
        }
        else if (scope[identifier].type == FunctionBinding) {
            std::cout << "Function \"" << identifier << "\" defined before interface" << "\n";
            assert(false);
        }
        else {
            std::cout << "Multiple definitions for interface " << interface->identifier->value << "\n";
            assert(false);
        }
    }

    // Add functions from block to current scope
    for (auto& function: functions) {
        auto& identifier = function->identifier->value;

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = semantic::Binding(function);
        }
        else if (scope[identifier].type == InterfaceBinding) {
            bool alreadyAdded = false;
            for (auto function2: get_interface(scope[identifier])->functions) {
                if (function2 == function) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                get_interface(scope[identifier])->functions.push_back(function);
            }
        }
        else if (scope[identifier].type == FunctionBinding) {
            std::cout << "Multiple definitions for function " << function->identifier->value << "\n";
            assert(false);
        }
        else {
            std::cout << scope[identifier].type << "\n";
            assert(false);
        }
    }

    // Add types from current block to current scope
    for (auto& type: types) {
        auto& identifier = type->identifier->value;

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = semantic::Binding(type);
        }
        else {
            context.errors.push_back(errors::generic_error(Location{type->line, type->column, type->module_path}, "This type is already defined."));
            return Error {};
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::add_definitions_to_current_scope(Context& context, ast::BlockNode& block) {
    // Add functions from modules
    auto current_directory = context.current_module.parent_path();
    std::set<std::filesystem::path> already_included_modules = {context.current_module};
    for (auto path: std_libs.elements) {
        already_included_modules.insert(path);
    }

    for (auto& use_stmt: block.use_statements) {
        auto module_path = std::filesystem::canonical(current_directory / (use_stmt->path->value + ".dmd"));
        assert(std::filesystem::exists(module_path));
        auto result = semantic::add_module_functions(context, module_path, already_included_modules);
        if (result.is_error()) return result;
    }

    // Add functions from current scope
    add_definitions_to_current_scope(context, block.functions, block.interfaces, block.types);

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
        add_definitions_to_current_scope(
            context,
            context.ast->modules[module_path.string()]->functions,
            context.ast->modules[module_path.string()]->interfaces,
            context.ast->modules[module_path.string()]->types
        );

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
            if (it->second.type == semantic::FunctionBinding
            ||  it->second.type == semantic::InterfaceBinding
            ||  it->second.type == semantic::TypeBinding) {
                scopes[i][it->first] = it->second;
            }
        }
    }
    return scopes;
}

// For type infer and unify
ast::Type semantic::new_type_variable(Context& context) {
    ast::Type new_type = ast::Type(ast::TypeVariable(context.type_inference.current_type_variable_number));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

void semantic::add_constraint(Context& context, Set<ast::Type> constraint) {
    context.type_inference.type_constraints.push_back(constraint);
}

void semantic::add_interface_constraint(Context& context, ast::Type type, ast::InterfaceType interface) {
    if (context.type_inference.interface_constraints.find(type) == context.type_inference.interface_constraints.end()) {
        context.type_inference.interface_constraints[type] = interface;
    }
    else if (context.type_inference.interface_constraints[type] == interface) {
        // do nothing
    }
    else {
        assert(false);
    }
}

// For unify and analyze
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
                    type_var.as_struct_type().fields[field.name] = semantic::get_unified_type(context, field.type);
                } 
            }

            return type_var;
        }
    }

    if (type_var.is_type_variable()) { 
        ast::Type new_type_var = semantic::new_final_type_variable(context);
        context.type_inference.labeled_type_constraints[new_type_var] = Set<ast::Type>({type_var});
        return new_type_var;
    }
    else if (type_var.is_nominal_type()) {
        for (size_t i = 0; i < type_var.as_nominal_type().parameters.size(); i++) {
            type_var.as_nominal_type().parameters[i] = semantic::get_unified_type(context, type_var.as_nominal_type().parameters[i]);
        }
        return type_var;
    }
    else if (type_var.is_final_type_variable()) {
        return type_var;
    }
    else {
        assert(false);
        return type_var;
    }
}

static std::string as_letter(size_t type_var) {
    char letters[] = "abcdefghijklmnopqrstuvwxyz";
    if (type_var > 25) {
        return letters[(type_var + 18) % 26] + std::to_string(1 + type_var / 26);
    }
    else {
        return std::string(1, letters[(type_var + 18) % 26]);
    }
}

ast::Type semantic::new_final_type_variable(Context& context) {
    std::string letter = as_letter(context.type_inference.current_type_variable_number);
    
    if (context.current_function.has_value()) {
        while (true) {
            bool already_used = false;
            for (auto& parameter: context.current_function.value()->type_parameters) {
                if (parameter.type.as_final_type_variable().id == letter) {
                    already_used = true;
                    break;
                }
            }

            if (!already_used) {
                break;
            }
    
            context.type_inference.current_type_variable_number++;
            letter = as_letter(context.type_inference.current_type_variable_number);
        }
    }

    ast::Type new_type = ast::Type(ast::FinalTypeVariable(as_letter(context.type_inference.current_type_variable_number)));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

std::vector<ast::FunctionNode*> semantic::remove_incompatible_functions_with_argument_type(std::vector<ast::FunctionNode*> functions, size_t argument_position, ast::Type argument_type, bool is_mutable) {
    assert(argument_type.is_concrete());
    
    functions.erase(std::remove_if(functions.begin(), functions.end(), [&argument_position, &argument_type, &is_mutable](ast::FunctionNode* function) {
        if (function->is_extern_and_variadic) {
            return false;
        }
        else {
            return semantic::are_types_compatible(*function, function->args[argument_position]->type, argument_type) == false || function->args[argument_position]->is_mutable != is_mutable;
        }
    }), functions.end());

    return functions;
}

std::vector<ast::FunctionNode*> semantic::remove_incompatible_functions_with_return_type(std::vector<ast::FunctionNode*> functions, ast::Type return_type) {
    assert(return_type.is_concrete());
    
    functions.erase(std::remove_if(functions.begin(), functions.end(), [&return_type](ast::FunctionNode* function) {
        return semantic::are_types_compatible(*function, function->return_type, return_type) == false;
    }), functions.end());

    return functions;
}

std::vector<ast::Type> semantic::get_possible_types_for_argument(std::vector<ast::FunctionNode*> functions, size_t argument_position) {
    std::vector<ast::Type> possible_types;
    for (auto function: functions) {
        if (!function->args[argument_position]->type.is_type_variable()) {
            possible_types.push_back(function->args[argument_position]->type);
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
        else {
            assert(false);
        }
    }
    return possible_types;
}
