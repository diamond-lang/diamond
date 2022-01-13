#include<set>
#include <unordered_map>
#include <bits/stdc++.h>

#include "type_inference.hpp"
#include "intrinsics.hpp"
#include "errors.hpp"

namespace type_inference {
    struct Binding;
    struct Context;
}

struct type_inference::Binding {
	std::string identifier;
	Type type;
};

struct type_inference::Context {
    std::vector<std::unordered_map<std::string, Binding>> scopes;
	size_t current_type_variable_number = 1;
    std::vector<std::set<std::string>> sets;
    std::unordered_map<std::string, std::set<std::string>> labeled_sets;

	void analyze(std::shared_ptr<Ast::Function> node);
	void analyze(std::shared_ptr<Ast::Expression> node);
	void analyze(std::shared_ptr<Ast::IfElseExpr> node);
	void analyze(std::shared_ptr<Ast::Identifier> node);
	void analyze(std::shared_ptr<Ast::Call> node);

	void unify(std::shared_ptr<Ast::Expression> node);
	void unify(std::shared_ptr<Ast::IfElseExpr> node);
	void unify(std::shared_ptr<Ast::Identifier> node);
    void unify(std::shared_ptr<Ast::Integer> node);
    void unify(std::shared_ptr<Ast::Number> node);
	void unify(std::shared_ptr<Ast::Call> node);

	Binding* get_binding(std::string identifier);
	std::unordered_map<std::string, Binding>& current_scope() {return this->scopes[this->scopes.size() - 1];}
	void add_scope() {this->scopes.push_back(std::unordered_map<std::string, Binding>());}
    Type get_unified_type(std::string type_var);
};

type_inference::Binding* type_inference::Context::get_binding(std::string identifier) {
	for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
		if (scope->find(identifier) != scope->end()) {
			return &(*scope)[identifier];
		}
	}
	return nullptr;
}

