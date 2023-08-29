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
    struct BlockNode;
    struct FunctionNode;
    struct TypeNode;
    struct AssignmentNode;
    struct FieldAssignmentNode;
    struct DereferenceAssignmentNode;
    struct ReturnNode;
    struct BreakNode;
    struct ContinueNode;
    struct IfElseNode;
    struct WhileNode;
    struct UseNode;
    struct LinkWithNode;
    struct CallArgumentNode;
    struct CallNode;
    struct FloatNode;
    struct IntegerNode;
    struct IdentifierNode;
    struct BooleanNode;
    struct StringNode;
    struct FieldAccessNode;
    struct AddressOfNode;
    struct DereferenceNode;
    using FunctionArgumentNode = IdentifierNode;

    enum NodeVariant {
        Block,
        Function,
        TypeDef,
        Assignment,
        FieldAssignment,
        DereferenceAssignment,
        Return,
        Break,
        Continue,
        IfElse,
        While,
        Use,
        LinkWith,
        CallArgument,
        Call,
        Float,
        Integer,
        Identifier,
        Boolean,
        String,
        FieldAccess,
        AddressOf,
        Dereference
    };

    using Node = std::variant<
        BlockNode,
        FunctionNode,
        TypeNode,
        AssignmentNode,
        FieldAssignmentNode,
        DereferenceAssignmentNode,
        ReturnNode,
        BreakNode,
        ContinueNode,
        IfElseNode,
        WhileNode,
        UseNode,
        LinkWithNode,
        CallArgumentNode,
        CallNode,
        FloatNode,
        IntegerNode,
        IdentifierNode,
        BooleanNode,
        StringNode,
        FieldAccessNode,
        AddressOfNode,
        DereferenceNode
    >;

    struct Type;

    struct Interface {
        std::string name;
        
        Interface(std::string name) : name(name) {}
        bool operator==(const Interface &interface) const;
        bool operator!=(const Interface &interface) const;

        ast::Type get_default_type();
    };

    struct Type {
        std::string name;
        std::vector<Type> parameters;
        TypeNode* type_definition = nullptr;
        std::optional<Interface> interface; 

        Type() : name("") {}
        Type(std::string name) : name(name) {}
        Type(std::string name, std::vector<Type> parameters) : name(name), parameters(parameters) {}
        Type(std::string name, TypeNode* type_definition) : name(name), type_definition(type_definition) {}
        bool operator==(const Type &t) const;
        bool operator!=(const Type &t) const;
        std::string to_str(std::string output = "") const;
        bool is_type_variable() const;
        bool is_concrete() const;
        bool is_integer() const;
        bool is_float() const;
        bool is_pointer() const;
    };

    Type get_type(Node* node);
    Type get_concrete_type(Node* node, std::unordered_map<std::string, Type>& type_bindings);
    Type get_concrete_type(Type type_variable, std::unordered_map<std::string, Type>& type_bindings);
    void set_type(Node* node, Type type);
    void set_types(std::vector<CallArgumentNode*> nodes, std::vector<Type> types);
    std::vector<Type> get_types(std::vector<Node*> nodes);
    std::vector<Type> get_types(std::vector<CallArgumentNode*> nodes);
    std::vector<Type> get_types(std::vector<FunctionArgumentNode*> nodes);
    std::vector<Type> get_default_types(std::vector<ast::Type> types);
    std::vector<Type> get_concrete_types(std::vector<Node*> nodes, std::unordered_map<std::string, Type>& type_bindings);
    std::vector<Type> get_concrete_types(std::vector<Type> type_variables, std::unordered_map<std::string, Type>& type_bindings);
    bool is_expression(Node* node);
    bool could_be_expression(Node* node);
    void transform_to_expression(Node*& node);
    bool has_type_variables(std::vector<Type> types);
    bool types_are_concrete(std::vector<Type> types);

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

    struct FunctionConstraint {
        std::string identifier;
        std::vector<Type> args;
        Type return_type;
        ast::CallNode* call = nullptr;

        FunctionConstraint(std::string identifier, std::vector<Type> args, Type return_type, ast::CallNode* call) : identifier(identifier), args(args), return_type(return_type), call(call) {}
        bool operator==(const FunctionConstraint &t) const;
        bool operator!=(const FunctionConstraint &t) const;
    };

    using FunctionConstraints = std::vector<FunctionConstraint>;

    struct FunctionNode {
        size_t line;
        size_t column;
        Type type = Type("");

        IdentifierNode* identifier;
        std::vector<FunctionArgumentNode*> args;
        Node* body;

        bool generic = false;
        bool is_extern = false;
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
        std::filesystem::path module_path; // Used in to tell from which module the type comes from
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

    struct FieldAssignmentNode {
        size_t line;
        size_t column;
        Type type = Type("");

        bool is_mutable = false;
        bool nonlocal = false;

        FieldAccessNode* identifier;
        Node* expression;
    };

    struct DereferenceAssignmentNode {
        size_t line;
        size_t column;
        Type type = Type("");

        DereferenceNode* identifier;
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

    struct LinkWithNode {
        size_t line;
        size_t column;
        Type type = Type("");

        StringNode* directives;
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
        FunctionNode* function = nullptr;
        std::vector<FunctionNode*> functions;
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

        std::vector<ast::IdentifierNode*> fields_accessed;
    };

    struct AddressOfNode {
        size_t line;
        size_t column;
        Type type = Type("");

        ast::Node* expression;
    };

    struct DereferenceNode {
        size_t line;
        size_t column;
        Type type = Type("");
    
        ast::Node* expression;
    };

    struct Ast {
        // High level
        BlockNode* program;
        std::filesystem::path module_path;
        std::unordered_map<std::string, BlockNode*> modules;
        std::vector<std::string> link_with;

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

    Type get_concrete_type_or_type_variable(Type type, PrintContext context);
    void print(const Ast& ast, PrintContext context = PrintContext{});
    void print(Node* node, PrintContext context = PrintContext{});
    void print_with_concrete_types(const Ast& ast, PrintContext context = PrintContext{});
    void print_with_concrete_types(Node* node, PrintContext context = PrintContext{});
};

// Add hash struct for ast::Type to be able to use ast::Type as keys of std::unordered_map
namespace std {
    template <>
    struct hash<ast::Type> {
        std::size_t operator()(const ast::Type& type) const;
    };
};

#endif
