#include "semantic.hpp"
#include "../semantic.hpp"
#include "type_inference.hpp"

// Bindings
// --------
semantic::Binding::Binding(ast::AssignmentNode* assignment) : type(AssignmentBinding), value({(ast::Node*) assignment}) {}
semantic::Binding::Binding(ast::Node* function_argument) : type(FunctionArgumentBinding), value({function_argument}) {}
semantic::Binding::Binding(ast::FunctionNode* function) : type(GenericFunctionBinding), value({(ast::Node*) function}) {}
semantic::Binding::Binding(std::vector<ast::FunctionNode*> functions) {
    this->type = semantic::OverloadedFunctionsBinding;
    for (size_t i = 0; i < functions.size(); i++) {
        this->value.push_back((ast::Node*) functions[i]);
    }
}
semantic::Binding::Binding(ast::TypeNode* type) : type(TypeBinding), value({(ast::Node*) type}) {}

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

// Helper functions
// ----------------
semantic::Context::Context(ast::Ast& ast) : ast(ast) {
    // Add intrinsic functions
    this->add_scope();

    for (auto it = intrinsics.begin(); it != intrinsics.end(); it++) {
        auto& identifier = it->first;
        auto& scope = this->current_scope();
        std::vector<ast::FunctionNode*> overloaded_functions;
        for (auto& prototype: it->second) {
            // Create function node
            auto function_node = ast::FunctionNode {};
            function_node.generic = false;

            // Create identifier node
            auto identifier_node = ast::IdentifierNode {};
            identifier_node.value = identifier;
            ast.push_back(identifier_node);

            function_node.identifier = (ast::IdentifierNode*) ast.last_element();

            // Create args nodes
            for (auto& arg: prototype.first) {
                auto arg_node = ast::IdentifierNode {};
                arg_node.type = arg;
                ast.push_back(arg_node);
                function_node.args.push_back((ast::FunctionArgumentNode*) ast.last_element());
            }

            function_node.return_type = prototype.second;

            ast.push_back(function_node);
            overloaded_functions.push_back((ast::FunctionNode*) ast.last_element());
        }
        scope[identifier] = Binding(overloaded_functions);
    }
}

void semantic::Context::add_scope() {
    this->scopes.push_back(std::unordered_map<std::string, semantic::Binding>());
}

void semantic::Context::remove_scope() {
    this->scopes.pop_back();
}

std::unordered_map<std::string, semantic::Binding>& semantic::Context::current_scope() {
    return this->scopes[this->scopes.size() - 1];
}

semantic::Binding* semantic::Context::get_binding(std::string identifier) {
    for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
        if (scope->find(identifier) != scope->end()) {
            return &(*scope)[identifier];
        }
    }
    return nullptr;
}

