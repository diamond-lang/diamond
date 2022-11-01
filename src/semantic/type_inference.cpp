#include<set>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstring>

#include "semantic.hpp"
#include "type_inference.hpp"
#include "intrinsics.hpp"

// Prototypes and definitions
// --------------------------
namespace type_inference {
    template <typename T>
    struct Set {
        std::vector<T> elements;

        void insert(T element) {
            for (size_t i = 0; i < this->elements.size(); i++) {
                if (this->elements[i] == element) {
                    return;
                }
            }
            this->elements.push_back(element);
        }

        bool contains(T element) {
            for (size_t i = 0; i < this->elements.size(); i++) {
                if (this->elements[i] == element) return true;
            }
            return false;
        }

        size_t size() {
            return this->elements.size();
        }

        void merge(Set<T>& set) {
            for (size_t i = 0; i < set.elements.size(); i++) {
                this->insert(set.elements[i]);
            }
        }

        Set<T> intersect(Set<T> set) {        
            Set<T> intersection = Set<T>();
            for (auto& element: this->elements) {
                if (set.contains(element)) {
                    intersection.elements.push_back(element);
                }
            }
            return intersection;
        }

        Set() : elements({}) {}
        Set(std::vector<T> elements) {
            for (size_t i = 0; i < elements.size(); i++) {
                this->insert(elements[i]);
            }
        }
        ~Set() {}
    };

    struct StructType {
        std::unordered_map<std::string, ast::Type> fields;
    };

