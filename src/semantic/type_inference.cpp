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
    struct Context {
        semantic::Context& semantic_context;
        size_t current_type_variable_number = 1;
        std::vector<std::set<std::string>> sets;
        std::unordered_map<std::string, std::set<std::string>> labeled_sets;
        ast::FunctionNode* function_node;
        std::vector<ast::FunctionNode*> call_stack;

        Context(semantic::Context& semantic_context) : semantic_context(semantic_context) {}

        void analyze(ast::Node* node);
		void analyze(ast::BlockNode& node);
		void analyze(ast::FunctionNode& node);
		void analyze(ast::AssignmentNode& node);
		void analyze(ast::ReturnNode& node);
		void analyze(ast::BreakNode& node) {}
		void analyze(ast::ContinueNode& node) {}
		void analyze(ast::IfElseNode& node);
		void analyze(ast::WhileNode& node);
		void analyze(ast::UseNode& node) {}
		void analyze(ast::IncludeNode& node) {}
		void analyze(ast::CallNode& node);
		void analyze(ast::FloatNode& node);
		void analyze(ast::IntegerNode& node);
		void analyze(ast::IdentifierNode& node);
		void analyze(ast::BooleanNode& node);
		void analyze(ast::StringNode& node) {}

        void unify(ast::Node* node);
		void unify(ast::BlockNode& node);
		void unify(ast::FunctionNode& node) {}
		void unify(ast::AssignmentNode& node);
		void unify(ast::ReturnNode& node);
		void unify(ast::BreakNode& node) {}
		void unify(ast::ContinueNode& node) {}
		void unify(ast::IfElseNode& node);
		void unify(ast::WhileNode& node);
		void unify(ast::UseNode& node) {}
		void unify(ast::IncludeNode& node) {}
		void unify(ast::CallNode& node);
		void unify(ast::FloatNode& node);
		void unify(ast::IntegerNode& node);
		void unify(ast::IdentifierNode& node);
		void unify(ast::BooleanNode& node) {}
		void unify(ast::StringNode& node) {}

        ast::Type get_unified_type(std::string type_var) {
            std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
            for (auto it = this->labeled_sets.begin(); it != this->labeled_sets.end(); it++) {
                if (it->second.count(type_var) != 0) {
                    return ast::Type(it->first);
                }
            }

            assert(this->current_type_variable_number != alphabet.size());
            auto result = ast::Type("$" + alphabet[(this->current_type_variable_number + 18) % alphabet.size()]);
            this->current_type_variable_number++;
            return result;
        }
    };

    bool is_number(std::string str) {
        for (size_t i = 0; i < str.size(); i++) {
            if (!isdigit((int)str[i])) return false;
        }
        return true;
    }

    std::vector<std::set<std::string>> merge_sets_with_shared_elements(std::vector<std::set<std::string>> sets) {
        size_t i = 0;
        while (i < sets.size()) {
            bool merged = false;
            
            for (size_t j = i + 1; j < sets.size(); j++) {
                std::set<std::string> intersect;
                std::set_intersection(sets[i].begin(), sets[i].end(), sets[j].begin(), sets[j].end(), std::inserter(intersect, intersect.begin()));
                
                if (intersect.size() != 0) {
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
};

// Type inference
// --------------
void type_inference::analyze(semantic::Context& semantic_context, ast::FunctionNode* function) {
    // Create context
    type_inference::Context context(semantic_context);

    // Analyze function
    context.analyze(*function);
}

void type_inference::Context::analyze(ast::Node* node) {
	return std::visit([this](auto& variant) {return this->analyze(variant);}, *node);
}

void type_inference::Context::analyze(ast::FunctionNode& node) {
    this->function_node = &node;
    this->call_stack.push_back(&node);

    // Add scope
    this->semantic_context.add_scope();

    // Assign a type variable to every argument and return type
    for (size_t i = 0; i < node.args.size(); i++) {
        if (ast::get_type(node.args[i]) == ast::Type("")) { 
            ast::set_type(node.args[i], (std::to_string(this->current_type_variable_number)));
            this->current_type_variable_number++;
            this->sets.push_back(std::set<std::string>{ast::get_type(node.args[i]).to_str()});
        }

        auto identifier = std::get<ast::IdentifierNode>(*node.args[i]).value;
        this->semantic_context.current_scope()[identifier] = semantic::Binding(node.args[i]);
    }
    node.return_type = ast::Type(std::to_string(this->current_type_variable_number));
    this->current_type_variable_number++;
    this->sets.push_back(std::set<std::string>{node.return_type.to_str()});

    // Analyze function body
    this->analyze(node.body);

    // Assume it's an expression if it isn't a block
    if (node.body->index() != ast::Block) {
        // Unify expression type with return type
        this->sets.push_back(std::set<std::string>{node.return_type.to_str(), ast::get_type(node.body).to_str()});
    }

    // Merge sets that share elements
    this->sets = type_inference::merge_sets_with_shared_elements(this->sets);

     // Check if return type is alone, if is alone it means the function is void
    for (size_t i = 0; i < this->sets.size(); i++) {
        if (this->sets[i].count(node.return_type.to_str())) {
            if (this->sets[i].size() == 1) {
                this->sets[i].insert("void");
            }
        }
    }

    // Label sets
    this->current_type_variable_number = 1;
    std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
    assert(this->labeled_sets.size() <= alphabet.size());
    for (size_t i = 0; i < this->sets.size(); i++) {
        bool representative_found = false;
        for (auto it = this->sets[i].begin(); it != this->sets[i].end(); it++) {
            if (!type_inference::is_number(*it)) {
                assert(!representative_found);
                representative_found = true;
                if (this->labeled_sets.find(*it) != this->labeled_sets.end()) {
                        this->labeled_sets[*it].merge(this->sets[i]);
                }
                else {
                    this->labeled_sets[*it] = this->sets[i];
                }
            }
        }

        if (!representative_found) {
            this->labeled_sets[std::string("$") + alphabet[(this->current_type_variable_number + 18) % alphabet.size()]] = this->sets[i];
            this->current_type_variable_number++;
        }
    }

    // Unify
    this->unify(node.body);

    // Unify args and return type
    for (size_t i = 0; i < node.args.size(); i++) {
        ast::set_type(node.args[i], this->get_unified_type(ast::get_type(node.args[i]).to_str()));
    }
    node.return_type = this->get_unified_type(node.return_type.to_str());

    // Unify constraints
    for (size_t i = 0; i < node.constraints.size(); i++) {
        for (size_t j = 0; j < node.constraints[i].args.size(); j++) {
            node.constraints[i].args[j] = this->get_unified_type(node.constraints[i].args[j].to_str());
        }
        node.constraints[i].return_type = this->get_unified_type(node.constraints[i].return_type.to_str());
    }

    // Remove scope
    this->semantic_context.remove_scope();
}

void type_inference::Context::analyze(ast::BlockNode& block) {
    this->semantic_context.add_scope();

    for (size_t i = 0; i < block.statements.size(); i++) {
        this->analyze(block.statements[i]);
    }

    this->semantic_context.remove_scope();
}

void type_inference::Context::analyze(ast::AssignmentNode& node) {
    this->analyze(node.expression);
    std::string identifier = node.identifier->value;
    this->semantic_context.current_scope()[identifier] = semantic::Binding(&node);
}

void type_inference::Context::analyze(ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        this->analyze(node.expression.value());
        this->sets.push_back(std::set<std::string>{this->function_node->return_type.to_str(), ast::get_type(node.expression.value()).to_str()});
    }
    else {
        this->sets.push_back(std::set<std::string>{this->function_node->return_type.to_str(), "void"});
    }
}

void type_inference::Context::analyze(ast::IfElseNode& node) {
    this->analyze(node.condition);
    this->analyze(node.if_branch);
    if (node.else_branch.has_value()) this->analyze(node.else_branch.value());
}

void type_inference::Context::analyze(ast::WhileNode& node) {
    this->analyze(node.condition);
    this->analyze(node.block);
}

void type_inference::Context::analyze(ast::IdentifierNode& node) {
	if (this->semantic_context.get_binding(node.value)) {
		node.type = semantic::get_binding_type(*this->semantic_context.get_binding(node.value));
	}
}

void type_inference::Context::analyze(ast::IntegerNode& node) {
    node.type = ast::Type(std::to_string(this->current_type_variable_number));
    this->current_type_variable_number++;
}

void type_inference::Context::analyze(ast::FloatNode& node) {
	node.type = ast::Type(std::to_string(this->current_type_variable_number));
    this->current_type_variable_number++;
}

void type_inference::Context::analyze(ast::BooleanNode& node) {
	node.type = ast::Type("bool");
}

void type_inference::Context::analyze(ast::CallNode& node) {
	// Assign a type variable to every argument and return type
	for (size_t i = 0; i < node.args.size(); i++) {
        this->analyze(node.args[i]);
	}
	node.type = ast::Type(std::to_string(this->current_type_variable_number));
	this->current_type_variable_number++;

	// Infer things based on interface if exists
    auto& identifier = node.identifier->value;
	if (interfaces.find(identifier) != interfaces.end()) {
        for (auto& interface: interfaces[identifier]) {
            if (interface.first.size() != node.args.size()) continue;

            // Create prototype type vector
            std::vector<ast::Type> prototype = ast::get_types(node.args);
            prototype.push_back(node.type);

            // Create interface prototype vector
            std::vector<ast::Type> interface_prototype = interface.first;
            interface_prototype.push_back(interface.second);

            // Infer stuff, basically is loop trough the interface prototype that groups type variables
            std::unordered_map<std::string, std::set<std::string>> sets;
            for (size_t i = 0; i < interface_prototype.size(); i++) {
                if (sets.find(interface_prototype[i].to_str()) == sets.end()) {
                    sets[interface_prototype[i].to_str()] = std::set<std::string>{};
                }

                sets[interface_prototype[i].to_str()].insert(prototype[i].to_str());
            
                if (!interface_prototype[i].is_type_variable()) {
                    sets[interface_prototype[i].to_str()].insert(interface_prototype[i].to_str());
                }
            }

            // Save sets found
            for (auto it = sets.begin(); it != sets.end(); it++) {
                this->sets.push_back(it->second);
            }

            // Check binding exists
            semantic::Binding* binding = this->semantic_context.get_binding(identifier);
            if (!binding || !is_function(*binding)) {
                std::cout << "Error: Function " + identifier + " doesn't exists\n";
                this->semantic_context.errors.push_back(Error{"Error: Function doesn't exists"});
                return;
            }
            
            // Add constraint
            auto constraint = ast::FunctionPrototype{identifier, ast::get_types(node.args), node.type};
            if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
                this->function_node->constraints.push_back(constraint);
            }

            return;
        }
	}

    // Check binding exists
    semantic::Binding* binding = this->semantic_context.get_binding(identifier);
    if (!binding || !is_function(*binding)) {
        std::cout << "Error: Function " + identifier + " doesn't exists\n";
        this->semantic_context.errors.push_back(Error{"Error: Function doesn't exists"});
        return;
    }
    
    if (binding->type == semantic::GenericFunctionBinding) {
        ast::FunctionNode* function = semantic::get_generic_function(*binding);
        assert(function->return_type != ast::Type(""));

        // Create prototype type vector
        std::vector<ast::Type> prototype = ast::get_types(node.args);
        prototype.push_back(node.type);

        // Create interface prototype vector
        std::vector<ast::Type> function_prototype = ast::get_types(function->args);
        function_prototype.push_back(function->return_type);

        // Infer stuff, basically is loop trough the interface prototype that groups type variables
        std::unordered_map<std::string, std::set<std::string>> sets;
        for (size_t i = 0; i < function_prototype.size(); i++) {
            if (sets.find(function_prototype[i].to_str()) == sets.end()) {
                sets[function_prototype[i].to_str()] = std::set<std::string>{};
            }

            sets[function_prototype[i].to_str()].insert(prototype[i].to_str());
        
            if (!function_prototype[i].is_type_variable()) {
                sets[function_prototype[i].to_str()].insert(function_prototype[i].to_str());
            }
        }

        // Save sets found
        for (auto it = sets.begin(); it != sets.end(); it++) {
            this->sets.push_back(it->second);
        }
    }

    // Add constraint
    auto constraint = ast::FunctionPrototype{identifier, ast::get_types(node.args), node.type};
    if (std::find(this->function_node->constraints.begin(), this->function_node->constraints.end(), constraint) == this->function_node->constraints.end()) {
        this->function_node->constraints.push_back(constraint);
    }
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

void type_inference::Context::unify(ast::ReturnNode& node) {
    if (node.expression.has_value()) this->unify(node.expression.value());
}

void type_inference::Context::unify(ast::IfElseNode& node) {
    this->unify(node.condition);
    this->unify(node.if_branch);
    if (node.else_branch.has_value()) this->unify(node.else_branch.value());
}

void type_inference::Context::unify(ast::WhileNode& node) {
    this->unify(node.condition);
    this->unify(node.block);
}

void type_inference::Context::unify(ast::IdentifierNode& node) {
    node.type = this->get_unified_type(node.type.to_str());
}

void type_inference::Context::unify(ast::IntegerNode& node) {
    node.type = this->get_unified_type(node.type.to_str());
}

void type_inference::Context::unify(ast::FloatNode& node) {
    node.type = this->get_unified_type(node.type.to_str());
}

void type_inference::Context::unify(ast::CallNode& node) {
	for (size_t i = 0; i < node.args.size(); i++) {
        this->unify(node.args[i]);
	}

    node.type = this->get_unified_type(node.type.to_str());
}