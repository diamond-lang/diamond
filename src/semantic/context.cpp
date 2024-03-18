#include "context.hpp"
#include "intrinsics.hpp"
#include "../utilities.hpp"
#include "../lexer.hpp"
#include "../parser.hpp"
#include  "../semantic.hpp"

// Bindings
// --------
semantic::Binding::Binding(ast::AssignmentNode* assignment) {
    this->type = semantic::AssignmentBinding;
    this->value.push_back((ast::Node*) assignment);
}

semantic::Binding::Binding(ast::Node* function_argument) {
    this->type = semantic::FunctionArgumentBinding;
    this->value.push_back((ast::Node*) function_argument);
}

semantic::Binding::Binding(std::vector<ast::FunctionNode*> functions) {
    this->type = semantic::FunctionBinding;
    for (size_t i = 0; i < functions.size(); i++) {
        this->value.push_back((ast::Node*) functions[i]);
    }
}

semantic::Binding::Binding(ast::TypeNode* type) {
    this->type = semantic::TypeBinding;
    this->value.push_back((ast::Node*) type);
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
void semantic::Context::init_with(ast::Ast* ast) {
    this->current_module = ast->module_path;
    this->ast = ast;

    // Add intrinsic functions
    semantic::add_scope(*this);

    for (auto it = intrinsic_functions.begin(); it != intrinsic_functions.end(); it++) {
        auto& identifier = it->first;
        auto& scope = current_scope(*this);
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
        scope[identifier] = semantic::Binding(overloaded_functions);
    }
}

// Manage scopes
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

// Work with modules
Result<Ok, Error> semantic::add_definitions_to_current_scope(Context& context, ast::BlockNode& block) {
    // Add functions from block to current scope
    for (auto& function: block.functions) {
        auto& identifier = function->identifier->value;
        auto& scope = semantic::current_scope(context);

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = semantic::Binding(std::vector{function});
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
            scope[identifier] = semantic::Binding(type);
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
                scope[identifier] = semantic::Binding(std::vector{function});
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
                scope[identifier] = semantic::Binding(type);
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

// For type infer and unify
ast::Type semantic::new_type_variable(Context& context) {
    ast::Type new_type = ast::Type(ast::TypeVariable(context.type_inference.current_type_variable_number));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

void semantic::add_constraint(Context& context, Set<ast::Type> constraint) {
    context.type_inference.type_constraints.push_back(constraint);
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

ast::Type semantic::new_final_type_variable(Context& context) {
    ast::Type new_type = ast::Type(ast::TypeVariable(context.type_inference.current_type_variable_number, true));
    context.type_inference.current_type_variable_number++;
    return new_type;
}

std::vector<ast::FunctionNode*> semantic::remove_incompatible_functions_with_argument_type(std::vector<ast::FunctionNode*> functions, size_t argument_position, ast::Type argument_type) {
    assert(argument_type.is_concrete());
    
    functions.erase(std::remove_if(functions.begin(), functions.end(), [&argument_position, &argument_type](ast::FunctionNode* function) {
        return semantic::are_types_compatible(*function, function->args[argument_position]->type, argument_type) == false;
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
        else if (function->args[argument_position]->type.is_type_variable()
        &&       function->get_type_parameter(function->args[argument_position]->type).value()->overload_constraints.size() > 0) {
            for (auto type: function->get_type_parameter(function->args[argument_position]->type).value()->overload_constraints.elements) {
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
        &&       function->get_type_parameter(function->return_type).value()->overload_constraints.size() > 0) {
            for (auto type: function->get_type_parameter(function->return_type).value()->overload_constraints.elements) {
                possible_types.push_back(type);
            }
        }
        else {
            assert(false);
        }
    }
    return possible_types;
}