    // Used for debugging
    void print(std::unordered_map<ast::Type, StructType> struct_types) {
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

    void print(Set<ast::Type> set) {
        std::cout << "{";
        for (size_t i = 0; i < set.elements.size(); i++) {
            std::cout << set.elements[i].to_str();
            if (i != set.elements.size() - 1) std::cout << ", ";
        }
        std::cout << "}\n";
    }

    void print(std::vector<Set<ast::Type>> sets) {
        for (auto& set: sets) {
            print(set);
        }
    }

    void print(std::unordered_map<ast::Type, Set<ast::Type>> labeled_sets) {
        for (auto it: labeled_sets) {
            std::cout << it.first.to_str() << ": ";
            print(it.second);
        }
    }

    struct Context {
        semantic::Context& semantic_context;
        size_t current_type_variable_number = 1;
        std::vector<Set<ast::Type>> sets;
        std::unordered_map<ast::Type, Set<ast::Type>> labeled_sets;
        ast::FunctionNode* function_node;
        std::unordered_map<ast::Type, StructType> struct_types;

        Context(semantic::Context& semantic_context) : semantic_context(semantic_context) {}

        Result<Ok, Error> analyze(ast::Node* node);
        Result<Ok, Error> analyze(ast::BlockNode& node);
        Result<Ok, Error> analyze(ast::FunctionNode& node);
        Result<Ok, Error> analyze(ast::TypeNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::FieldAssignmentNode& node);
        Result<Ok, Error> analyze(ast::AssignmentNode& node);
        Result<Ok, Error> analyze(ast::ReturnNode& node);
        Result<Ok, Error> analyze(ast::BreakNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::ContinueNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::IfElseNode& node);
        Result<Ok, Error> analyze(ast::WhileNode& node);
        Result<Ok, Error> analyze(ast::UseNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::LinkWithNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::CallArgumentNode& node) {return Ok {};}
        Result<Ok, Error> analyze(ast::CallNode& node);
        Result<Ok, Error> analyze(ast::FloatNode& node);
        Result<Ok, Error> analyze(ast::IntegerNode& node);
        Result<Ok, Error> analyze(ast::IdentifierNode& node);
        Result<Ok, Error> analyze(ast::BooleanNode& node);
        Result<Ok, Error> analyze(ast::StringNode& node);
        Result<Ok, Error> analyze(ast::FieldAccessNode& node);

        void unify(ast::Node* node);
        void unify(ast::BlockNode& node);
        void unify(ast::FunctionNode& node) {}
        void unify(ast::TypeNode& node) {}
        void unify(ast::AssignmentNode& node);
        void unify(ast::FieldAssignmentNode& node);
        void unify(ast::ReturnNode& node);
        void unify(ast::BreakNode& node) {}
        void unify(ast::ContinueNode& node) {}
        void unify(ast::IfElseNode& node);
        void unify(ast::WhileNode& node);
        void unify(ast::UseNode& node) {}
        void unify(ast::LinkWithNode& node) {}
        void unify(ast::CallArgumentNode& node) {}
        void unify(ast::CallNode& node);
        void unify(ast::FloatNode& node);
        void unify(ast::IntegerNode& node);
        void unify(ast::IdentifierNode& node);
        void unify(ast::BooleanNode& node) {}
        void unify(ast::StringNode& node) {}
        void unify(ast::FieldAccessNode& node);

        ast::Type new_type_variable() {
            ast::Type new_type = ast::Type(std::to_string(this->current_type_variable_number));
            this->current_type_variable_number++;
            return new_type;
        }

        ast::Type new_final_type_variable() {
            ast::Type new_type = ast::Type("$" + std::to_string(this->current_type_variable_number));
            this->current_type_variable_number++;
            return new_type;
        }

        void add_constraint(Set<ast::Type> constraint) {
            this->sets.push_back(constraint);
        }

        ast::Type get_unified_type(ast::Type type_var) {
            if (type_var.is_type_variable()) {
                for (auto it = this->labeled_sets.begin(); it != this->labeled_sets.end(); it++) {
                    if (it->second.contains(type_var)) {
                        return ast::Type(it->first);
                    }
                }

                ast::Type new_type_var = this->new_final_type_variable();
                this->labeled_sets[new_type_var] = Set<ast::Type>({type_var});
                return new_type_var;
            }
            return type_var;
        }
    };

    std::vector<Set<ast::Type>> merge_sets_with_shared_elements(std::vector<Set<ast::Type>> sets) {
        size_t i = 0;
        while (i < sets.size()) {
            bool merged = false;

            for (size_t j = i + 1; j < sets.size(); j++) {
                Set<ast::Type> intersection = sets[i].intersect(sets[j]);
                if (intersection.size() != 0) {
                    // Merge sets
                    sets[i].merge(sets[j]);

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

    bool has_type_variables(std::vector<ast::Type> types) {
        for (auto& type: types) {
            if (type.is_type_variable()) return true;
        }
        return false;
    }

    bool is_number(std::string str) {
        for (size_t i = 0; i < str.size(); i++) {
            if (!isdigit((int)str[i])) return false;
        }
        return true;
    }
};

// Type inference
// --------------
Result<Ok, Error> type_inference::analyze(semantic::Context& semantic_context, ast::FunctionNode* function) {
    // Create context
    type_inference::Context context(semantic_context);

    // Analyze function
    return context.analyze(*function);
}

Result<Ok, Error> type_inference::Context::analyze(ast::Node* node) {
    return std::visit([this](auto& variant) {return this->analyze(variant);}, *node);
}

Result<Ok, Error> type_inference::Context::analyze(ast::FunctionNode& node) {
    this->function_node = &node;

    // Add scope
    this->semantic_context.add_scope();

    // Assign a type variable to every argument and return type
    for (size_t i = 0; i < node.args.size(); i++) {
        if (node.args[i]->type == ast::Type("")) {
            node.args[i]->type = this->new_type_variable();
            this->add_constraint(Set<ast::Type>({node.args[i]->type}));
        }

        auto identifier = node.args[i]->value;
        this->semantic_context.current_scope()[identifier] = semantic::Binding((ast::Node*) node.args[i]);
    }
    node.return_type = this->new_type_variable();
    this->add_constraint(Set<ast::Type>({node.return_type}));

    // Analyze function body
    auto result = this->analyze(node.body);
    if (result.is_error()) return Error {};

    // Assume it's an expression if it isn't a block
    if (node.body->index() != ast::Block) {
        // Unify expression type with return type
        this->add_constraint(Set<ast::Type>({node.return_type, ast::get_type(node.body)}));
    }

    // Merge sets that share elements
    this->sets = type_inference::merge_sets_with_shared_elements(this->sets);

    // Check if return type is alone, if is alone it means the function is void
    for (size_t i = 0; i < this->sets.size(); i++) {
        if (this->sets[i].contains(node.return_type)) {
            if (this->sets[i].size() == 1) {
                this->sets[i].insert(ast::Type("void"));
            }
        }
    }

    // Label sets
    this->current_type_variable_number = 1;
    for (size_t i = 0; i < this->sets.size(); i++) {
        bool representative_found = false;
        for (auto& type: this->sets[i].elements) {
            if (!type.is_type_variable()) {
                assert(!representative_found);
                representative_found = true;
                if (this->labeled_sets.find(type) != this->labeled_sets.end()) {
                        this->labeled_sets[type].merge(this->sets[i]);
                }
                else {
                    this->labeled_sets[type] = this->sets[i];
                }
            }
        }

        if (!representative_found) {
            this->labeled_sets[this->new_final_type_variable()] = this->sets[i];
        }
    }

    // Unify
    this->unify(node.body);

    // Unify args and return type
    for (size_t i = 0; i < node.args.size(); i++) {
        node.args[i]->type = this->get_unified_type(node.args[i]->type);
    }
    node.return_type = this->get_unified_type(node.return_type);

    // Remove scope
    this->semantic_context.remove_scope();

    // Return
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::BlockNode& node) {
    this->semantic_context.add_scope();

    // Add functions to the current scope
    this->semantic_context.add_definitions_to_current_scope(node);

    // Analyze functions
    for (size_t i = 0; i < node.functions.size(); i++) {
        this->analyze(*node.functions[i]);
    }

    size_t number_of_errors = this->semantic_context.errors.size();
    for (size_t i = 0; i < node.statements.size(); i++) {
        auto result = this->analyze(node.statements[i]);

        // Type checking
        switch (node.statements[i]->index()) {
            case ast::Assignment: break;
            case ast::FieldAssignment: break;
            case ast::Call: {
                ast::CallNode* call = (ast::CallNode*) node.statements[i];

                // Is a call as an statement so its type must be void
                this->add_constraint(Set<ast::Type>({call->type, ast::Type("void")}));
                break;
            }
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

    this->semantic_context.remove_scope();

    if (this->semantic_context.errors.size() > number_of_errors) return Error {};
    else                                                         return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::AssignmentNode& node) {
    auto result = this->analyze(node.expression);
    if (result.is_error()) return Error {};

    std::string identifier = node.identifier->value;
    this->semantic_context.current_scope()[identifier] = semantic::Binding(&node);

    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::FieldAssignmentNode& node) {
    auto identifier = this->analyze(*node.identifier);
    if (identifier.is_error()) return Error {};

    auto result = this->analyze(node.expression);
    if (result.is_error()) return Error {};

    this->add_constraint(Set<ast::Type>({node.identifier->type, ast::get_type(node.expression)}));
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        auto result = this->analyze(node.expression.value());
        if (result.is_error()) return Error {};
        this->add_constraint(Set<ast::Type>({this->function_node->return_type, ast::get_type(node.expression.value())}));
    }
    else {
        this->add_constraint(Set<ast::Type>({this->function_node->return_type, ast::Type("void")}));
    }

    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::IfElseNode& node) {
    auto result = this->analyze(node.condition);
    if (result.is_error()) return Error {};

    result = this->analyze(node.if_branch);
    if (result.is_error()) return Error {};

    if (node.else_branch.has_value()) {
        result = this->analyze(node.else_branch.value());
        if (result.is_error()) return Error {};
    }

    if (ast::is_expression((ast::Node*) &node)) {
        node.type = this->new_type_variable();

        // Add constraints
        this->add_constraint(Set<ast::Type>({
            ast::get_type(node.if_branch),
            ast::get_type(node.else_branch.value()),
            node.type
        }));
    }

    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::WhileNode& node) {
    auto result = this->analyze(node.condition);
    if (result.is_error()) return Error {};

    result = this->analyze(node.block);
    if (result.is_error()) return Error {};

    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::IdentifierNode& node) {
    semantic::Binding* binding = this->semantic_context.get_binding(node.value);
    if (!binding) {
        this->semantic_context.errors.push_back(errors::undefined_variable(node, this->semantic_context.current_module)); // tested in test/errors/undefined_variable.dm
        return Error {};
    }
    else {
        node.type = semantic::get_binding_type(*binding);
    }

    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::IntegerNode& node) {
    node.type = this->new_type_variable();
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::FloatNode& node) {
    node.type = this->new_type_variable();
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::BooleanNode& node) {
    node.type = ast::Type("bool");
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::StringNode& node) {
    node.type = ast::Type("string");
    return Ok {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::CallNode& node) {
    auto& identifier = node.identifier->value;
      
    // Check binding exists
    semantic::Binding* binding = this->semantic_context.get_binding(identifier);
    if (!binding) {
        this->semantic_context.errors.push_back(errors::undefined_function(node, this->semantic_context.current_module));
        return Error {};
    }

    if (binding->type == semantic::TypeBinding) {
        ast::TypeNode* type_definition = semantic::get_type_definition(*binding);
        ast::Type type = ast::Type(type_definition->identifier->value, type_definition);
        
        // Set type
        node.type = type;

        // Add type to struct_types if doesn't exists yet
        if (this->struct_types.find(type) == this->struct_types.end()) {
            this->struct_types[type] = {};
        }
        type_inference::StructType* struct_type = &this->struct_types[type];

        // Add types constraints on fields
        for (size_t i = 0; i < type_definition->fields.size(); i++) {
            std::string field = type_definition->fields[i]->value;
            bool founded = false;

            for (auto& arg: node.args) {
                assert(arg->identifier.has_value());

                if (arg->identifier.value()->value == field) {
                    founded = true;

                    auto result = this->analyze(node.args[i]->expression);
                    if (result.is_error()) return Error {};

                    if (struct_type->fields.find(field) == struct_type->fields.end()) {
                        struct_type->fields[field] = type_definition->fields[i]->type;
                    }

                    this->add_constraint(Set<ast::Type>({struct_type->fields[field], ast::get_type(arg->expression)}));
                }
            }

            if (!founded) {
                this->semantic_context.errors.push_back(Error{"Error: Not all fields are initialized"});
                return Error {};
            }
        }

        return Ok {};
    }
    else if (is_function(*binding)) {
        // Assign a type variable to every argument and return type
        for (size_t i = 0; i < node.args.size(); i++) {
            auto result = this->analyze(node.args[i]->expression);
            if (result.is_error()) return Error {};
        }
        node.type = this->new_type_variable();

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

                    sets[interface_prototype[i]].insert(prototype[i]);

                    if (!interface_prototype[i].is_type_variable()) {
                        sets[interface_prototype[i]].insert(interface_prototype[i]);
                    }
                }

                // Add constraints found
                for (auto it = sets.begin(); it != sets.end(); it++) {
                    this->add_constraint(it->second);
                }

                // Check binding exists
                semantic::Binding* binding = this->semantic_context.get_binding(identifier);
                if (!binding || !is_function(*binding)) {
                    this->semantic_context.errors.push_back(errors::undefined_function(node, this->semantic_context.current_module));
                    return Error {};
                }

                return Ok {};
            }
        }
        
        if (binding->type == semantic::GenericFunctionBinding) {
            node.function = semantic::get_generic_function(*binding);
            if (node.function->return_type == ast::Type("")) {
                semantic::Context context(this->semantic_context.ast);
                context.scopes = this->semantic_context.get_definitions();
                context.analyze(*node.function);
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

                sets[function_prototype[i]].insert(prototype[i]);

                if (!function_prototype[i].is_type_variable()) {
                    sets[function_prototype[i]].insert(function_prototype[i]);
                }
            }

            // Save sets found
            for (auto it = sets.begin(); it != sets.end(); it++) {
                this->add_constraint(it->second);
            }
        }

        return Ok {};
    }

    assert(false);
    return Error {};
}

Result<Ok, Error> type_inference::Context::analyze(ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 2);
    auto result = this->analyze(*node.fields_accessed[0]);
    if (result.is_error()) return Error {};

    if (this->struct_types.find(node.fields_accessed[0]->type) == this->struct_types.end()) {
        this->struct_types[node.fields_accessed[0]->type] = {};
    }
    type_inference::StructType* struct_type = &this->struct_types[node.fields_accessed[0]->type];

    for (size_t i = 1; i < node.fields_accessed.size(); i++) {
        // Set type of field
        std::string field = node.fields_accessed[i]->value;
        if (struct_type->fields.find(field) == struct_type->fields.end()) {
            struct_type->fields[field] = this->new_type_variable();
        }
        node.fields_accessed[i]->type = struct_type->fields[field];

        // Iterate on struct_type
        if (i != node.fields_accessed.size() - 1) {
            if (this->struct_types.find(node.fields_accessed[i]->type) == this->struct_types.end()) {
                this->struct_types[node.fields_accessed[i]->type] = {};
            }
            struct_type = &this->struct_types[node.fields_accessed[i]->type];
        }
    }

    // Set overall node type
    node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;

    return Ok {};
}

void type_inference::Context::unify(ast::Node* node) {
    return std::visit([this](auto& variant) {return this->unify(variant);}, *node);
}

void type_inference::Context::unify(ast::BlockNode& block) {
    for (size_t i = 0; i < block.statements.size(); i++) {
       this->unify(block.statements[i]);
    }
}

void type_inference::Context::unify(ast::AssignmentNode& node) {
    this->unify(node.expression);
}

void type_inference::Context::unify(ast::FieldAssignmentNode& node) {
    this->unify(*node.identifier);
    this->unify(node.expression);
}

void type_inference::Context::unify(ast::ReturnNode& node) {
    if (node.expression.has_value()) this->unify(node.expression.value());
}

void type_inference::Context::unify(ast::IfElseNode& node) {
    if (ast::is_expression((ast::Node*) &node)) {
        node.type = this->get_unified_type(node.type);
    }
    this->unify(node.condition);
    this->unify(node.if_branch);
    if (node.else_branch.has_value()) this->unify(node.else_branch.value());
}

void type_inference::Context::unify(ast::WhileNode& node) {
    this->unify(node.condition);
    this->unify(node.block);
}

void type_inference::Context::unify(ast::IdentifierNode& node) {
    node.type = this->get_unified_type(node.type);
}

void type_inference::Context::unify(ast::IntegerNode& node) {
    node.type = this->get_unified_type(node.type);

    // Add constraint
    if (node.type.is_type_variable()) {
        auto constraint = ast::FunctionPrototype{"Number", std::vector{node.type}, ast::Type("")};
        if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
            this->function_node->constraints.push_back(constraint);
        }
    }
}

void type_inference::Context::unify(ast::FloatNode& node) {
    node.type = this->get_unified_type(node.type);

    // Add constraint
    if (node.type.is_type_variable()) {
        auto constraint = ast::FunctionPrototype{"Float", std::vector{node.type}, ast::Type("")};
        if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
            this->function_node->constraints.push_back(constraint);
        }
    }
}

void type_inference::Context::unify(ast::CallNode& node) {
    for (size_t i = 0; i < node.args.size(); i++) {
        this->unify(node.args[i]->expression);
    }

    node.type = this->get_unified_type(node.type);

    // Add constraint
    if (node.type.is_type_variable() || has_type_variables(ast::get_types(node.args))) {
        auto constraint = ast::FunctionPrototype{node.identifier->value, ast::get_types(node.args), node.type};
        if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
            this->function_node->constraints.push_back(constraint);
        }
    }
}

void type_inference::Context::unify(ast::FieldAccessNode& node) {
    assert(node.fields_accessed.size() >= 2);

    // Get struct type
    type_inference::StructType* struct_type = &this->struct_types[node.fields_accessed[0]->type];
    
    // Unify
    this->unify(*node.fields_accessed[0]);

    for (size_t i = 1; i < node.fields_accessed.size(); i++) {
        std::string field = node.fields_accessed[i]->value;
        ast::Type type = node.fields_accessed[i]->type;

        // Get unified type
        node.fields_accessed[i]->type = this->get_unified_type(struct_type->fields[field]);
        
        // Add constraint
        if (node.fields_accessed[0]->type.is_type_variable() || node.fields_accessed[i]->type.is_type_variable()) {
            auto constraint = ast::FunctionPrototype{"." + node.fields_accessed[i]->value, {node.fields_accessed[i - 1]->type}, node.fields_accessed[i]->type};
            if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
                this->function_node->constraints.push_back(constraint);
            }
        }
        
        // Iterate
        if (i != node.fields_accessed.size() - 1) {
            struct_type = &this->struct_types[type];
        }
    }

    // Unify overall node type
    node.type = node.fields_accessed[node.fields_accessed.size() - 1]->type;
}
