#include "make_concrete.hpp"

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

        if (statement->index() == ast::Call) {
            if (semantic::get_type(context, ast::get_type(statement)) != ast::Type("void")) {
                context.errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*statement), context.current_module));
                return Error{};
            }
        }
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

Result<Ok, Error> set_expected_types_of_arguments_and_check(semantic::Context& context, ast::CallNode* call, ast::FunctionNode* called_function, size_t from_argument, std::vector<ast::CallInCallStack> call_stack) {
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
            if (called_function->args[i]->type.is_concrete()
            &&  arg_type != called_function->args[i]->type) {
                arg_type = ast::get_concrete_type(call->args[i]->type, context.type_inference.type_bindings);
                context.errors.push_back(errors::unexpected_argument_type(*call, context.current_module, i, arg_type, {called_function->args[i]->type}, call_stack));
                return Error {};
            }
            else if (called_function->args[i]->type.is_type_variable()
            && called_function->args[i]->type.as_type_variable().overload_constraints.size() > 0
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

Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);
    assert(binding->type == semantic::FunctionBinding);

    assert(node.functions.size() != 0);
    auto functions_that_can_be_called = node.functions;

    if (semantic::is_type_concrete(context, node.type)) {
        // Remove functions that aren't expected
        functions_that_can_be_called = remove_incompatible_functions_with_return_type(functions_that_can_be_called, ast::get_concrete_type(node.type, context.type_inference.type_bindings));

        if (functions_that_can_be_called.size() == 0) {
            std::vector<ast::Type> possible_types = semantic::get_possible_types_for_return_type(node.functions);
            context.errors.push_back(errors::unexpected_return_type(node, context.current_module, ast::get_concrete_type(node.type, context.type_inference.type_bindings), possible_types, call_stack));
            return Error {};
        }
    }

    if (functions_that_can_be_called.size() == 1) {
        // Set and check expected types of arguments
        auto result = set_expected_types_of_arguments_and_check(context, &node, functions_that_can_be_called[0], 0, call_stack);
        if (result.is_error()) return Error{};
    }
    else {
        size_t i = 0;
        while (i < node.args.size()) {     
            // Get concrete type for current argument
            auto result = get_concrete_as_type_bindings(context, *node.args[i], call_stack);
            if (result.is_error()) return result;

            // Remove functions that don't match with the type founded for the argument
            functions_that_can_be_called = semantic::remove_incompatible_functions_with_argument_type(functions_that_can_be_called, i, ast::get_concrete_type(node.args[i]->type, context.type_inference.type_bindings));

            // If only one function that can be called remains
            if (functions_that_can_be_called.size() == 1) {
                auto called_function = functions_that_can_be_called[0];

                // Check types of remaining arguments
                auto result = set_expected_types_of_arguments_and_check(context, &node, called_function, i + 1, call_stack);
                if (result.is_error()) return Error{};
                break;
            }
            else if (functions_that_can_be_called.size() == 0) {
                std::vector<ast::Type> possible_types = semantic::get_possible_types_for_argument(node.functions, i);
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
    }

    auto called_function = functions_that_can_be_called[0];

    // Check if type of called function matchs with expected
    if (semantic::is_type_concrete(context, node.type)) {
        if (called_function->state != ast::FunctionCompletelyTyped) {
            auto result = semantic::get_function_type(context, &node, called_function, ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings), call_stack);
            if (result.is_error()) return Error {};
            if (result.get_value().is_concrete()) {
                assert(semantic::get_type(context, node.type) == result.get_value());
            }
        }
    }
    // Or get the function type
    else {
        if (called_function->state == ast::FunctionCompletelyTyped) {
            auto result =  semantic::get_type_completly_typed_function(called_function, ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings));
            if (result.is_error()) return Error{};
            context.type_inference.type_bindings[node.type.as_type_variable().id] = result.get_value();
        }
        else {
            auto result = semantic::get_function_type(context, &node, called_function, ast::get_concrete_types(ast::get_types(node.args), context.type_inference.type_bindings), call_stack);
            if (result.is_error()) return Error {};
            context.type_inference.type_bindings[node.type.as_type_variable().id] = result.get_value();
        }
    }

    return Ok {};
}


Result<Ok, Error> semantic::get_concrete_as_type_bindings(Context& context, ast::StructLiteralNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    for (auto field: node.fields) {
        auto result = semantic::get_concrete_as_type_bindings(context, field.second, call_stack);
        if (result.is_error()) return result;
    }
    node.type = ast::Type(node.identifier->value, semantic::get_type_definition(*binding));

    return Ok{};
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

// Get type completly typed function
// ---------------------------------
Result<Ok, Error> unify_types(std::unordered_map<size_t, ast::Type>& type_bindings, ast::Type function_type, ast::Type argument_type) {
    if (function_type.is_type_variable()) {
        type_bindings[function_type.as_type_variable().id] = argument_type;
    }
    else if (function_type.is_nominal_type()) {
        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            auto result = unify_types(type_bindings, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i]);
            if (result.is_error()) return Error{};
        }
    }
    else {
        assert(false);
    }

    return Ok{};
}

