#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <cmath>
#include <cassert>
#include <iostream>
#include "data_structures.hpp"

namespace ast {
    struct BlockNode;
    struct FunctionArgumentNode;
    struct FunctionNode;
    struct TypeNode;
    struct DeclarationNode;
    struct AssignmentNode;
    struct FieldAssignmentNode;
    struct DereferenceAssignmentNode;
    struct IndexAssignmentNode;
    struct ReturnNode;
    struct BreakNode;
    struct ContinueNode;
    struct IfElseNode;
    struct WhileNode;
    struct UseNode;
    struct LinkWithNode;
    struct CallArgumentNode;
    struct CallNode;
    struct StructLiteralNode;
    struct FloatNode;
    struct IntegerNode;
    struct IdentifierNode;
    struct BooleanNode;
    struct StringNode;
    struct ArrayNode;
    struct FieldAccessNode;
    struct AddressOfNode;
    struct DereferenceNode;

    enum NodeVariant {
        Block,
        FunctionArgument,
        Function,
        TypeDef,
        Declaration,
        Assignment,
        FieldAssignment,
        DereferenceAssignment,
        IndexAssignment,
        Return,
        Break,
        Continue,
        IfElse,
        While,
        Use,
        LinkWith,
        CallArgument,
        Call,
        StructLiteral,
        Float,
        Integer,
        Identifier,
        Boolean,
        String,
        Array,
        FieldAccess,
        AddressOf,
        Dereference
    };

    using Node = std::variant<
        BlockNode,
        FunctionArgumentNode,
        FunctionNode,
        TypeNode,
        DeclarationNode,
        AssignmentNode,
        FieldAssignmentNode,
        DereferenceAssignmentNode,
        IndexAssignmentNode,
        ReturnNode,
        BreakNode,
        ContinueNode,
        IfElseNode,
        WhileNode,
        UseNode,
        LinkWithNode,
        CallArgumentNode,
        CallNode,
        StructLiteralNode,
        FloatNode,
        IntegerNode,
        IdentifierNode,
        BooleanNode,
        StringNode,
        ArrayNode,
        FieldAccessNode,
        AddressOfNode,
        DereferenceNode
    >;

    struct Type;
    struct FieldConstraint;
    
    struct FieldTypes {
        std::vector<FieldConstraint> fields;

        std::vector<FieldConstraint>::iterator begin();
        std::vector<FieldConstraint>::iterator end();
        std::size_t size() const;
        std::vector<FieldConstraint>::iterator find(std::string field_name);
        ast::Type& operator[](const std::string& field_name);
        const ast::Type& operator[](const std::string& field_name) const;
        std::string to_str() const;
    };

    struct Interface {
        std::string name;
        
        Interface() {}
        Interface(std::string name) : name(name) {}
        bool operator==(const Interface &interface) const;
        bool operator!=(const Interface &interface) const;

        ast::Type get_default_type();
        bool is_compatible_with(ast::Type type);
    };

    struct NoType {

    };

    struct TypeVariable {
        size_t id;
        bool is_final = false;

        TypeVariable(size_t id) : id(id) {}
        TypeVariable(size_t id, bool is_final) : id(id), is_final(is_final) {}
    };

    struct NominalType {
        std::string name;
        std::vector<Type> parameters;
        TypeNode* type_definition = nullptr;

        NominalType(std::string name) : name(name) {}
        NominalType(std::string name, std::vector<Type> parameters) : name(name), parameters(parameters) {}
        NominalType(std::string name, TypeNode* type_definition) : name(name), type_definition(type_definition) {}
    };

    struct StructType {
        bool open = false;
        FieldTypes fields;

        StructType(FieldTypes fields) : fields(fields) {}
        StructType(FieldTypes fields, bool open) : fields(fields), open(open) {}
    };

    enum TypeVariant {
        NoTypeVariant,
        TypeVariableVariant,
        NominalTypeVariant,
        StructTypeVariant
    };

    struct Type {
        std::variant<NoType, TypeVariable, NominalType, StructType> type;

        Type() : type(NoType{}) {}
        Type(std::string name) : type(NominalType(name)) {}
        Type(std::string name, std::vector<Type> parameters) : type(NominalType(name, parameters)) {}
        Type(std::string name, TypeNode* type_definition) : type(NominalType(name, type_definition)) {}
        Type(std::variant<NoType, TypeVariable, NominalType, StructType> type) : type(type) {}

        ast::NoType& as_no_type();
        ast::TypeVariable& as_type_variable();
        ast::NominalType& as_nominal_type();
        ast::StructType& as_struct_type();

        ast::NoType as_no_type() const;
        ast::TypeVariable as_type_variable() const;
        ast::NominalType as_nominal_type() const;
        ast::StructType as_struct_type() const;

        bool operator==(const Type &t) const;
        bool operator!=(const Type &t) const;
        std::string to_str() const;
        bool is_no_type() const;
        bool is_type_variable() const;
        bool is_nominal_type() const;
        bool is_struct_type() const;
        bool is_structural_struct_type() const;
        bool is_concrete() const;
        bool is_integer() const;
        bool is_float() const;
        bool is_pointer() const;
        bool is_array() const;
        size_t get_array_size() const;
    };

