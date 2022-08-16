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
    struct TypeNode;
    struct AssignmentNode;
    struct ReturnNode;
    struct BreakNode;
    struct ContinueNode;
    struct IfElseNode;
    struct WhileNode;
    struct UseNode;
    struct CallArgumentNode;
    struct CallNode;
    struct FloatNode;
    struct IntegerNode;
    struct IdentifierNode;
    struct BooleanNode;
    struct StringNode;
    struct FieldAccessNode;

    enum NodeVariant {
        Block,
        Function,
        TypeDef,
        Assignment,
        Return,
        Break,
        Continue,
        IfElse,
        While,
        Use,
        CallArgument,
        Call,
        Float,
        Integer,
        Identifier,
        Boolean,
        String,
        FieldAccess
    };

    using Node = std::variant<
        BlockNode,
        FunctionNode,
        TypeNode,
        AssignmentNode,
        ReturnNode,
        BreakNode,
        ContinueNode,
        IfElseNode,
        WhileNode,
        UseNode,
        CallArgumentNode,
        CallNode,
        FloatNode,
        IntegerNode,
        IdentifierNode,
        BooleanNode,
        StringNode,
        FieldAccessNode
    >;

    Type get_type(Node* node);
    Type get_concrete_type(Node* node, std::unordered_map<std::string, Type>& type_bindings);
    Type get_concrete_type(Type type_variable, std::unordered_map<std::string, Type>& type_bindings);
    void set_type(Node* node, Type type);
    std::vector<Type> get_types(std::vector<Node*> nodes);
    std::vector<Type> get_types(std::vector<CallArgumentNode*> nodes);
    std::vector<Type> get_concrete_types(std::vector<Node*> nodes, std::unordered_map<std::string, Type>& type_bindings);
    std::vector<Type> get_concrete_types(std::vector<Type> type_variables, std::unordered_map<std::string, Type>& type_bindings);
    bool is_expression(Node* node);
    bool could_be_expression(Node* node);
    void transform_to_expression(Node*& node);

    struct BlockNode {
        size_t line;
        size_t column;
        Type type = Type("");

        std::vector<Node*> statements;
        std::vector<UseNode*> use_statements;
        std::vector<FunctionNode*> functions;
        std::vector<TypeNode*> types;
    };

    struct FunctionSpecialization {
        std::vector<Type> args;
        Type return_type;
        std::unordered_map<std::string, Type> type_bindings;
    };

    struct FunctionPrototype {
        std::string identifier;
        std::vector<Type> args;
        Type return_type;

        FunctionPrototype(std::string identifier, std::vector<Type> args, Type return_type) : identifier(identifier), args(args), return_type(return_type) {}
        bool operator==(const FunctionPrototype &t) const;
        bool operator!=(const FunctionPrototype &t) const;
    };

    using FunctionConstraint = FunctionPrototype;
    using FunctionConstraints = std::vector<FunctionConstraint>;

    struct FunctionNode {
        size_t line;
        size_t column;
        Type type = Type("");

        IdentifierNode* identifier;
        std::vector<Node*> args;
        Node* body;

        bool generic = false;
        Type return_type = Type("");
        std::vector<FunctionSpecialization> specializations;
        FunctionConstraints constraints;
        std::filesystem::path module_path; // Used in to tell from which module the function comes from
    };

    struct TypeNode {
        size_t line;
        size_t column;
        Type type = Type("");

        IdentifierNode* identifier;
        std::vector<IdentifierNode*> fields;
    };

    struct AssignmentNode {
        size_t line;
        size_t column;
        Type type = Type("");

        bool is_mutable = false;
        bool nonlocal = false;

        IdentifierNode* identifier;
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

        StringNode* path;
        bool include = false;
    };

    struct CallArgumentNode {
        size_t line;
        size_t column;
        Type type = Type("");

        std::optional<IdentifierNode*> identifier = std::nullopt;
        Node* expression;
    };

    struct CallNode {
        size_t line;
        size_t column;
        Type type = Type("");

        IdentifierNode* identifier;
        std::vector<CallArgumentNode*> args;
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

    struct FieldAccessNode {
        size_t line;
        size_t column;
        Type type = Type("");

        ast::IdentifierNode* identifier;
        std::vector<ast::IdentifierNode*> fields_accessed;
    };

    struct Ast {
        // High level
        BlockNode* program;
        std::filesystem::path module_path;
        std::unordered_map<std::string, BlockNode*> modules;

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

    struct PrintContext {
        size_t indent_level = 0;
        std::vector<bool> last = {};
        bool concrete = false;
        std::unordered_map<std::string, ast::Type> type_bindings;
    };

    Type get_concrete_type(Type type, PrintContext context);
    void print(const Ast& ast, PrintContext context = PrintContext{});
    void print(Node* node, PrintContext context = PrintContext{});
    void print_with_concrete_types(const Ast& ast, PrintContext context = PrintContext{});
    void print_with_concrete_types(Node* node, PrintContext context = PrintContext{});
};

#endif
