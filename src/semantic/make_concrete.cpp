#include "make_concrete.hpp"

// Helper functions
// ----------------
static void print_bindings(std::unordered_map<std::string, ast::Type>& type_bindings) {
    std::cout << "bindings\n";
    for (auto binding: type_bindings) {
        std::cout << "    " << binding.first << ": " << binding.second.to_str() << "\n";
    }
}
    

bool semantic::is_type_concrete(Context& context, ast::Type type) {
    if (type.is_concrete()) {
        return true;
    }
    if (type.is_final_type_variable()
    &&  context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
        return true;
    }
    return false;
}

ast::Type semantic::get_type(Context& context, ast::Type type) {
    if (type.is_final_type_variable()) {
        if (context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
            type = context.type_inference.type_bindings[type.as_final_type_variable().id];
        }
        else {
            return type;
        }
    }
    if (!type.is_concrete()) {
        for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
            type.as_nominal_type().parameters[i] = semantic::get_type_or_default(context, type.as_nominal_type().parameters[i]);
        }
    }
    return type;
}

ast::Type semantic::get_type_or_default(Context& context, ast::Type type) {
    assert(!type.is_type_variable());

    if (type.is_final_type_variable()) {
        if (context.type_inference.type_bindings.find(type.as_final_type_variable().id) != context.type_inference.type_bindings.end()) {
            type = context.type_inference.type_bindings[type.as_final_type_variable().id];
        }
        else if (context.type_inference.interface_constraints.find(type) != context.type_inference.interface_constraints.end()) {
            type = context.type_inference.interface_constraints[type].get_default_type();
        }
        else {
            std::cout << "unknown type: " << type.to_str() << "\n";
            assert(false);
        }
    }
    if (!type.is_concrete()) {
        if (type.is_nominal_type()) {
            for (size_t i = 0; i < type.as_nominal_type().parameters.size(); i++) {
                type.as_nominal_type().parameters[i] = semantic::get_type_or_default(context, type.as_nominal_type().parameters[i]);
            }
        }
        else if (type.is_struct_type()) {
            assert(false);
        }
    }
    return type;
}

ast::Type semantic::get_type_or_default(Context& context, ast::Node* node) {
    return semantic::get_type_or_default(context, ast::get_type(node));
}

std::vector<ast::Type> semantic::get_types_or_default(Context& context, std::vector<ast::Type> type_variables) {
    std::vector<ast::Type> types;
    for (size_t i = 0; i < type_variables.size(); i++) {
        types.push_back(semantic::get_type_or_default(context, type_variables[i]));
    }
    return types;
}

std::vector<ast::Type> semantic::get_types_or_default(Context& context, std::vector<ast::Node*> nodes) {
    std::vector<ast::Type> types;
    for (size_t i = 0; i < nodes.size(); i++) {
        types.push_back(semantic::get_type_or_default(context, nodes[i]));
    }
    return types;
}

