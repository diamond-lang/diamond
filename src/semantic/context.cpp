#include "context.hpp"
#include "intrinsics.hpp"
#include "../utilities.hpp"
#include "../lexer.hpp"
#include "../parser.hpp"
#include  "../semantic.hpp"
#include "semantic.hpp"

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
}

// Manage scopes
void semantic::add_scope(Context& context) {
    context.scopes.variables_scopes.push_back({});
}

Result<Ok, Error> semantic::add_scope(Context& context, ast::BlockNode& block) {
    context.scopes.variables_scopes.push_back({});
    auto result = context.scopes.functions_and_types_scopes.add_definitions_from_block_to_scope(*context.ast, context.current_module, block);
    if (result.is_error()) {
        auto errors = result.get_error();
        context.errors.insert(context.errors.end(), errors.begin(), errors.end());
        return Error{};
    }
    return Ok{};
}

void semantic::remove_scope(Context& context) {
    context.scopes.variables_scopes.pop_back();
    context.scopes.functions_and_types_scopes.remove_scope();
}

semantic::Scope semantic::current_scope(Context& context) {
    assert(context.scopes.variables_scopes.size() != 0);
    return semantic::Scope{
        context.scopes.variables_scopes[context.scopes.variables_scopes.size() - 1],
        context.scopes.functions_and_types_scopes.scopes[context.scopes.functions_and_types_scopes.scopes.size() - 1]
    };
}

std::optional<semantic::Binding> semantic::get_binding(Context& context, std::string identifier) {
    for (auto scope = context.scopes.variables_scopes.rbegin(); scope != context.scopes.variables_scopes.rend(); scope++) {
        if (scope->find(identifier) != scope->end()) {
            return (*scope)[identifier];
        }
    }
    for (auto scope = context.scopes.functions_and_types_scopes.scopes.rbegin(); scope != context.scopes.functions_and_types_scopes.scopes.rend(); scope++) {
        if (scope->find(identifier) != scope->end()) {
            if ((*scope)[identifier]->index() == ast::Interface) {
                return Binding((ast::InterfaceNode*) (*scope)[identifier]);
            }
            else if ((*scope)[identifier]->index() == ast::Function) {
                return Binding((ast::FunctionNode*) (*scope)[identifier]);
            }
            else if ((*scope)[identifier]->index() == ast::TypeDef) {
                return Binding((ast::TypeNode*) (*scope)[identifier]);
            }
        }
    }
    return std::nullopt;
}

// Work with modules
semantic::Scopes semantic::get_definitions(Context& context) {
    Scopes scopes;
    scopes.functions_and_types_scopes = context.scopes.functions_and_types_scopes;
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
    assert(type.to_str() != "t");
    if (context.type_inference.interface_constraints.find(type) == context.type_inference.interface_constraints.end()) {
        context.type_inference.interface_constraints[type] = Set<ast::InterfaceType>();
    }

    context.type_inference.interface_constraints[type].insert(interface);
}

void semantic::add_parameter_constraint(Context& context, ast::Type type, ast::Type parameter) {
    if (context.type_inference.parameter_constraints[type].size() == 0) {
        context.type_inference.parameter_constraints[type] = {parameter};
    }
    else {
        semantic::add_constraint(context, Set<ast::Type>({context.type_inference.parameter_constraints[type][0], parameter}));
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

void semantic::set_unified_type(Context& context, ast::Type type_var, ast::Type new_type) {
    auto it = context.type_inference.labeled_type_constraints.find(type_var);

    if (it == context.type_inference.labeled_type_constraints.end()) {
        context.type_inference.labeled_type_constraints[new_type] = Set<ast::Type>();
        context.type_inference.labeled_type_constraints[new_type].insert(type_var);
    }
    else {
        Set<ast::Type> set = context.type_inference.labeled_type_constraints[type_var];
        context.type_inference.labeled_type_constraints.erase(it);

        if (context.type_inference.labeled_type_constraints.find(new_type) == context.type_inference.labeled_type_constraints.end()) {
            context.type_inference.labeled_type_constraints[new_type] = set;
        }
        else {
            for (auto type: set.elements) {
                context.type_inference.labeled_type_constraints[new_type].insert(type);
            }
        }
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
