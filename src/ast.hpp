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
    struct InterfaceNode;
    struct TypeNode;
    struct DeclarationNode;
    struct AssignmentNode;
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
    struct InterpolatedStringNode;
    struct ArrayNode;
    struct FieldAccessNode;
    struct AddressOfNode;
    struct DereferenceNode;
    struct NewNode;

    enum NodeVariant {
        Block,
        FunctionArgument,
        Function,
        Interface,
        TypeDef,
        Declaration,
        Assignment,
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
        InterpolatedString,
        Array,
        FieldAccess,
        AddressOf,
        Dereference,
        New
    };

    using Node = std::variant<
        BlockNode,
        FunctionArgumentNode,
        FunctionNode,
        InterfaceNode,
        TypeNode,
        DeclarationNode,
        AssignmentNode,
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
        InterpolatedStringNode,
        ArrayNode,
        FieldAccessNode,
        AddressOfNode,
        DereferenceNode,
        NewNode
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

    struct InterfaceType {
        std::string name;
        
        InterfaceType() {}
        InterfaceType(std::string name) : name(name) {}
        bool operator==(const InterfaceType &interface) const;
        bool operator!=(const InterfaceType &interface) const;

        ast::Type get_default_type();
        bool is_compatible_with(ast::Type type, ast::InterfaceNode* interface);
    };

    ast::Type get_default_type(Set<ast::InterfaceType> interface);

    struct NoType {

    };

    struct TypeVariable {
        size_t id;

        TypeVariable(size_t id) : id(id) {}
    };

    struct FinalTypeVariable {
        std::string id;
        std::vector<ast::FieldConstraint> field_constraints;
        std::vector<ast::Type> parameter_constraints;

        FinalTypeVariable(std::string id) : id(id) {}
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
        FinalTypeVariableVariant,
        NominalTypeVariant,
        StructTypeVariant
    };

    struct Type {
        std::variant<NoType, TypeVariable, FinalTypeVariable, NominalType, StructType> type;

        Type() : type(NoType{}) {}
        Type(std::string name) : type(NominalType(name)) {}
        Type(std::string name, std::vector<Type> parameters) : type(NominalType(name, parameters)) {}
        Type(std::string name, TypeNode* type_definition) : type(NominalType(name, type_definition)) {}
        Type(std::variant<NoType, TypeVariable, FinalTypeVariable, NominalType, StructType> type) : type(type) {}

        ast::NoType& as_no_type();
        ast::TypeVariable& as_type_variable();
        ast::FinalTypeVariable& as_final_type_variable();
        ast::NominalType& as_nominal_type();
        ast::StructType& as_struct_type();

        ast::NoType as_no_type() const;
        ast::TypeVariable as_type_variable() const;
        ast::FinalTypeVariable as_final_type_variable() const;
        ast::NominalType as_nominal_type() const;
        ast::StructType as_struct_type() const;

        bool operator==(const Type &t) const;
        bool operator!=(const Type &t) const;
        std::string to_str() const;
        bool is_no_type() const;
        bool is_type_variable() const;
        bool is_final_type_variable() const;
        bool is_nominal_type() const;
        bool is_struct_type() const;
        bool is_structural_struct_type() const;
        bool is_concrete() const;
        bool is_integer() const;
        bool is_float() const;
        bool is_pointer() const;
        bool is_boxed() const;
        bool has_boxed_elements() const;
        bool is_array() const;
        bool is_builtin_type() const;
        bool is_collection() const;
        size_t array_size_known() const;
        size_t get_array_size() const;
    };

    Type get_type(Node* node);
    Type get_concrete_type(Node* node, std::unordered_map<std::string, Type>& type_bindings);
    Type get_concrete_type(Type type_variable, std::unordered_map<std::string, Type>& type_bindings);
    Type try_to_get_concrete_type(Type type_variable, std::unordered_map<std::string, Type>& type_bindings);
    void set_type(Node* node, Type type);
    std::vector<Type> get_types(std::vector<CallArgumentNode*> nodes);
    std::vector<Type> get_types(std::vector<FunctionArgumentNode*> nodes);  
    std::vector<Type> get_concrete_types(std::vector<Node*> nodes, std::unordered_map<std::string, Type>& type_bindings);
    std::vector<Type> get_concrete_types(std::vector<Type> type_variables, std::unordered_map<std::string, Type>& type_bindings);
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
        Set<InterfaceType> interface;

        std::string to_str();
    };

    struct BlockNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::vector<Node*> statements;
        std::vector<UseNode*> use_statements;
        std::vector<FunctionNode*> functions;
        std::vector<InterfaceNode*> interfaces;
        std::vector<TypeNode*> types;
    };

    struct FunctionArgumentNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        bool is_mutable = false;
        ast::IdentifierNode* identifier;
    };

    struct FunctionSpecialization {
        std::vector<Type> args;
        Type return_type;
        std::unordered_map<std::string, Type> type_bindings;
    };

    enum FunctionState {
        FunctionCompletelyTyped,
        FunctionGenericCompletelyTyped,
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
        bool is_extern_and_variadic = false;
        bool is_builtin = false;
        std::vector<FunctionSpecialization> specializations;
        Type return_type = Type(ast::NoType{});
        bool return_type_is_mutable = false;
        std::filesystem::path module_path; // Used in to tell from which module the function comes from
        bool is_used = false;

        bool typed_parameter_aready_added(ast::Type type);
        std::optional<ast::TypeParameter*> get_type_parameter(ast::Type type);
        bool is_in_type_parameter(ast::Type type);
    };

    struct InterfaceNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::vector<ast::TypeParameter> type_parameters;
        std::vector<FunctionArgumentNode*> args;

        Type return_type = Type(ast::NoType{});
        bool return_type_is_mutable = false;
        std::filesystem::path module_path; // Used in to tell from which module the function comes from
        std::vector<ast::FunctionNode*> functions;
    
        bool typed_parameter_aready_added(ast::Type type);
        std::optional<ast::TypeParameter*> get_type_parameter(ast::Type type);
        std::vector<ast::Type> get_prototype();
        bool is_in_type_parameter(ast::Type type);

        bool is_compatible_with(ast::Type type);
    };

    std::optional<ast::TypeParameter*> get_type_parameter(std::vector<ast::TypeParameter>& type_parameters, ast::Type type);

    struct TypeNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        IdentifierNode* identifier;
        std::vector<IdentifierNode*> fields;
        std::vector<TypeNode*> cases;
        std::filesystem::path module_path; // Used in to tell from which module the type comes from

        size_t get_index_of_field(std::string field_name);
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

        Node* assignable;
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

        std::vector<bool> get_args_mutability() {
            std::vector<bool> result;
            for (auto arg: this->args) {
                result.push_back(arg->is_mutable);
            }
            return result;
        }
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

    struct InterpolatedStringNode {
        size_t line;
        size_t column;
        Type type = Type(ast::NoType{});

        std::vector<std::string> strings;
        std::vector<ast::Node*> expressions;
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

    struct NewNode {
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
