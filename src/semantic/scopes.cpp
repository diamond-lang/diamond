#include "scopes.hpp"
#include "intrinsics.hpp"
#include "../lexer.hpp"
#include "../semantic.hpp"

void semantic::FunctionsAndTypesScopes::add_scope() {
    this->scopes.push_back(std::unordered_map<std::string, ast::Node*>());
}

void semantic::FunctionsAndTypesScopes::remove_scope() {
    for (auto binding: this->current_scope()) {
        if (binding.second->index() == ast::Interface) {
            ((ast::InterfaceNode*)binding.second)->functions = {};
        }
    }

    this->scopes.pop_back();
}
std::unordered_map<std::string, ast::Node*>& semantic::FunctionsAndTypesScopes::current_scope() {
    assert(this->scopes.size() != 0);
    return this->scopes[this->scopes.size() - 1];
}

ast::Node* semantic::FunctionsAndTypesScopes::get_binding(std::string identifier) {
    for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
        if (scope->find(identifier) != scope->end()) {
            return (*scope)[identifier];
        }
    }
    return nullptr;
}

Result<Ok, Error> semantic::FunctionsAndTypesScopes::add_definitions_to_current_scope(std::vector<ast::FunctionNode*>& functions, std::vector<ast::InterfaceNode*>& interfaces, std::vector<ast::TypeNode*>& types) {
    auto& scope = this->current_scope();

    // Add interfaces from block to current scope
    for (auto& interface: interfaces) {
        auto& identifier = interface->identifier->value;

        if (this->get_binding(identifier) == nullptr) {
            scope[identifier] = (ast::Node*) interface;
        }
        else if (this->get_binding(identifier)->index() == ast::Function) {
            std::cout << "Function \"" << identifier << "\" defined before interface" << "\n";
            assert(false);
        }
        else if (this->get_binding(identifier)->index() == ast::Interface) {
            std::cout << "Multiple definitions for interface " << interface->identifier->value << "\n";
            assert(false);
        }
        else {
            assert(false);
        }
    }

    // Add functions from block to current scope
    for (auto& function: functions) {
        auto& identifier = function->identifier->value;

        if (this->get_binding(identifier) == nullptr) {
            scope[identifier] = (ast::Node*) function;
        }
        else if (this->get_binding(identifier)->index() == ast::Interface) {
            bool alreadyAdded = false;
            for (auto function2: ((ast::InterfaceNode*) this->get_binding(identifier))->functions) {
                if (function2 == function) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                ((ast::InterfaceNode*) this->get_binding(identifier))->functions.push_back(function);
            }
        }
        else if (this->get_binding(identifier)->index() == ast::Function) {
            std::cout << "Multiple definitions for function " << function->identifier->value << "\n";
            assert(false);
        }
        else {
            std::cout << scope[identifier]->index() << "\n";
            assert(false);
        }
    }

    // Add types from current block to current scope
    for (auto& type: types) {
        auto& identifier = type->identifier->value;

        if (scope.find(identifier) == scope.end()) {
            scope[identifier] = (ast::Node*) type;
        }
        else {
            return Error{errors::generic_error(Location{type->line, type->column, type->module_path}, "This type is already defined.")};
        }
    }

    return Ok {};
}

Result<Ok, Errors> semantic::FunctionsAndTypesScopes::add_definitions_from_block_to_scope(ast::Ast& ast, std::filesystem::path module_path, ast::BlockNode& block) {
    std::set<std::filesystem::path> already_included_modules = {module_path};

    // Add std libs
    if (this->scopes.size() == 0) {
        this->add_scope();
        if (std::find(std_libs.elements.begin(), std_libs.elements.end(), module_path) == std_libs.elements.end()) {
            for (auto path: std_libs.elements) {
                this->add_module_functions(ast, path, already_included_modules);
            }
        }
    }

    // Add scope for block definitions
    this->add_scope();

    // Add functions from modules
    auto current_directory = module_path.parent_path();
    for (auto& use_stmt: block.use_statements) {
        auto module_path = std::filesystem::canonical(current_directory / (use_stmt->path->value + ".dmd"));
        assert(std::filesystem::exists(module_path));
        auto result = this->add_module_functions(ast, module_path, already_included_modules);
        if (result.is_error()) return result;
    }

    // Add functions from current scope
    auto result = this->add_definitions_to_current_scope(block.functions, block.interfaces, block.types);
    if (result.is_error()) return Errors{result.get_error()};

    return Ok {};
}

Result<Ok, Errors> semantic::FunctionsAndTypesScopes::add_module_functions(ast::Ast& ast, std::filesystem::path module_path, std::set<std::filesystem::path>& already_included_modules) {
    if (ast.modules.find(module_path.string()) == ast.modules.end()) {
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
        auto parsing_result = parse::module(ast, tokens.get_value(), module_path);
        if (parsing_result.is_error()) {
            std::vector<Error> errors = parsing_result.get_errors();
            for (size_t i = 0; i < errors.size(); i++) {
                std::cout << errors[i].value << '\n';
            }
            exit(EXIT_FAILURE);
        }

        // Analyze new module added
        auto analyze_result = semantic::analyze_module(ast, module_path);
        if (analyze_result.is_error()) {
            return analyze_result.get_error();
        }
    }

    if (already_included_modules.find(module_path) == already_included_modules.end()) {
        this->add_definitions_to_current_scope(
            ast.modules[module_path.string()]->functions,
            ast.modules[module_path.string()]->interfaces,
            ast.modules[module_path.string()]->types
        );

        already_included_modules.insert(module_path);

        // Add includes
        for (auto& use_stmt: ast.modules[module_path.string()]->use_statements) {
            if (use_stmt->include) {
                auto include_path = std::filesystem::canonical(module_path.parent_path() / (use_stmt->path->value + ".dmd"));
                auto result = this->add_module_functions(ast, include_path, already_included_modules);
                if (result.is_error()) return result;
            }
        }
    }

    return Ok {};
}