void semantic::Context::add_definitions_to_current_scope(ast::BlockNode& block) {
    // Add functions from block to current scope
    for (auto& function: block.functions) {
        auto& identifier = function->identifier->value;
        auto& scope = this->current_scope();

        if (scope.find(identifier) == scope.end()) {
            if (function->generic) {
                scope[identifier] = Binding(function);

            }
            else {
                scope[identifier] = Binding(std::vector{function});
            }
        }
        else if (is_function(scope[identifier])) {
            if (scope[identifier].type == GenericFunctionBinding) {
                this->errors.push_back(Error{"Error: Trying to overload generic function!"});
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
        auto& scope = this->current_scope();

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = Binding(type);
        }
        else {
            this->errors.push_back(Error{"Error: Type defined multiple times"});
        }
    }

    // Add functions from modules
    auto current_directory = this->current_module.parent_path();
    std::set<std::filesystem::path> already_included_modules = {this->current_module};

    for (auto& use_stmt: block.use_statements) {
        auto module_path = std::filesystem::canonical(current_directory / (use_stmt->path->value + ".dmd"));
        assert(std::filesystem::exists(module_path));
        this->add_module_functions(module_path, already_included_modules);
    }
}

 void semantic::Context::add_module_functions(std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules) {
    if (this->ast.modules.find(module_path.string()) == this->ast.modules.end()) {
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
        auto parsing_result = parse::module(this->ast, tokens.get_value(), module_path);
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
        for (auto& function: this->ast.modules[module_path.string()]->functions) {
            auto& identifier = function->identifier->value;
            auto& scope = this->current_scope();

            if (scope.find(identifier) == scope.end()) {
                if (function->generic) {
                    scope[identifier] = Binding(function);

                }
                else {
                    scope[identifier] = Binding(std::vector{function});
                }
            }
            else if (is_function(scope[identifier])) {
                if (scope[identifier].type == GenericFunctionBinding) {
                    this->errors.push_back(Error{"Error: Trying to overload generic function!"});
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
        for (auto& type: this->ast.modules[module_path.string()]->types) {
            auto& identifier = type->identifier->value;
            auto& scope = this->current_scope();

            if (scope.find(identifier) == scope.end()) {
                scope[identifier] = Binding(type);
            }
            else {
                this->errors.push_back(Error{"Error: Type defined multiple times"});
            }
        }

        already_included_modules.insert(module_path);

        // Add includes
        for (auto& use_stmt: this->ast.modules[module_path.string()]->use_statements) {
            if (use_stmt->include) {
                auto include_path = std::filesystem::canonical(module_path.parent_path() / (use_stmt->path->value + ".dmd"));
                this->add_module_functions(include_path, already_included_modules);
            }
        }
    }
}

std::vector<std::unordered_map<std::string, semantic::Binding>> semantic::Context::get_definitions() {
    std::vector<std::unordered_map<std::string, Binding>> scopes;
    for (size_t i = 0; i < this->scopes.size(); i++) {
        scopes.push_back(std::unordered_map<std::string, Binding>());
        for (auto it = this->scopes[i].begin(); it != this->scopes[i].end(); it++) {
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

Result<ast::Type, Error> semantic::Context::get_type_of_generic_function(std::vector<ast::Type> args, ast::FunctionNode* function, std::vector<ast::FunctionPrototype> call_stack) {
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
                this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
                return Error {};
            }
        }
        else if (type != type_variable) {
            this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
            return Error {};
        }

        specialization.args.push_back(type);
    }

    // Check constraints
    call_stack.push_back(ast::FunctionPrototype{function->identifier->value, args, ast::Type("")});
    for (auto& constraint: function->constraints) {
        this->check_constraint(specialization.type_bindings, constraint, call_stack);
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

Result<Ok, Error> semantic::Context::check_constraint(std::unordered_map<std::string, ast::Type>& type_bindings, ast::FunctionConstraint constraint, std::vector<ast::FunctionPrototype> call_stack) {
    if (constraint.identifier == "Number") {
        ast::Type type_variable = constraint.args[0];

        // If type variable was not already included
        if (type_bindings.find(type_variable.to_str()) == type_bindings.end()) {
            type_bindings[type_variable.to_str()] = ast::Type("int64");
        }
        // Else compare with previous type founded for her
        else if (type_bindings[type_variable.to_str()] != ast::Type("int64")
        &&       type_bindings[type_variable.to_str()] != ast::Type("int32")
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
        Binding* type_binding = this->get_binding(type_name);
        if (!type_binding) {
            this->errors.push_back(Error{"Errors: Undefined type"});
            return Error {};
        }
        if (type_binding->type != semantic::TypeBinding) {
            this->errors.push_back(Error{"Error: Binding is not a type\n"});
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

        this->errors.push_back(Error{"Error: Struct type doesn't have that field"});
        return Error {};
    }
    else {
        semantic::Binding* binding = this->get_binding(constraint.identifier);
        if (!binding || !is_function(*binding)) {
            this->errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
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
                        this->errors.push_back(Error{"Error: Incompatible types in function constraints"});
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
            auto result = this->get_type_of_generic_function(ast::get_concrete_types(constraint.args, type_bindings), semantic::get_generic_function(*binding), call_stack);
            if (result.is_error()) return Error {};
        }
        else {
            this->errors.push_back(Error{"Error: Undefined constraint. The function doesnt exists."});
            return Error {};
        }

        return Ok {};
    }
}

bool semantic::Context::depends_on_binding_with_concrete_type(ast::Node* node) {
    switch (node->index()) {
        case ast::Block: assert(false);
        case ast::Function: assert(false);
        case ast::Assignment: assert(false);
        case ast::Return: assert(false);
        case ast::Break: assert(false);
        case ast::Continue: assert(false);
        case ast::IfElse: {
            assert(ast::is_expression(node));
            auto& if_else = std::get<ast::IfElseNode>(*node);
            return this->depends_on_binding_with_concrete_type(if_else.if_branch)
            ||     this->depends_on_binding_with_concrete_type(if_else.else_branch.value());
        }
        case ast::While: assert(false);
        case ast::Use: assert(false);
        case ast::Call: {
            auto& call = std::get<ast::CallNode>(*node);
            for (size_t i = 0; i != call.args.size(); i++) {
                if (this->depends_on_binding_with_concrete_type(call.args[i]->expression)) {
                    return true;
                }
            }
            return false;
        }
        case ast::Float: return false;
        case ast::Integer: return false;
        case ast::Identifier: {
            auto& identifier = std::get<ast::IdentifierNode>(*node);
            semantic::Binding* binding = this->get_binding(identifier.value);
            if (binding) {
                if (binding->type == semantic::AssignmentBinding
                || binding->type == semantic::FunctionArgumentBinding) {
                    return true;
                }
            }
            return false;
        }
        case ast::Boolean: return false;
        case ast::String: return false;
        default: assert(false);
    }
    return false;
}

// Semantic analysis
// -----------------
Result<Ok, Errors> semantic::analyze(ast::Ast& ast) {
    semantic::Context context(ast);
    context.current_module = ast.module_path;
    context.analyze((ast::Node*) ast.program);

    if (context.errors.size() > 0) return context.errors;
    else                           return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::Node* node) {
    return std::visit([this](auto& variant) {return this->analyze(variant);}, *node);
}

Result<Ok, Error> semantic::Context::analyze(ast::BlockNode& node) {
    node.type = ast::Type("");
    size_t number_of_errors = this->errors.size();

    // Add scope
    this->add_scope();

    // Add functions to the current scope
    this->add_definitions_to_current_scope(node);

    // Analyze functions and types in current scope
    auto scope = this->current_scope();
    for (auto& it: scope) {
        auto& binding = it.second;
        if (binding.type == semantic::GenericFunctionBinding) {
            auto result = this->analyze(*semantic::get_generic_function(binding));
            if (result.is_error()) return result;
        }
        else if (binding.type == semantic::OverloadedFunctionsBinding) {
            auto functions = semantic::get_overloaded_functions(binding);
            for (size_t i = 0; i < functions.size(); i++) {
                auto result = this->analyze(*functions[i]);
                if (result.is_error()) return result;
            }
        }
        else if (binding.type == semantic::TypeBinding) {
            auto result = this->analyze(*semantic::get_type_definition(binding));
            if (result.is_error()) return result;
        }
    }

    // Analyze statements
    for (size_t i = 0; i < node.statements.size(); i++) {
        // Analyze
        auto result = this->analyze(node.statements[i]);

        // Type checking
        switch (node.statements[i]->index()) {
            case ast::Assignment: break;
            case ast::FieldAssignment: break;
            case ast::Call: {
                if (result.is_ok() && ast::get_type(node.statements[i]) != ast::Type("void")) {
                    this->errors.push_back(errors::unhandled_return_value(std::get<ast::CallNode>(*node.statements[i]), this->current_module)); // tested in test/errors/unhandled_return_value.dm
                }
                break;
            }
            case ast::Return: {
                if (result.is_ok()) {
                    auto return_type = std::get<ast::ReturnNode>(*node.statements[i]).expression.has_value() ? ast::get_type(std::get<ast::ReturnNode>(*node.statements[i]).expression.value()) : ast::Type("void");
                    if (node.type == ast::Type("")) node.type = return_type;
                    else if (node.type != return_type) this->errors.push_back(Error("Error: Incompatible return type"));
                }
                break;
            }
            case ast::IfElse: {
                if (result.is_ok()) {
                    auto if_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).if_branch);
                    if (if_type != ast::Type("")) {
                        if (node.type == ast::Type("")) node.type = if_type;
                        else if (node.type != if_type) this->errors.push_back(Error("Error: Incompatible return type"));
                    }

                    if (std::get<ast::IfElseNode>(*node.statements[i]).else_branch.has_value()) {
                        auto else_type = ast::get_type(std::get<ast::IfElseNode>(*node.statements[i]).else_branch.value());
                        if (else_type != ast::Type("")) {
                            if (node.type == ast::Type("")) node.type = else_type;
                            else if (node.type != else_type) this->errors.push_back(Error("Error: Incompatible return type"));
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
                else if (node.type != ast::Type("void")) {
                    this->errors.push_back(Error{"Error: Incompatible return types"});
                }
                break;
            }
            default: assert(false);
        }
    }

    // Remove scope
    this->remove_scope();

    if (this->errors.size() > number_of_errors) return Error {};
    else                                        return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::FunctionNode& node) {
    if (node.is_extern) {
        return Ok {};
    }
    
    if (node.generic) {
        if (node.return_type == ast::Type("")) {
            if (this->current_module == node.module_path) {
                auto result = type_inference::analyze(*this, &node);
                if (result.is_error()) return Error {};
            }
            else {
                Context context(this->ast);
                context.current_module = node.module_path;
                context.add_definitions_to_current_scope(*this->ast.modules[context.current_module.string()]);
                auto result = type_inference::analyze(context, &node);
                if (result.is_error()) return Error {};
            }
        }
    }
    else {
        Context context(this->ast);
        context.current_module = node.module_path;

        if (this->current_module == node.module_path) {
            context.scopes = this->get_definitions();
        }
        else {
            context.add_definitions_to_current_scope(*this->ast.modules[context.current_module.string()]);
        }

        context.add_scope();
        for (auto arg: node.args) {
            auto identifier = arg;
            context.analyze(identifier->type);
            context.current_scope()[identifier->value] = semantic::Binding((ast::Node*) arg);
        }

        context.analyze(node.return_type);

        auto result = context.analyze(node.body);
        if (result.is_error()) {
            this->errors.insert(this->errors.end(), context.errors.begin(), context.errors.end());
            return Error {};
        }
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::Type& type) {
    if (type == ast::Type("int64")) return Ok {};
    else if (type == ast::Type("float64")) return Ok {};
    else if (type == ast::Type("bool")) return Ok {};
    else {
        Binding* type_binding = this->get_binding(type.to_str());
        if (!type_binding) {
            this->errors.push_back(Error{"Errors: Undefined type"});
            return Error {};
        }
        if (type_binding->type != semantic::TypeBinding) {
            this->errors.push_back(Error{"Error: Binding is not a type\n"});
            return Error {};
        }

        type.type_definition = semantic::get_type_definition(*type_binding);
        return Ok {};
    }
    assert(false);
    return Error {};
}

Result<Ok, Error> semantic::Context::analyze(ast::TypeNode& node) {
    for (auto field: node.fields) {
        auto result = this->analyze(field->type);
        if (result.is_error()) return Error {};
    }

    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::AssignmentNode& node) {
    auto& identifier = node.identifier->value;

    // Analyze expression
    auto result = this->analyze(node.expression);
    if (result.is_error()) return result;

    // nonlocal assignment
    if (node.nonlocal) {
        Binding* binding = this->get_binding(identifier);
        if (!binding) {
            this->errors.push_back(errors::undefined_variable(*node.identifier, this->current_module));
            return Error {};
        }
        if (semantic::get_binding_type(*binding) != ast::get_type(node.expression)) {
            this->errors.push_back(std::string("Error: Incompatible type for variable"));
            return Error {};
        }
        *binding = &node;
    }

    // normal assignment
    else {
        if (this->current_scope().find(identifier) != this->current_scope().end()
        && this->current_scope()[identifier].type == AssignmentBinding) {
            auto assignment = get_assignment(this->current_scope()[identifier]);
            if (!assignment->is_mutable) {
                this->errors.push_back(errors::reassigning_immutable_variable(*node.identifier, *assignment, this->current_module));
                return Error {};
            }
        }
        this->current_scope()[identifier] = &node;
    }

    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::FieldAssignmentNode& node) {
    // Analyze field access
    auto identifier = this->analyze(*node.identifier);
    if (identifier.is_error()) return Error {};

    // Set expected type for expression if it does not have a type annotation
    if (ast::get_type(node.expression) == ast::Type("")) {
        ast::set_type(node.expression, node.identifier->type);
    }

     // Analyze expression
    auto result = this->analyze(node.expression);
    if (result.is_error()) return result;

    if (node.identifier->type != ast::get_type(node.expression)) {
        this->errors.push_back(std::string("Error: Incompatible type for variable"));
        return Error {};
    }

    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        auto result = this->analyze(node.expression.value());
        if (result.is_error()) return result;
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IfElseNode& node) {
    auto condition = this->analyze(node.condition);
    if (condition.is_error()) return condition;
    if (ast::get_type(node.condition) != ast::Type("bool")) {
        this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
    }

    if (ast::is_expression((ast::Node*) &node)) {
        auto if_branch = this->analyze(node.if_branch);
        if (if_branch.is_error()) return if_branch;

        auto else_branch = this->analyze(node.else_branch.value());
        if (else_branch.is_error()) return else_branch;

        if (ast::get_type(node.if_branch) != ast::get_type(node.else_branch.value())) {
            this->errors.push_back(std::string("Error: Both branches of if else expression must have the same type"));
            return Error {};
        }

        node.type = ast::get_type(node.if_branch);
    }
    else {
        auto if_branch = this->analyze(node.if_branch);
        if (if_branch.is_error()) return if_branch;

        if (!node.else_branch.has_value()) return Ok {};

        auto else_branch = this->analyze(node.else_branch.value());
        if (else_branch.is_error()) return else_branch;
    }

    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::WhileNode& node) {
    auto condition = this->analyze(node.condition);
    if (condition.is_error()) return condition;
    if (ast::get_type(node.condition) != ast::Type("bool")) {
        this->errors.push_back(std::string("Error: The condition in a if must be boolean"));
    }

    auto block = this->analyze(node.block);
    if (block.is_error()) return block;

    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::CallNode& node) {
    // Check function exists
    semantic::Binding* binding = this->get_binding(node.identifier->value);
    if (!binding || (!is_function(*binding) && binding->type != semantic::TypeBinding)) {
        // Analyze arguments
        for (size_t i = 0; i < node.args.size(); i++) {
            auto result = this->analyze(node.args[i]->expression);
            if (result.is_error()) return result;
        }

        this->errors.push_back(errors::undefined_function(node, this->current_module));
        return Error {};
    }

    // If is a type constructor
    if (binding->type == semantic::TypeBinding) {
        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);
       
       // Set call type
       node.type = ast::Type(node.identifier->value, type_definition);

        // Check all fields are initialized and give them a expected type
        for (auto& field: type_definition->fields) {
            auto identifier = field->value;
            bool founded = false;
            for (auto& arg: node.args) {
                assert(arg->identifier.has_value());
                if (arg->identifier.value()->value == identifier) {
                    founded = true;
                    ast::set_type(arg->expression, field->type);
                    break;
                }
            }

            if (!founded) {
                this->errors.push_back(Error{"Error: Not all fields are initialized"});
                return Error {};
            }
        }

        // Analyze arguments
        for (size_t i = 0; i < node.args.size(); i++) {
            auto result = this->analyze(node.args[i]->expression);
            if (result.is_error()) return Error {};
        }

        return Ok {};
    }

    // If is a overloaded function
    else if (binding->type == semantic::OverloadedFunctionsBinding) {
        // Analyze arguments
        for (size_t i = 0; i < node.args.size(); i++) {
            auto result = this->analyze(node.args[i]->expression);
            if (result.is_error()) return result;
        }

        // Search definition that match arguments
        for (auto function: semantic::get_overloaded_functions(*binding)) {
            // Check arguments types match
            if (ast::get_types(function->args) == ast::get_types(node.args)) {
                node.function = function;
                node.type = function->return_type;
                return Ok {};
            }
        }

        // If no function was founded for default arguments types
        std::vector<ast::FunctionNode*> functions_that_can_be_called;
        for (auto function: semantic::get_overloaded_functions(*binding)) {
            for (size_t i = 0; i < node.args.size(); i++) {
                // Set expected type 
                ast::set_type(node.args[i]->expression, function->args[i]->type);
                
                // Analyze if expression works with expected type
                std::vector<Error> errors = this->errors;
                auto result = this->analyze(node.args[i]->expression);
                this->errors = errors;
                if (result.is_error()) continue;
            }

            // Everything Ok so add function to functions_that_can_be_called
            functions_that_can_be_called.push_back(function);
        }

        if (functions_that_can_be_called.size() < 1) {
            this->errors.push_back(errors::undefined_function(node, this->current_module));
            return Error {};
        }
        if (functions_that_can_be_called.size() > 1) {
            this->errors.push_back(Error{"Error: Is ambiguous what function to call"});
            return Error {};
        }
        else {
            node.function = functions_that_can_be_called[0];
            node.type = functions_that_can_be_called[0]->return_type;
            return Ok {};
        }
    }

    // If is a generic function
    else if (binding->type == semantic::GenericFunctionBinding) {
        auto function = semantic::get_generic_function(*binding);

        std::unordered_map<std::string, ast::Type> type_bindings;
        for (size_t i = 0; i < node.args.size(); i++) {
            bool type_variable = function->args[i]->type.is_type_variable();
            bool has_type_annotation = ast::get_type(node.args[i]->expression) != ast::Type("");

            if (type_variable && (has_type_annotation || this->depends_on_binding_with_concrete_type(node.args[i]->expression))) {
                if (type_bindings.find(function->args[i]->type.to_str()) == type_bindings.end()) {
                    if (this->depends_on_binding_with_concrete_type(node.args[i]->expression)) {
                        this->analyze(node.args[i]->expression);
                    }
                    type_bindings[function->args[i]->type.to_str()] = ast::get_type(node.args[i]->expression);
                }
                else if (type_bindings[function->args[i]->type.to_str()] != ast::get_type(node.args[i]->expression)) {
                    this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
                    return Error {};
                }
            }
        }
        if (function->return_type.is_type_variable() && node.type != ast::Type("")) {
            if (type_bindings.find(function->return_type.to_str()) == type_bindings.end()) {
                type_bindings[function->return_type.to_str()] = node.type;
            }
            else if (type_bindings[function->return_type.to_str()] != node.type) {
                this->errors.push_back(Error{"Error: Incompatible types in function arguments"});
                return Error {};
            }
        }

        // Analyze arguments
        for (size_t i = 0; i < node.args.size(); i++) {
            // Set expected type
            if (type_bindings.find(function->args[i]->type.to_str()) != type_bindings.end()) {
                ast::set_type(node.args[i]->expression, type_bindings[function->args[i]->type.to_str()]);
            }

            // Analyze argument
            auto result = this->analyze(node.args[i]->expression);
            if (result.is_error()) return result;
        }

        // Get return type and analyze body
        Result<ast::Type, Error> result;
        if (this->current_module == function->module_path) {
            result = this->get_type_of_generic_function(ast::get_types(node.args), function);
        }
        else {
            Context context(this->ast);
            context.current_module = function->module_path;
            context.add_definitions_to_current_scope(*this->ast.modules[function->module_path.string()]);
            result = context.get_type_of_generic_function(ast::get_types(node.args), function);
        }

        if (result.is_ok()) {
            node.function = semantic::get_generic_function(*binding);
            node.type = result.get_value();
            return Ok {};
        }

        return Error {};
    }
    else {
        assert(false);
        return Error {};
    }
}

Result<Ok, Error> semantic::Context::analyze(ast::FloatNode& node) {
    if (node.type == ast::Type("")) {
        node.type = ast::Type("float64");
    }
    else if (node.type != ast::Type("float64")){
        this->errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IntegerNode& node) {
    if (node.type == ast::Type("")) {
        node.type = ast::Type("int64");
    }
    else if (node.type != ast::Type("int64")
    &&       node.type != ast::Type("int32")
    &&       node.type != ast::Type("float64")) {
        this->errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::IdentifierNode& node) {
    Binding* binding = this->get_binding(node.value);
    if (!binding) {
        this->errors.push_back(errors::undefined_variable(node, this->current_module)); // tested in test/errors/undefined_variable.dm
        return Error {};
    }
    else {
        if (node.type == ast::Type("")) {
            node.type = semantic::get_binding_type(*binding);
        }
        else if (node.type != semantic::get_binding_type(*binding)) {
            this->errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
            return Error {};
        }
        return Ok {};
    }
}

Result<Ok, Error> semantic::Context::analyze(ast::BooleanNode& node) {
    if (node.type == ast::Type("")) {
        node.type = ast::Type("bool");
    }
    else if (node.type != ast::Type("bool")) {
        this->errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::StringNode& node) {
    if (node.type == ast::Type("")) {
        node.type = ast::Type("string");
    }
    else if (node.type != ast::Type("string")) {
        this->errors.push_back(Error("Error: Type mismatch between type annotation and expression"));
        return Error {};
    }
    return Ok {};
}

Result<Ok, Error> semantic::Context::analyze(ast::FieldAccessNode& node) {
    // There should be at least 2 identifiers in fields accessed. eg: circle.radius
    assert(node.fields_accessed.size() >= 2);

    // Get variable binding
    Binding* variable_binding = this->get_binding(node.fields_accessed[0]->value);
    if (!variable_binding) {
        this->errors.push_back(errors::undefined_variable(*node.fields_accessed[0], this->current_module)); 
        return Error {};
    }

    // Get type binding
    std::string type_name = semantic::get_binding_type(*variable_binding).to_str();
    Binding* type_binding = this->get_binding(type_name);
    if (!type_binding) {
        this->errors.push_back(Error{"Errors: Undefined type"});
        return Error {};
    }
    if (type_binding->type != semantic::TypeBinding) {
        this->errors.push_back(Error{"Error: Binding is not a type\n"});
        return Error {};
    }
    
    // Get type definition
    ast::TypeNode* type_definition = semantic::get_type_definition(*type_binding);

    // Set type of accessed field 0
    node.fields_accessed[0]->type = ast::Type(type_name, type_definition);

    // Iterate over accessed fields
    for (size_t i = 1; i < node.fields_accessed.size() - 1; i++) {
        // Search for field in type definition
        bool founded = false;
        for (size_t j = 0; j < type_definition->fields.size(); j++) {
            if (node.fields_accessed[i]->value == type_definition->fields[j]->value) {
                type_name = type_definition->fields[j]->type.to_str();
                founded = true;
                break;
            }
        }
        if (!founded){
            this->errors.push_back(Error{"Error: Type does not have that field\n"});
            return Error {};
        }

        // Get type binding
        Binding* type_binding = this->get_binding(type_name);
        if (!type_binding) {
            this->errors.push_back(Error{"Errors: Undefined type"});
            return Error {};
        }
        if (type_binding->type != semantic::TypeBinding) {
            this->errors.push_back(Error{"Error: Binding is not a type\n"});
            return Error {};
        }
        
        // Get type definition
        type_definition = semantic::get_type_definition(*type_binding);

        // Set node type
        node.fields_accessed[i]->type = ast::Type(type_name, type_definition);
    }

    // Search for field in type definition
    size_t last_element = node.fields_accessed.size() - 1;
    for (size_t j = 0; j < type_definition->fields.size(); j++) {
        if (node.fields_accessed[last_element]->value == type_definition->fields[j]->value) {
            // Set type of last field accessed
            node.fields_accessed[last_element]->type = type_definition->fields[j]->type;

            // Set type of overall node
            node.type = type_definition->fields[j]->type;

            // Return
            return Ok {};
        }
    }

    this->errors.push_back(Error{"Error: Type does not have that field\n"});
    return Error {};
}