    Type get_type(Node* node);
    Type get_concrete_type(Node* node, std::unordered_map<size_t, Type>& type_bindings);
    Type get_concrete_type(Type type_variable, std::unordered_map<size_t, Type>& type_bindings);
    void set_type(Node* node, Type type);
    std::vector<Type> get_types(std::vector<CallArgumentNode*> nodes);
    std::vector<Type> get_types(std::vector<FunctionArgumentNode*> nodes);  
    std::vector<Type> get_concrete_types(std::vector<Node*> nodes, std::unordered_map<size_t, Type>& type_bindings);
    std::vector<Type> get_concrete_types(std::vector<Type> type_variables, std::unordered_map<size_t, Type>& type_bindings);
    bool is_expression(Node* node);
    bool could_be_expression(Node* node);
    void transform_to_expression(Node*& node);
    bool has_type_variables(std::vector<Type> types);
    bool types_are_concrete(std::vector<Type> types);

    struct FieldConstraint {
        std::string name;
        ast::Type type;
    };

    struct TypeParameter {
        ast::Type type;
        FieldTypes field_constraints;
        std::optional<Interface> interface = std::nullopt;

        std::string to_str();
    };

    struct BlockNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::vector<Node*> statements;
        std::vector<UseNode*> use_statements;
        std::vector<FunctionNode*> functions;
        std::vector<TypeNode*> types;
    };

    struct FunctionArgumentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool is_mutable = false;
        ast::IdentifierNode* identifier;
    };

    struct CallInCallStack {
        std::string identifier;
        std::vector<Type> args;
        Type return_type;
        CallNode* call;
        FunctionNode* function;
        std::filesystem::path file;

        CallInCallStack(std::string identifier, std::vector<Type> args, CallNode* call, FunctionNode* function, std::filesystem::path file) : identifier(identifier), args(args), call(call), function(function), file(file) {}
        bool operator==(const CallInCallStack &t) const;
        bool operator!=(const CallInCallStack &t) const;
    };

    struct FunctionSpecialization {
        std::vector<Type> args;
        Type return_type;
        std::unordered_map<size_t, Type> type_bindings;
    };

    enum FunctionState {
        FunctionCompletelyTyped,
        FunctionAnalyzed,
        FunctionBeingAnalyzed,
        FunctionNotAnalyzed
    };

    struct FunctionNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::vector<ast::TypeParameter> type_parameters;
        std::vector<FunctionArgumentNode*> args;
        Node* body;

        FunctionState state = FunctionCompletelyTyped;
        bool is_extern = false;
        std::vector<FunctionSpecialization> specializations;
        Type return_type = Type(ast::NoType{});
        bool return_type_is_mutable = false;
        std::filesystem::path module_path; // Used in to tell from which module the function comes from
    
        bool typed_parameter_aready_added(ast::Type type);
        std::optional<ast::TypeParameter*> get_type_parameter(ast::Type type);
    };

    struct TypeNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::vector<IdentifierNode*> fields;
        std::filesystem::path module_path; // Used in to tell from which module the type comes from
    };

    struct DeclarationNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool is_mutable = false;
    
        IdentifierNode* identifier;
        Node* expression;
    };

    struct AssignmentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        Node* expression;
    };

    struct FieldAssignmentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool is_mutable = false;
        bool nonlocal = false;

        FieldAccessNode* identifier;
        Node* expression;
    };

    struct DereferenceAssignmentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        DereferenceNode* identifier;
        Node* expression;
    };

    struct IndexAssignmentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        CallNode* index_access;
        Node* expression;
    };

    struct ReturnNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::optional<Node*> expression;
    };

    struct BreakNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});
    };

    struct ContinueNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});
    };

    struct IfElseNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        Node* condition;
        Node* if_branch;
        std::optional<Node*> else_branch;
    };

    struct WhileNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        Node* condition;
        Node* block;
    };

    struct UseNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        StringNode* path;
        bool include = false;
    };

    struct LinkWithNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        StringNode* directives;
    };

    struct CallArgumentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool is_mutable = false;
        std::optional<IdentifierNode*> identifier = std::nullopt;
        Node* expression;
    };

    struct CallNode {
        size_t line;
        size_t end_line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::vector<CallArgumentNode*> args;
        std::vector<FunctionNode*> functions;
    };

    struct StructLiteralNode {
        size_t line;
        size_t end_line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::unordered_map<IdentifierNode*, Node*> fields;
    };

    struct FloatNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        double value;
    };

    struct IntegerNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        int64_t value;
    };

    struct IdentifierNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::string value;
    };

    struct BooleanNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool value;
    };

    struct StringNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::string value;
    };

    struct ArrayNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::vector<Node*> elements;
    };

    struct FieldAccessNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        ast::Node* accessed;
        std::vector<ast::IdentifierNode*> fields_accessed;
    };

    struct AddressOfNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        ast::Node* expression;
    };

    struct DereferenceNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});
    
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
        std::unordered_map<size_t, ast::Type> type_bindings;
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
