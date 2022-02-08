#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>

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

namespace Ast {
	struct Node {
		size_t line;
		size_t col;
		std::string file;

		Node(size_t line, size_t col, std::string file): line(line), col(col), file(file) {}
		virtual ~Node() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false) = 0;
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct Identifier;
	struct Function;
	struct Use;
	struct Program;

	struct Program : Node {
		std::vector<std::shared_ptr<Ast::Node>> statements;
		std::vector<std::shared_ptr<Ast::Function>> functions;
		std::vector<std::shared_ptr<Ast::Use>> use_statements;
		std::unordered_map<std::string, std::shared_ptr<Ast::Program>> modules;

		Program(std::vector<std::shared_ptr<Ast::Node>> statements, std::vector<std::shared_ptr<Ast::Function>> functions, std::vector<std::shared_ptr<Ast::Use>> use_statements, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements), functions(functions), use_statements(use_statements) {}
		virtual ~Program() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
		void print_with_concrete_types();
	};

	struct Block : Node {
		std::vector<std::shared_ptr<Ast::Node>> statements;
		std::vector<std::shared_ptr<Ast::Function>> functions;
		std::vector<std::shared_ptr<Ast::Use>> use_statements;
		Type type = Type("");

		Block(std::vector<std::shared_ptr<Ast::Node>> statements, std::vector<std::shared_ptr<Ast::Function>> functions, std::vector<std::shared_ptr<Ast::Use>> use_statements, size_t line, size_t col, std::string file) : Node(line, col, file), statements(statements), functions(functions), use_statements(use_statements) {}
		virtual ~Block() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization;

	struct Function : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Identifier>> args;
		std::shared_ptr<Ast::Node> body;

		bool generic = false;
		Type return_type = Type("");
		std::vector<std::shared_ptr<Ast::FunctionSpecialization>> specializations;

		Function(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Identifier>> args, std::shared_ptr<Ast::Node> body, size_t line, size_t col, std::string file) :  Node(line, col, file), identifier(identifier), args(args), body(body) {}
		virtual ~Function() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct FunctionSpecialization : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Identifier>> args;
		std::shared_ptr<Ast::Node> body;
		
		bool valid = false;
		Type return_type = Type("");

		FunctionSpecialization(size_t line, size_t col, std::string file) :  Node(line, col, file) {}
		virtual ~FunctionSpecialization() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Expression;

	struct Assignment : Node {
		std::shared_ptr<Ast::Identifier> identifier;
		std::shared_ptr<Ast::Expression> expression;
		bool is_mutable = false;
		bool nonlocal = false;

		Assignment(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Expression> expression, bool is_mutable, bool nonlocal, size_t line, size_t col, std::string file) : Node(line, col, file), identifier(identifier), expression(expression), is_mutable(is_mutable), nonlocal(nonlocal) {}
		virtual ~Assignment() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Return : Node {
		std::shared_ptr<Ast::Expression> expression;

		Return(std::shared_ptr<Ast::Expression> expression, size_t line, size_t col, std::string file) : Node(line, col, file), expression(expression) {}
		virtual ~Return() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Break : Node {
		Break(size_t line, size_t col, std::string file) : Node(line, col, file) {}
		virtual ~Break() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Continue : Node {
		Continue(size_t line, size_t col, std::string file) : Node(line, col, file) {}
		virtual ~Continue() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct IfElseStmt : Node {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Block> block;
		std::shared_ptr<Ast::Block> else_block;

		IfElseStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block), else_block(nullptr) {}
		IfElseStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, std::shared_ptr<Ast::Block> else_block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block), else_block(else_block) {}
		virtual ~IfElseStmt() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct WhileStmt : Node {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Block> block;

		WhileStmt(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Block> block, size_t line, size_t col, std::string file) : Node(line, col, file), condition(condition), block(block) {}
		virtual ~WhileStmt() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct String;

	struct Use : Node {
		std::shared_ptr<Ast::String> path;
		bool include = false;

		Use(std::shared_ptr<Ast::String> path, size_t line, size_t col, std::string file) : Node(line, col, file), path(path) {}
		virtual ~Use() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Expression : Node {
		Type type = Type();

		Expression(size_t line, size_t col, std::string file) : Node(line, col, file) {}
		virtual ~Expression() {}
		virtual std::shared_ptr<Node> clone() = 0;
	};

	struct IfElseExpr : Expression {
		std::shared_ptr<Ast::Expression> condition;
		std::shared_ptr<Ast::Expression> expression;
		std::shared_ptr<Ast::Expression> else_expression;
		
		IfElseExpr(std::shared_ptr<Ast::Expression> condition, std::shared_ptr<Ast::Expression> expression, std::shared_ptr<Ast::Expression> else_expression, size_t line, size_t col, std::string file) : Expression(line, col, file), condition(condition), expression(expression), else_expression(else_expression) {}
		virtual ~IfElseExpr() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Call : Expression {
		std::shared_ptr<Ast::Identifier> identifier;
		std::vector<std::shared_ptr<Ast::Expression>> args;
		std::shared_ptr<Ast::Node> function = nullptr;  

		Call(std::shared_ptr<Ast::Identifier> identifier, std::vector<std::shared_ptr<Ast::Expression>> args, size_t line, size_t col, std::string file) : Expression(line, col, file), identifier(identifier), args(args) {}
		virtual ~Call() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Number : Expression {
		double value;

		Number(double value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Number() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Integer : Expression {
		int64_t value;

		Integer(int64_t value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Integer() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Identifier : Expression {
		std::string value;

		Identifier(std::string value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Identifier() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct Boolean : Expression {
		bool value;

		Boolean(bool value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~Boolean() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};

	struct String : Expression {
		std::string value;

		String(std::string value, size_t line, size_t col, std::string file) : Expression(line, col, file), value(value) {}
		virtual ~String() {}
		virtual void print(size_t indent_level = 0, std::vector<bool> last = {}, bool concrete = false);
		virtual std::shared_ptr<Node> clone();
	};
}

std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Identifier>> args);
std::vector<Type> get_args_types(std::vector<std::shared_ptr<Ast::Expression>> args);

#endif