Result<ast::Type, Error> semantic::get_type_completly_typed_function(ast::FunctionNode* function, std::vector<ast::Type> args) {
    std::unordered_map<size_t, ast::Type> type_bindings;

    for (size_t i = 0; i < function->args.size(); i++) {
        ast::Type& function_type = function->args[i]->type;
        if (function_type.is_concrete()) {
            continue;
        }

        auto result = unify_types(type_bindings, function_type, args[i]);
        if (result.is_error()) return Error{};
    }

    return ast::get_concrete_type(function->return_type, type_bindings);
}

// Get function type
// -----------------
Result<ast::Type, Error> get_field_type(semantic::Context& context, std::string field, ast::Type struct_type) {
    assert(struct_type.is_nominal_type() && struct_type.as_nominal_type().type_definition);

    for (size_t i = 0; i < struct_type.as_nominal_type().type_definition->fields.size(); i++) {
        if (field == struct_type.as_nominal_type().type_definition->fields[i]->value) {
            return struct_type.as_nominal_type().type_definition->fields[i]->type;
        }
    }

    assert(false);
    return Error{};
}


void add_argument_type_to_context(semantic::Context& context, ast::Type function_type, ast::Type argument_type) {
    if (function_type.is_type_variable()) {
        // If it was no already included
        if (context.type_inference.type_bindings.find(function_type.as_type_variable().id) == context.type_inference.type_bindings.end()) {
            context.type_inference.type_bindings[function_type.as_type_variable().id] = argument_type;
        }
        // Else compare with previous type founded for it
        else if (context.type_inference.type_bindings[function_type.as_type_variable().id] != argument_type) {
            assert(false);
        }

        // If function argument has field constraints
        if (function_type.as_type_variable().field_constraints.size() > 0) {
            for (auto field: function_type.as_type_variable().field_constraints) {
                add_argument_type_to_context(context, field.type, get_field_type(context, field.name, argument_type).get_value());
            }
        }
    }
    else if (function_type.is_concrete()
    &&       function_type != argument_type) {
        assert(false);
    }
    else if (function_type.is_nominal_type()) {
        assert(argument_type.is_nominal_type());
        assert(function_type.as_nominal_type().name == argument_type.as_nominal_type().name);
        assert(function_type.as_nominal_type().parameters.size() == argument_type.as_nominal_type().parameters.size());
        assert(argument_type.is_concrete());

        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            add_argument_type_to_context(context, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i]);
        }
    }
}

void add_return_type_to_context(semantic::Context& context, ast::FunctionSpecialization& specialization, ast::Type function_type, ast::Type call_type) {
    if (function_type.is_type_variable()) {
        if (specialization.type_bindings.find(function_type.as_type_variable().id) != specialization.type_bindings.end()) {
            specialization.return_type = specialization.type_bindings[function_type.as_type_variable().id];
        }
        else if (function_type.is_type_variable()
        &&       function_type.as_type_variable().interface.has_value()) {
            specialization.type_bindings[function_type.as_type_variable().id] = function_type.as_type_variable().interface.value().get_default_type();
            specialization.return_type = specialization.type_bindings[function_type.as_type_variable().id];
        }
        else {
            assert(false);
        }

        // If function return type has field constraints
        if (function_type.as_type_variable().field_constraints.size() > 0) {
            for (auto field: function_type.as_type_variable().field_constraints) {
                add_return_type_to_context(context, specialization, field.type, get_field_type(context, field.name, call_type).get_value());
            }
        }
    }
    else if (function_type.is_nominal_type()
    &&       call_type.is_nominal_type()) {
        assert(function_type.as_nominal_type().name == call_type.as_nominal_type().name);
        assert(function_type.as_nominal_type().parameters.size() == call_type.as_nominal_type().parameters.size());
        assert(call_type.is_concrete());

        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            add_return_type_to_context(context, specialization, function_type.as_nominal_type().parameters[i], call_type.as_nominal_type().parameters[i]);
        }

        specialization.return_type = semantic::get_type(context, function_type);
    }
    else {
        specialization.return_type = function_type;
    }
}

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
                return function->return_type;
            }
        }
    }
    call_stack.push_back(ast::CallInCallStack(function->identifier->value, args, call, function, context.current_module));

    // Else type check and create specialization type
    ast::FunctionSpecialization specialization;

    // Create new context
    Context new_context;
    new_context.init_with(context.ast);
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
        add_argument_type_to_context(new_context, function_type, call_type);
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

    add_return_type_to_context(new_context, specialization, function->return_type, call->type);

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

        case ast::StructLiteral: {
            auto& struct_litereal = std::get<ast::StructLiteralNode>(*node);
            for (auto field: struct_litereal.fields) {
                semantic::make_concrete(context, field.second);
            }
            
            struct_litereal.type = ast::get_concrete_type(struct_litereal.type, context.type_inference.type_bindings);
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
            auto& array =  std::get<ast::ArrayNode>(*node);
            for (auto element: array.elements) {
                semantic::make_concrete(context, element);
            }
            array.type = ast::get_concrete_type(array.type, context.type_inference.type_bindings);
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