// Make concrete
// -------------
Result<Ok, Error> semantic::make_concrete(Context& context, ast::Node* node, std::vector<ast::CallInCallStack> call_stack) {
    return std::visit([&context, node, &call_stack](auto& variant) {
        return semantic::make_concrete(context, variant, call_stack);
    }, *node);
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BlockNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Add scope
    semantic::add_scope(context);

    // Add functions to the current scope
    auto result = semantic::add_definitions_to_current_scope(context, node);
    if (result.is_error()) return result;

    // Make statements concrete
    size_t number_of_errors = context.errors.size();
    for (auto statement: node.statements) {
        auto result = semantic::make_concrete(context, statement, call_stack);
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

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FunctionArgumentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FunctionNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(false);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::TypeNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::DeclarationNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return semantic::make_concrete(context, node.expression, call_stack);
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::AssignmentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.assignable, call_stack);
    if (result.is_error()) return result;
    
    result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ReturnNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (node.expression.has_value()) {
        return semantic::make_concrete(context, node.expression.value(), call_stack);
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BreakNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ContinueNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IfElseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::make_concrete(context, node.if_branch, call_stack);
    if (result.is_error()) return result;
    
    if (node.else_branch.has_value()) {
        result = semantic::make_concrete(context, node.else_branch.value(), call_stack);
        if (result.is_error()) return result;
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::WhileNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.condition, call_stack);
    if (result.is_error()) return result;

    result = semantic::make_concrete(context, node.block, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::UseNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::LinkWithNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::CallArgumentNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> set_expected_types_of_arguments_and_check(semantic::Context& context, ast::CallNode* call, ast::FunctionNode* called_function, size_t from_argument, std::vector<ast::CallInCallStack> call_stack) {
    // Set and check expected types
    for (size_t i = from_argument; i < call->args.size(); i++) {
        auto arg_type = semantic::get_type(context, call->args[i]->type);
        if (!called_function->args[i]->type.is_concrete()) {
            if (arg_type.is_final_type_variable()
            &&  context.type_inference.interface_constraints.find(arg_type) != context.type_inference.interface_constraints.end()) {
                arg_type = context.type_inference.interface_constraints[arg_type].get_default_type();
            } 
        }

        // If type is concrete
        if (arg_type.is_concrete()) {
            // If type dont match with expected type
            if (called_function->args[i]->type.is_concrete()
            &&  arg_type != called_function->args[i]->type) {
                arg_type = semantic::get_type_or_default(context, call->args[i]->type);
                context.errors.push_back(errors::unexpected_argument_type(*call, context.current_module, i, arg_type, {called_function->args[i]->type}, call_stack));
                return Error {};
            }
            else {
                // Check if everything typechecks
                auto result = make_concrete(context, *call->args[i], call_stack);
                if (result.is_error()) return Error{};
            }
        }
        else {
            // Set expected type
            if (called_function->args[i]->type.is_concrete()) {
                context.type_inference.type_bindings[call->args[i]->type.as_final_type_variable().id] = called_function->args[i]->type;
            }

            // Check if it works
            auto result = make_concrete(context, *call->args[i], call_stack);
            if (result.is_error()) return Error{};
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::CallNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);
    assert(binding->type == semantic::FunctionBinding);

    assert(node.functions.size() != 0);
    auto functions_that_can_be_called = node.functions;

    if (semantic::is_type_concrete(context, node.type)) {
        // Remove functions that aren't expected
        functions_that_can_be_called = remove_incompatible_functions_with_return_type(functions_that_can_be_called, semantic::get_type_or_default(context, node.type));

        if (functions_that_can_be_called.size() == 0) {
            std::vector<ast::Type> possible_types = semantic::get_possible_types_for_return_type(node.functions);
            context.errors.push_back(errors::unexpected_return_type(node, context.current_module, semantic::get_type_or_default(context, node.type), possible_types, call_stack));
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
            auto result = make_concrete(context, *node.args[i], call_stack);
            if (result.is_error()) return result;

            // Remove functions that don't match with the type founded for the argument
            functions_that_can_be_called = semantic::remove_incompatible_functions_with_argument_type(functions_that_can_be_called, i, semantic::get_type_or_default(context, node.args[i]->type), node.args[i]->is_mutable);

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
                context.errors.push_back(errors::unexpected_argument_type(node, context.current_module, i, semantic::get_type_or_default(context, node.args[i]->type), possible_types, call_stack));
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
            auto result = semantic::get_function_type(context, &node, called_function, semantic::get_types_or_default(context, ast::get_types(node.args)), call_stack);
            if (result.is_error()) return Error {};
            if (result.get_value().is_concrete()) {
                assert(semantic::get_type(context, node.type) == result.get_value());
            }
        }
    }
    // Or get the function type
    else {
        if (called_function->state == ast::FunctionCompletelyTyped) {
            auto result =  semantic::get_type_completely_typed_function(context, called_function, semantic::get_types_or_default(context, ast::get_types(node.args)));
            if (result.is_error()) return Error{};
            semantic::unify_types(context.type_inference.type_bindings, node.type, result.get_value());
        }
        else {
            auto result = semantic::get_function_type(context, &node, called_function, semantic::get_types_or_default(context, ast::get_types(node.args)), call_stack);
            if (result.is_error()) return Error {};
            semantic::unify_types(context.type_inference.type_bindings, node.type, result.get_value());
        }
    }

    return Ok {};
}


Result<Ok, Error> semantic::make_concrete(Context& context, ast::StructLiteralNode& node, std::vector<ast::CallInCallStack> call_stack) {
    // Get binding
    semantic::Binding* binding = semantic::get_binding(context, node.identifier->value);
    assert(binding);

    for (auto field: node.fields) {
        auto result = semantic::make_concrete(context, field.second, call_stack);
        if (result.is_error()) return result;
    }
    node.type = ast::Type(node.identifier->value, semantic::get_type_definition(*binding));

    return Ok{};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FloatNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (semantic::get_type(context, node.type).is_final_type_variable()) {
        context.type_inference.type_bindings[node.type.as_final_type_variable().id] = ast::Type("float64");
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IntegerNode& node, std::vector<ast::CallInCallStack> call_stack) {
    if (semantic::get_type(context, node.type).is_final_type_variable()) {
        context.type_inference.type_bindings[node.type.as_final_type_variable().id] = ast::Type("int64");
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::IdentifierNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::BooleanNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::StringNode& node, std::vector<ast::CallInCallStack> call_stack) {
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::InterpolatedStringNode& node, std::vector<ast::CallInCallStack> call_stack) {
    for (auto expression: node.expressions) {
        auto result = semantic::make_concrete(context, expression, call_stack);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::ArrayNode& node, std::vector<ast::CallInCallStack> call_stack) {
    for (auto element: node.elements) {
        auto result = make_concrete(context, element, call_stack);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::FieldAccessNode& node, std::vector<ast::CallInCallStack> call_stack) {
    assert(node.fields_accessed.size() >= 1);

    if (!node.type.is_final_type_variable()) {
        return Ok {};
    }
    else {
        auto result = semantic::make_concrete(context, node.accessed, call_stack);
        if (result.is_error()) return Error {};

        assert(semantic::get_type(context, ast::get_type(node.accessed)).is_concrete());
        ast::Type struct_type = semantic::get_type(context, ast::get_type(node.accessed));

        for (size_t i = 0; i < node.fields_accessed.size(); i++) {
            auto field = node.fields_accessed[i]->value;
            assert(struct_type.is_nominal_type());

            bool field_founded = false;
            for (auto field_in_definition: struct_type.as_nominal_type().type_definition->fields) {
                if (field == field_in_definition->value) {
                    field_founded = true;
                    context.type_inference.type_bindings[node.fields_accessed[i]->type.as_final_type_variable().id] = field_in_definition->type;
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

Result<Ok, Error> semantic::make_concrete(Context& context, ast::AddressOfNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::DereferenceNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    assert(semantic::get_type(context, ast::get_type(node.expression)).is_pointer()
    ||     semantic::get_type(context, ast::get_type(node.expression)).is_boxed());

    semantic::unify_types(context.type_inference.type_bindings, node.type, semantic::get_type(context, ast::get_type(node.expression)).as_nominal_type().parameters[0]);
    return Ok {};
}

Result<Ok, Error> semantic::make_concrete(Context& context, ast::NewNode& node, std::vector<ast::CallInCallStack> call_stack) {
    auto result = semantic::make_concrete(context, node.expression, call_stack);
    if (result.is_error()) return result;

    return Ok {};
}

// Get type completly typed function
// ---------------------------------
Result<Ok, Error> semantic::unify_types(std::unordered_map<std::string, ast::Type>& type_bindings, ast::Type function_type, ast::Type argument_type) {
    if (function_type.is_final_type_variable()) {
        type_bindings[function_type.as_final_type_variable().id] = argument_type;
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

Result<ast::Type, Error> semantic::get_type_completely_typed_function(Context& context, ast::FunctionNode* function, std::vector<ast::Type> args) {
    std::unordered_map<std::string, ast::Type> type_bindings;

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


void add_argument_type_to_context(semantic::Context& context, ast::FunctionNode& function, ast::Type function_type, ast::Type argument_type) {
    if (function_type.is_final_type_variable()) {
        // If it was no already included
        if (context.type_inference.type_bindings.find(function_type.as_final_type_variable().id) == context.type_inference.type_bindings.end()) {
            context.type_inference.type_bindings[function_type.as_final_type_variable().id] = argument_type;
        }
        // Else compare with previous type founded for it
        else if (context.type_inference.type_bindings[function_type.as_final_type_variable().id] != argument_type) {
            assert(false);
        }

        // If function argument has field constraints
        if (function.get_type_parameter(function_type).has_value()) {
            if (function.get_type_parameter(function_type).value()->field_constraints.size() > 0) {
                for (auto field: function.get_type_parameter(function_type).value()->field_constraints) {
                    add_argument_type_to_context(context, function, field.type, get_field_type(context, field.name, argument_type).get_value());
                }
            }
        }
    }
    else if (function_type.is_concrete()
    &&       function_type != argument_type) {
        assert(false);
    }
    else if (function_type.is_array()) {
        assert(argument_type.is_array());

        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            add_argument_type_to_context(context, function, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i]);
        }
    }
    else if (function_type.is_nominal_type()) {
        assert(argument_type.is_nominal_type());
        assert(function_type.as_nominal_type().name == argument_type.as_nominal_type().name);
        assert(function_type.as_nominal_type().parameters.size() == argument_type.as_nominal_type().parameters.size());
        assert(argument_type.is_concrete());

        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            add_argument_type_to_context(context, function, function_type.as_nominal_type().parameters[i], argument_type.as_nominal_type().parameters[i]);
        }
    }
}

void get_specialization_return_type(semantic::Context& context, ast::FunctionNode& function, ast::FunctionSpecialization& specialization, ast::Type function_type, ast::Type call_type) {
    if (function_type.is_final_type_variable()) {
        if (specialization.type_bindings.find(function_type.as_final_type_variable().id) != specialization.type_bindings.end()) {
            specialization.return_type = specialization.type_bindings[function_type.as_final_type_variable().id];
        }
        else if (function.get_type_parameter(function_type).has_value()
        &&       function.get_type_parameter(function_type).value()->interface.has_value()) {
            specialization.type_bindings[function_type.as_final_type_variable().id] = function.get_type_parameter(function_type).value()->interface.value().get_default_type();
            specialization.return_type = specialization.type_bindings[function_type.as_final_type_variable().id];
        }
        else {
            assert(false);
        }

        // If function return type has field constraints
        if (function.get_type_parameter(function_type).value()->field_constraints.size() > 0) {
            for (auto field: function.get_type_parameter(function_type).value()->field_constraints) {
                get_specialization_return_type(context, function, specialization, field.type, get_field_type(context, field.name, call_type).get_value());
            }
        }
    }
    else if (function_type.is_nominal_type()
    &&       call_type.is_nominal_type()) {
        assert(function_type.as_nominal_type().name == call_type.as_nominal_type().name);
        assert(function_type.as_nominal_type().parameters.size() == call_type.as_nominal_type().parameters.size());
        assert(call_type.is_concrete());

        for (size_t i = 0; i < function_type.as_nominal_type().parameters.size(); i++) {
            get_specialization_return_type(context, function, specialization, function_type.as_nominal_type().parameters[i], call_type.as_nominal_type().parameters[i]);
        }

        specialization.return_type = semantic::get_type(context, function_type);
    }
    else if (function_type.is_nominal_type() && !function_type.is_concrete()) {
        specialization.return_type = ast::get_concrete_type(function_type, specialization.type_bindings);
    }
    else {
        specialization.return_type = function_type;
    }
}

Result<ast::Type, Error> semantic::get_function_type(Context& context, ast::CallNode* call, ast::FunctionNode* function, std::vector<ast::Type> args, std::vector<ast::CallInCallStack> call_stack) {
    // Check if specialization already exists
    for (auto& specialization: function->specializations) {
        if (specialization.args == args) {
            if (function->args.size() == 0 && !semantic::get_type(context, call->type).is_concrete()) {
                auto type_parameter = function->get_type_parameter(function->return_type);
                context.type_inference.type_bindings[call->type.as_final_type_variable().id] = type_parameter.value()->interface.value().get_default_type();
            }
            
            if (!semantic::get_type(context, call->type).is_concrete()) {
                return specialization.return_type;
            }
            else if (semantic::get_type(context, call->type) == specialization.return_type) {
                return specialization.return_type;
            }
        }
    }

    // Add call to call stack
    for (auto& call_in_stack: call_stack) {
        if (call_in_stack.identifier == call->identifier->value
        && call_in_stack.args == args) {
            if (function->return_type.is_final_type_variable()
            &&  context.type_inference.interface_constraints.find(function->return_type) != context.type_inference.interface_constraints.end()) {
                context.type_inference.type_bindings[function->return_type.as_final_type_variable().id] = context.type_inference.interface_constraints[function->return_type].get_default_type();
                return context.type_inference.type_bindings[function->return_type.as_final_type_variable().id];
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
        add_argument_type_to_context(new_context, *function, function_type, call_type);
    }
    if (semantic::get_type(context, call->type).is_concrete()) {
        add_argument_type_to_context(new_context, *function, function->return_type, semantic::get_type(context, call->type));
    }

    // Analyze function with call argument types
    auto result = semantic::make_concrete(new_context, function->body, call_stack);
    if (result.is_error()) {
        context.errors.insert(context.errors.end(), new_context.errors.begin(), new_context.errors.end());
        return Error {};
    }

    // Add specialization
    specialization.type_bindings = new_context.type_inference.type_bindings;
    for (size_t i = 0; i < args.size(); i++) {
        specialization.args.push_back(args[i]);
    }

    // Get specialization return type
    get_specialization_return_type(new_context, *function, specialization, function->return_type, semantic::get_type(context, call->type));

    function->specializations.push_back(specialization);

    return specialization.return_type;
}

// Set concrete types
// ------------------
void semantic::set_concrete_types(Context& context, ast::Node* node) {
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
                semantic::set_concrete_types(context, statement);
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

        case ast::Declaration: {
            auto& declaration = std::get<ast::DeclarationNode>(*node);
            semantic::set_concrete_types(context, declaration.expression);
            break;
        }

        case ast::Assignment: {
            auto& assignment = std::get<ast::AssignmentNode>(*node);
            semantic::set_concrete_types(context, (ast::Node*) assignment.assignable);
            semantic::set_concrete_types(context, (ast::Node*) assignment.expression);
            break;
        }

        case ast::Return: {
            auto& return_node = std::get<ast::ReturnNode>(*node);
            if (return_node.expression.has_value()) {
                semantic::set_concrete_types(context, return_node.expression.value());
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
            semantic::set_concrete_types(context, if_else.condition);
            semantic::set_concrete_types(context, if_else.if_branch);
            if (if_else.else_branch.has_value()) {
                semantic::set_concrete_types(context, if_else.else_branch.value());
            }

            if (ast::is_expression(node)) {
                if_else.type = semantic::get_type_or_default(context, if_else.type);
            }
            break;
        }

        case ast::While: {
            auto& while_node = std::get<ast::WhileNode>(*node);
            semantic::set_concrete_types(context, while_node.condition);
            semantic::set_concrete_types(context, while_node.block);
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
            semantic::set_concrete_types(context, call_argument.expression);
            call_argument.type = ast::get_type(call_argument.expression);
            break;
        }

        case ast::Call: {
            auto& call = std::get<ast::CallNode>(*node);
            for (auto arg: call.args) {
                semantic::set_concrete_types(context, (ast::Node*) arg);
            }
            
            call.type = semantic::get_type_or_default(context, call.type);
            break;
        }

        case ast::StructLiteral: {
            auto& struct_litereal = std::get<ast::StructLiteralNode>(*node);
            for (auto field: struct_litereal.fields) {
                semantic::set_concrete_types(context, field.second);
            }
            
            struct_litereal.type = semantic::get_type_or_default(context, struct_litereal.type);
            break;
        }

        case ast::Float: {
            auto& float_node = std::get<ast::FloatNode>(*node);
            float_node.type = semantic::get_type_or_default(context, float_node.type);
            break;
        }

        case ast::Integer: {
            auto& integer = std::get<ast::IntegerNode>(*node);
            integer.type = semantic::get_type_or_default(context, integer.type);
            break;
        }

        case ast::Identifier: {
            auto& identifier = std::get<ast::IdentifierNode>(*node);
            identifier.type = semantic::get_type_or_default(context, identifier.type);
            break;
        }

        case ast::Boolean: {
            break;
        }

        case ast::String: {
            break;
        }

        case ast::InterpolatedString: {
            auto& string = std::get<ast::InterpolatedStringNode>(*node);
            for (auto expression: string.expressions) {
                semantic::set_concrete_types(context, expression);
            }
            break;
        }

        case ast::Array: {
            auto& array =  std::get<ast::ArrayNode>(*node);
            for (auto element: array.elements) {
                semantic::set_concrete_types(context, element);
            }
            array.type = semantic::get_type_or_default(context, array.type);
            break;
        }

        case ast::FieldAccess: {
            auto& field_access = std::get<ast::FieldAccessNode>(*node);
            ast::set_type(field_access.accessed, semantic::get_type_or_default(context, ast::get_type(field_access.accessed)));
            for (auto field: field_access.fields_accessed) {
                field->type = semantic::get_type_or_default(context, field->type);
            }

            field_access.type = semantic::get_type_or_default(context, field_access.type);
            break;
        }

        case ast::AddressOf: {
            auto& address_of = std::get<ast::AddressOfNode>(*node);
            semantic::set_concrete_types(context, address_of.expression);
    
            assert(address_of.type.is_pointer());
            if (!address_of.type.is_concrete()) {
                address_of.type.as_nominal_type().parameters[0] = semantic::get_type_or_default(context, address_of.expression);
            }            
            break;
        }

        case ast::Dereference: {
            auto& dereference = std::get<ast::DereferenceNode>(*node);
            assert(semantic::get_type_or_default(context, dereference.expression).is_pointer()
            ||     semantic::get_type_or_default(context, dereference.expression).is_boxed());
            dereference.type = semantic::get_type_or_default(context, dereference.type);
            semantic::set_concrete_types(context, dereference.expression);
            break;
        }

        case ast::New: {
            auto& new_node = std::get<ast::NewNode>(*node);
            semantic::set_concrete_types(context, new_node.expression);
    
            assert(new_node.type.is_boxed());
            if (!new_node.type.is_concrete()) {
                new_node.type.as_nominal_type().parameters[0] = semantic::get_type_or_default(context, new_node.expression);
            }            
            break;
        }

        default: {
            assert(false);
        }
    }
}