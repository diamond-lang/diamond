#include "type_infer.hpp"
#include "semantic.hpp"

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
            case ast::Declaration: break;
            case ast::Assignment: break;
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

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FunctionArgumentNode& node) {
    auto result = semantic::type_infer_and_analyze(context, *node.identifier);
    if (result.is_error()) return result;
    node.type = node.identifier->type;

    return Ok{};
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

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::DeclarationNode& node) {
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

    if (semantic::current_scope(context).find(identifier) != semantic::current_scope(context).end()
    && semantic::current_scope(context)[identifier].type == VariableBinding) {
        auto declaration = get_variable(semantic::current_scope(context)[identifier]);
        if (!declaration->is_mutable) {
            context.errors.push_back(errors::reassigning_immutable_variable(*node.identifier, *declaration, context.current_module));
            return Error {};
        }
    }
    semantic::current_scope(context)[identifier] = semantic::Binding(&node);

    return Ok{};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::AssignmentNode& node) {
    // Type infer and analyze assignable
    auto identifier = semantic::type_infer_and_analyze(context, node.assignable);
    if (identifier.is_error()) return Error {};

    // Type infer and analyze expression
    auto result = semantic::type_infer_and_analyze(context, node.expression);
    if (result.is_error()) return Error {};

    // Analyze type of expression
    if (!ast::get_type(node.expression).is_type_variable()) {
        auto type = ast::get_type(node.expression);
        auto result = semantic::analyze(context, type);
        ast::set_type(node.expression, type);
    }

    // Add constraint
    semantic::add_constraint(context, Set<ast::Type>({ast::get_type(node.assignable), ast::get_type(node.expression)}));

    // Check we aren't mutating a inmutable binding
    if (node.assignable->index() == ast::Identifier) {
         // Get identifier
        std::string identifier = ((ast::IdentifierNode*) node.assignable)->value;

        // Check variable exists
        semantic::Binding* binding = semantic::get_binding(context, identifier);
        if (!binding) {
            context.errors.push_back(errors::undefined_variable(*((ast::IdentifierNode*) node.assignable), context.current_module));
            return Error {};
        }
        semantic::add_constraint(context, Set<ast::Type>({semantic::get_binding_type(*binding), ast::get_type(node.expression)}));

        // normal assignment
        if (binding->type == VariableBinding) {
            auto declaration = get_variable(*binding);
            if (!declaration->is_mutable) {
                context.errors.push_back(errors::reassigning_immutable_variable(*((ast::IdentifierNode*) node.assignable), *declaration, context.current_module));
                return Error {};
            }
        }   
    }

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
        semantic::add_interface_constraint(context, node.type,  ast::Interface("Number"));
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
        semantic::add_interface_constraint(context, node.type,  ast::Interface("Float"));
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

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::InterpolatedStringNode& node) {
    node.type = ast::Type("string");
    for (auto expression: node.expressions) {
        auto result = semantic::type_infer_and_analyze(context, expression);
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::ArrayNode& node) {
    // Type infer and analyze elements
    for (size_t i = 0; i < node.elements.size(); i++) {
        auto result = semantic::type_infer_and_analyze(context, node.elements[i]);
        if (result.is_error()) return Error {};
    }

    // Add constraint for elements
    if (node.elements.size() > 0) {
        Set<ast::Type> constraint;
        for (auto element: node.elements) {
            constraint.insert(ast::get_type(element));
        }
        semantic::add_constraint(context, constraint);
    }

    if (node.type.is_no_type()) {
        node.type = ast::Type(ast::NominalType("array" + std::to_string(node.elements.size())));
        if (node.elements.size() > 0) {
            node.type.as_nominal_type().parameters.push_back(ast::get_type(node.elements[0]));
        }
    }
    else if (node.type.is_array() && node.type.as_nominal_type().name == "array") {
        node.type = ast::Type(ast::NominalType("array" + std::to_string(node.elements.size())));
        if (node.elements.size() > 0) {
            node.type.as_nominal_type().parameters.push_back(ast::get_type(node.elements[0]));
        }
    }

    return Ok {};
}

static std::vector<ast::Type> get_default_types(semantic::Context& context, std::vector<ast::Type> types) {
    std::vector<ast::Type> result;
    for (size_t i = 0; i < types.size(); i++) {
        assert(types[i].is_type_variable());
        assert(context.type_inference.interface_constraints.find(types[i]) != context.type_inference.interface_constraints.end());
        result.push_back(context.type_inference.interface_constraints[types[i]].get_default_type());
    }
    return result;
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
        context.errors.push_back(errors::undefined_function(node, get_default_types(context, ast::get_types(node.args)), context.current_module));
        return Error {};
    }

    assert(is_function(*binding));
    
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


Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::StructLiteralNode& node) {
    auto& identifier = node.identifier->value;

    // Analyze arguments
    for (auto field: node.fields) {
        auto result = semantic::type_infer_and_analyze(context, field.second);
        if (result.is_error()) return Error {};
    }
      
    // Check binding exists
    semantic::Binding* binding = semantic::get_binding(context, identifier);
    if (!binding) {
        std::cout << "Error: Undefined type"; 
        assert(false);
        return Error {};
    }

    ast::TypeNode* type_definition = semantic::get_type_definition(*binding);
    ast::Type type = ast::Type(type_definition->identifier->value, type_definition);
    
    // Set type
    node.type = type;

    // Add types constraints on fields
    for (size_t i = 0; i < type_definition->fields.size(); i++) {
        std::string field_name = type_definition->fields[i]->value;
        bool founded = false;

        for (auto& field: node.fields) {
            if (field.first->value == field_name) {
                founded = true;
                
                semantic::add_constraint(context, Set<ast::Type>({type_definition->fields[i]->type, ast::get_type(field.second)}));
            }
        }

        if (!founded) {
            context.errors.push_back(Error{"Error: Not all fields are initialized"});
            return Error {};
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 1);
    auto result = semantic::type_infer_and_analyze(context, node.accessed);
    if (result.is_error()) return Error {};

    if (!ast::get_type(node.accessed).is_type_variable()) {
        // Get binding
        semantic::Binding* binding = semantic::get_binding(context, ast::get_type(node.accessed).as_nominal_type().name);
        assert(binding);
        assert(binding->type == semantic::TypeBinding);

        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);

        for (size_t i = 0; i < node.fields_accessed.size(); i++) {
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
        if (context.type_inference.field_constraints.find(ast::get_type(node.accessed)) == context.type_inference.field_constraints.end()) {
            context.type_inference.field_constraints[ast::get_type(node.accessed)] = {};
        }
        ast::FieldTypes* field_constraints = &context.type_inference.field_constraints[ast::get_type(node.accessed)];

        for (size_t i = 0; i < node.fields_accessed.size(); i++) {
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
        if (ast::get_type(node.expression).is_pointer()
        ||  ast::get_type(node.expression).is_boxed()) {
            node.type = ast::get_type(node.expression).as_nominal_type().parameters[0];
        }
        else {
            node.type = semantic::new_type_variable(context);
            semantic::add_interface_constraint(context, ast::get_type(node.expression),  ast::Interface("pointer"));
        }
    }

    return Ok {};
}

Result<Ok, Error> semantic::type_infer_and_analyze(semantic::Context& context, ast::NewNode& node) {
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
        node.type = ast::Type(ast::NominalType("boxed"));
        node.type.as_nominal_type().parameters.push_back(ast::get_type(node.expression));
    }
    else if (!node.type.is_type_variable() && !(node.type.as_nominal_type().name != "boxed")) {
        context.errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }

    return Ok {};
}