#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <optional>

namespace Ast {
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

	struct BlockNode {
		size_t line;
		size_t column;
		Type type = Type("");

		std::vector<size_t> statements;
		std::vector<size_t> functions;
		std::vector<size_t> use_include_statements;
	};

	struct FunctionNode {
		size_t line;
		size_t column;

		size_t identifier;
		std::vector<size_t> args;
		size_t body;

		bool generic = false;
		Type return_type = Type("");
		std::vector<size_t> specializations;
	};

	struct FunctionSpecializationNode {
		size_t line;
		size_t column;

		size_t identifier;
		std::vector<size_t> args;
		size_t body;
		
		bool valid = false;
		Type return_type = Type("");
	};

	struct AssignmentNode {
		size_t line;
		size_t column;

		size_t identifier;
		size_t expression;
		bool is_mutable = false;
		bool nonlocal = false;
	};

	struct ReturnNode {
		size_t line;
		size_t column;

		std::optional<size_t> expression;
	};

	struct BreakNode {
		size_t line;
		size_t column;
	};

	struct ContinueNode {
		size_t line;
		size_t column;
	};

	struct IfElseNode {
		size_t line;
		size_t column;
		Type type = Type("");

		size_t condition;
		size_t if_branch;
		std::optional<size_t> else_branch;
	};

	struct WhileNode {
		size_t line;
		size_t column;

		size_t condition;
		size_t block;
	};

	struct UseNode {
		size_t line;
		size_t column;

		size_t path;
	};

	struct IncludeNode {
		size_t line;
		size_t column;

		size_t path;
	};

	struct CallNode {
		size_t line;
		size_t column;
		Type type = Type("");

		size_t identifier;
		std::vector<size_t> args;
		size_t function; 
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

	enum NodeVariant {
        Block,
        Function,
        FunctionSpecialization,
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
		FunctionSpecializationNode,
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

    struct Ast {
        std::vector<Node> nodes;
        size_t program;
    };

	void print(const Ast& ast, size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
	void print(const Ast& ast, size_t node, size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
	void print_with_concrete_types(const Ast& ast, size_t node, size_t indent_level = 0, std::vector<bool> last = {});
};

#endif