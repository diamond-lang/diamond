#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <cmath>

namespace ast {
	struct Type {
		std::string name;
		std::vector<Type> parameters;

		Type() : name("") {}
		Type(std::string name) : name(name) {}
		Type(std::string name, std::vector<Type> parameters) : name(name), parameters(parameters) {}
		bool operator==(const Type &t) const;
		bool operator!=(const Type &t) const;
		std::string to_str(std::string output = "") const;
		bool is_type_variable() const;
	};

	struct BlockNode;
	struct FunctionNode;
	struct AssignmentNode;
	struct ReturnNode;
	struct BreakNode;
	struct ContinueNode;
	struct IfElseNode;
	struct WhileNode;
	struct UseNode;
	struct IncludeNode;
	struct CallNode;
	struct FloatNode;
	struct IntegerNode;
	struct IdentifierNode;
	struct BooleanNode;
	struct StringNode;

	enum NodeVariant {
        Block,
        Function,
        Assignment,
        Return,
        Break,
        Continue,
        IfElse,
        While,
        Use,
		Include,
        Call,
        Float,
        Integer,
        Identifier,
        Boolean,
        String
    };

	using Node = std::variant<
		BlockNode,
		FunctionNode,
		AssignmentNode,
		ReturnNode,
		BreakNode,
		ContinueNode,
		IfElseNode,
		WhileNode,
		UseNode,
		IncludeNode,
		CallNode,
		FloatNode,
		IntegerNode,
		IdentifierNode,
		BooleanNode,
		StringNode
	>;

	Type get_type(Node* node);
	void set_type(Node* node, Type type);
	std::vector<Type> get_types(std::vector<Node*> nodes);

	struct BlockNode {
		size_t line;
		size_t column;
		Type type = Type("");

		std::vector<Node*> statements;
		std::vector<Node*> functions;
		std::vector<Node*> use_include_statements;
	};

	struct FunctionSpecialization {
		std::vector<Type> args;
		Type return_type;
		bool valid = false;
	};

	struct FunctionNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* identifier;
		std::vector<Node*> args;
		Node* body;

		bool generic = false;
		Type return_type = Type("");
		std::vector<FunctionSpecialization> specializations;
	};

	struct AssignmentNode {
		size_t line;
		size_t column;
		Type type = Type("");

		bool is_mutable = false;
		bool nonlocal = false;

		Node* identifier;
		Node* expression;
	};

	struct ReturnNode {
		size_t line;
		size_t column;
		Type type = Type("");

		std::optional<Node*> expression;
	};

	struct BreakNode {
		size_t line;
		size_t column;
		Type type = Type("");
	};

	struct ContinueNode {
		size_t line;
		size_t column;
		Type type = Type("");
	};

	struct IfElseNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* condition;
		Node* if_branch;
		std::optional<Node*> else_branch;
	};

	struct WhileNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* condition;
		Node* block;
	};

	struct UseNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* path;
	};

	struct IncludeNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* path;
	};

	struct CallNode {
		size_t line;
		size_t column;
		Type type = Type("");

		Node* identifier;
		std::vector<Node*> args;
		FunctionNode* function; 
	};

	struct FloatNode {
		size_t line;
		size_t column;
		Type type = Type("");

		double value;
	};

	struct IntegerNode {
		size_t line;
		size_t column;
		Type type = Type("");

		int64_t value;
	};

	struct IdentifierNode {
		size_t line;
		size_t column;
		Type type = Type("");

		std::string value;
	};

	struct BooleanNode {
		size_t line;
		size_t column;
		Type type = Type("");

		bool value;
	};

	struct StringNode {
		size_t line;
		size_t column;
		Type type = Type("");

		std::string value;
	};

    struct Ast {
		// High level
		Node* program;
		std::filesystem::path file;
		std::unordered_map<std::string, Node*> modules; 
        
		// Storage
		std::vector<Node*> nodes;
		unsigned int growth_factor = 2;
		unsigned int initial_size = 20;
		size_t size = 0;

		// Methods
		size_t capacity();
		void push_back(Node node);
		size_t size_of_arrays_filled();
		Node* last_element();
		void free();
    };

	void print(const Ast& ast, size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
	void print(Node* node, size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
	void print_with_concrete_types(const Ast& ast, size_t indent_level = 0, std::vector<bool> last = {});
	void print_with_concrete_types(Node* node, size_t indent_level = 0, std::vector<bool> last = {});
};

#endif