#include "../semantic.hpp"
#include "semantic.hpp"
#include "type_infer.hpp"
#include "unify.hpp"
#include "make_concrete.hpp"

// Helper functions
// ----------------
void add_type_parameters(semantic::Context& context, ast::FunctionNode& node, ast::Type type) {
    if (type.is_type_variable()) {
        ast::TypeParameter parameter;
        parameter.type = type;

        if (context.type_inference.interface_constraints.find(type) != context.type_inference.interface_constraints.end()) {
            parameter.interface = context.type_inference.interface_constraints[type];
        }
        if (context.type_inference.field_constraints.find(type) != context.type_inference.field_constraints.end()) {
            parameter.field_constraints = context.type_inference.field_constraints[type];
        }

        if (!node.typed_parameter_aready_added(type)) {
            node.type_parameters.push_back(parameter);
        }
    }
    else if (type.is_nominal_type()) {
        for (auto parameter: type.as_nominal_type().parameters) {
            if (!parameter.is_concrete()) {
                add_type_parameters(context, node, parameter);
            }
        }
    }
    else {
        assert(false);
    }
} 

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
    semantic::Context context;
    context.init_with(&ast);

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

    // Flatten type constrains
    auto& type_constraints = context.type_inference.type_constraints;
    for (size_t i = 0; i < type_constraints.size(); i++) {
        size_t j = 0;
        while (j < type_constraints[i].size()) {
            if (!type_constraints[i].elements[j].is_nominal_type() || type_constraints[i].elements[j].as_nominal_type().parameters.size() < 0) {
                j++;
                continue;
            }

            for (size_t k = j + 1; k < type_constraints[i].size(); k++) {
                if (type_constraints[i].elements[j].is_nominal_type() && type_constraints[i].elements[k].is_nominal_type()) {
                    if (type_constraints[i].elements[j].as_nominal_type().name == type_constraints[i].elements[k].as_nominal_type().name) {
                        assert(type_constraints[i].elements[k].as_nominal_type().parameters.size() > 0);
                        semantic::add_constraint(context, Set<ast::Type>({type_constraints[i].elements[j].as_nominal_type().parameters[0], type_constraints[i].elements[k].as_nominal_type().parameters[0]}));
                    }
                }
            }

            j++;
        }
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

    // Unify
    // -----
    result = semantic::unify_types_and_type_check(context, node);
    if(result.is_error()) return Error {};

    std::unordered_map<ast::Type, ast::FieldTypes> unified_field_constraints;
    for (auto type: context.type_inference.field_constraints) {
        auto unified_type = semantic::get_unified_type(context, type.first);
        unified_field_constraints[unified_type] = {};
        for (auto field: type.second) {
            unified_field_constraints[unified_type][field.name] = semantic::get_unified_type(context, field.type);
        }
    }
    context.type_inference.field_constraints = unified_field_constraints;

    std::unordered_map<ast::Type, ast::Interface> unified_interface_constraints;
    for (auto it: context.type_inference.interface_constraints) {
        auto unified_type = semantic::get_unified_type(context, it.first);
        unified_interface_constraints[unified_type] = it.second;
    }
    context.type_inference.interface_constraints = unified_interface_constraints;

    // Make concrete
    // -------------
    if (!context.current_function.has_value()
    ||   context.current_function.value()->state == ast::FunctionCompletelyTyped) {
        result = semantic::make_concrete(context, node, {});
        if (result.is_error()) return result;
        semantic::set_concrete_types(context, node);
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
    new_context.init_with(context.ast);
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
            auto identifier = arg->identifier->value;
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
            semantic::current_scope(new_context)[identifier] = semantic::Binding((ast::Node*) arg);
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

        // Unify args and return type and add type parameters
        for (size_t i = 0; i < node.args.size(); i++) {
            node.args[i]->type = semantic::get_unified_type(new_context, node.args[i]->type);
            if (!node.args[i]->type.is_concrete()) {
                add_type_parameters(new_context, node, node.args[i]->type);
            }
        }

        node.return_type = semantic::get_unified_type(new_context, node.return_type);
        if (!node.return_type.is_concrete()) {
            add_type_parameters(new_context, node, node.return_type);
        }

        // Set function as analyzed
        node.state = ast::FunctionAnalyzed;
    }
    else if (node.state == ast::FunctionCompletelyTyped) {
        for (auto arg: node.args) {
            auto identifier = arg->identifier->value;
            auto result = semantic::analyze(new_context, arg->type);
            if (result.is_error()) {
                context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
                return Error {};
            }
            semantic::current_scope(new_context)[identifier] = semantic::Binding((ast::Node*) arg);
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
            context.errors.push_back(Error{"Errors: Undefined type  \"" + type.to_str() + "\""});
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

// Check two types are compatible
// ------------------------------
bool semantic::are_types_compatible(ast::FunctionNode& function, ast::Type function_type, ast::Type argument_type) {
    assert(argument_type.is_concrete());

    if (function_type.is_concrete()) {
        return argument_type == function_type;
    }
    else if (function_type.is_type_variable()
    &&       function.get_type_parameter(function_type).has_value()
    &&       function.get_type_parameter(function_type).value()->interface.has_value()) {
        return function.get_type_parameter(function_type).value()->interface.value().is_compatible_with(argument_type);
    }
    else if (function_type.is_nominal_type()) {
        if (function_type.as_nominal_type().name == argument_type.as_nominal_type().name
        &&  function_type.as_nominal_type().parameters.size() == argument_type.as_nominal_type().parameters.size()) {
            for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
                if (!semantic::are_types_compatible(function, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i])) {
                    return false;
                }
            }
            return true;
        }
        if (function_type.as_nominal_type().name == "array" && argument_type.is_array()) {
            for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
                if (!semantic::are_types_compatible(function, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    return true;
}

bool semantic::are_types_compatible(ast::FunctionNode& function, std::vector<ast::Type> function_types, std::vector<ast::Type> argument_types) {
    for (size_t i = 0; i < function_types.size(); i++) {
        if (!semantic::are_types_compatible(function, function_types[i], argument_types[i])) {
            return false;
        }
    }
    return true;
}