std::vector<std::set<std::string>> merge_sets_with_shared_elements(std::vector<std::set<std::string>> sets) {
    size_t i = 0;
    while (i < sets.size()) {
        bool merged = false;
        
        for (size_t j = i + 1; j < sets.size(); j++) {
            std::set<std::string> intersect;
            set_intersection(sets[i].begin(), sets[i].end(), sets[j].begin(), sets[j].end(), std::inserter(intersect, intersect.begin()));
            
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

Type type_inference::Context::get_unified_type(std::string type_var) {
    for (auto it = this->labeled_sets.begin(); it != this->labeled_sets.end(); it++) {
        if (it->second.count(type_var)) {
            return Type(it->first);
        }
    }
    return Type("");
}

bool is_number(std::string str) {
    for (size_t i = 0; i < str.size(); i++) {
        if (!isdigit((int)str[i])) return false;
    }
    return true;
}

// Type Inference
// --------------
void type_inference::analyze(std::shared_ptr<Ast::Function> function) {
    // Create context
    type_inference::Context context;

    // Analyze function
    context.analyze(function);
}

void type_inference::Context::analyze(std::shared_ptr<Ast::Function> node) {
    this->add_scope();

    // Assign a type variable to every argument and return type
    for (size_t i = 0; i < node->args.size(); i++) {
		node->args[i]->type = Type(std::to_string(this->current_type_variable_number));
		this->current_type_variable_number++;

        type_inference::Binding binding;
        binding.identifier = node->args[i]->value;
        binding.type = node->args[i]->type;
        this->current_scope()[binding.identifier] = binding;
	}
    node->return_type = Type(std::to_string(this->current_type_variable_number));
    this->current_type_variable_number++;


	if (std::dynamic_pointer_cast<Ast::Expression>(node->body)) {
		auto expression = std::dynamic_pointer_cast<Ast::Expression>(node->body);

        // Analyze expression
        this->analyze(expression);

        // Unify expression type with return type
        this->sets.push_back(std::set<std::string>{node->return_type.to_str(), expression->type.to_str()});

        // Join sets that share elements
        this->sets = merge_sets_with_shared_elements(this->sets);

        // Label sets
        std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
        assert(this->sets.size() <= alphabet.size());
        size_t j = 19;
        for (size_t i = 0; i < this->sets.size(); i++) {
            bool representative_found = false;
            for (auto it = this->sets[i].begin(); it != this->sets[i].end(); it++) {
                if (!is_number(*it)) {
                    assert(!representative_found);
                    representative_found = true;
                    this->labeled_sets[*it] = this->sets[i];
                }
            }

            if (!representative_found) {
                this->labeled_sets[std::string("$") + alphabet[j % alphabet.size()]] = this->sets[i];
                j++;
            }
        }
        
        this->unify(expression);

        // Unify args and return type
        for (size_t i = 0; i < node->args.size(); i++) {
            node->args[i]->type = this->get_unified_type(node->args[i]->type.to_str());
        }
        node->return_type = this->get_unified_type(node->return_type.to_str());

        node->print();
	}
}

void type_inference::Context::analyze(std::shared_ptr<Ast::Expression> node) {
	if (std::dynamic_pointer_cast<Ast::Identifier>(node)) analyze(std::dynamic_pointer_cast<Ast::Identifier>(node));
	if (std::dynamic_pointer_cast<Ast::IfElseExpr>(node)) analyze(std::dynamic_pointer_cast<Ast::IfElseExpr>(node));
	if (std::dynamic_pointer_cast<Ast::Call>(node))       analyze(std::dynamic_pointer_cast<Ast::Call>(node));
}

void type_inference::Context::analyze(std::shared_ptr<Ast::IfElseExpr> node) {
	this->analyze(node->condition);
	this->analyze(node->expression);
	this->analyze(node->else_expression);

    node->type = Type(std::to_string(this->current_type_variable_number));
    this->current_type_variable_number++;

    this->sets.push_back(std::set<std::string>{node->type.to_str(), node->expression->type.to_str(), node->else_expression->type.to_str()});
}

void type_inference::Context::analyze(std::shared_ptr<Ast::Identifier> node) {
	if (this->get_binding(node->value)) {
		node->type = this->get_binding(node->value)->type;
	}
}

void type_inference::Context::analyze(std::shared_ptr<Ast::Call> node) {
	// Assign a type variable to every argument and return type
	for (size_t i = 0; i < node->args.size(); i++) {
        this->analyze(node->args[i]);
        if (node->args[i]->type == Type("")) {
            node->args[i]->type = Type(std::to_string(this->current_type_variable_number));
            this->current_type_variable_number++;
        }
	}
	node->type = Type(std::to_string(this->current_type_variable_number));
	this->current_type_variable_number++;

	// Infer things based on interface if exists
	if (interfaces.find(node->identifier->value) != interfaces.end()) {
		auto interface = interfaces[node->identifier->value];
		assert(node->args.size() == interface.first.size());

        // Create prototype type vector
		std::vector<Type> prototype = get_args_types(node->args);
		prototype.push_back(node->type);

        // Create interface prototype vector
		std::vector<Type> interface_prototype = interface.first;
		interface_prototype.push_back(interface.second);

        // Infer stuff, basically is loop trough the interface prototype that groups type variables
        std::unordered_map<std::string, std::set<std::string>> sets;
        for (size_t i = 0; i < interface_prototype.size(); i++) {
            if (sets.find(interface_prototype[i].to_str()) == sets.end()) {
                sets[interface_prototype[i].to_str()] = std::set<std::string>{};
            }

            sets[interface_prototype[i].to_str()].insert(prototype[i].to_str());
         
            if (interface_prototype[i].to_str()[0] != '$') {
                sets[interface_prototype[i].to_str()].insert(interface_prototype[i].to_str());
            }
        }

        // Save sets found
        for (auto it = sets.begin(); it != sets.end(); it++) {
            this->sets.push_back(it->second);
        }
	}
}

void type_inference::Context::unify(std::shared_ptr<Ast::Expression> node) {
    if (std::dynamic_pointer_cast<Ast::Integer>(node))    unify(std::dynamic_pointer_cast<Ast::Integer>(node));
    if (std::dynamic_pointer_cast<Ast::Number>(node))     unify(std::dynamic_pointer_cast<Ast::Number>(node));
	if (std::dynamic_pointer_cast<Ast::Identifier>(node)) unify(std::dynamic_pointer_cast<Ast::Identifier>(node));
	if (std::dynamic_pointer_cast<Ast::IfElseExpr>(node)) unify(std::dynamic_pointer_cast<Ast::IfElseExpr>(node));
	if (std::dynamic_pointer_cast<Ast::Call>(node))       unify(std::dynamic_pointer_cast<Ast::Call>(node));
}

void type_inference::Context::unify(std::shared_ptr<Ast::IfElseExpr> node) {
	this->unify(node->condition);
	this->unify(node->expression);
	this->unify(node->else_expression);
    node->type = this->get_unified_type(node->type.to_str());
}

void type_inference::Context::unify(std::shared_ptr<Ast::Identifier> node) {
    node->type = this->get_unified_type(node->type.to_str());
}

void type_inference::Context::unify(std::shared_ptr<Ast::Integer> node) {
    node->type = this->get_unified_type(node->type.to_str());
}

void type_inference::Context::unify(std::shared_ptr<Ast::Number> node) {
    node->type = this->get_unified_type(node->type.to_str());
}

void type_inference::Context::unify(std::shared_ptr<Ast::Call> node) {
	for (size_t i = 0; i < node->args.size(); i++) {
        this->unify(node->args[i]);
	}

    node->type = this->get_unified_type(node->type.to_str());
}