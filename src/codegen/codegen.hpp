#ifndef CODEGEN_CODEGEN_HPP
#define CODEGEN_CODEGEN_HPP

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "lld/Common/Driver.h"

#include "../ast.hpp"
#include "../utilities.hpp"
#include "../semantic.hpp"
#include "../semantic/scopes.hpp"

namespace codegen {
    struct CollectionAsArguments {
        std::vector<llvm::Type*> types;
        std::optional<llvm::StructType*> struct_type = std::nullopt;
    };

    struct Context {
        ast::Ast& ast;
        std::filesystem::path current_module;

        llvm::LLVMContext* context;
        llvm::Module* module;
        llvm::IRBuilder<>* builder;
        llvm::legacy::FunctionPassManager* function_pass_manager;
        llvm::BasicBlock* current_entry_block = nullptr; // Needed for doing stack allocations
        llvm::BasicBlock* last_after_while_block = nullptr; // Needed for break
        llvm::BasicBlock* last_while_block = nullptr; // Needed for continue


        struct Binding {
            ast::Node* node = nullptr;
            llvm::AllocaInst* pointer = nullptr;
            bool is_mutable = false;

            Binding() {}
            Binding(ast::Node* node, llvm::AllocaInst* pointer) : node(node), pointer(pointer) {}
            Binding(ast::Node* node, llvm::AllocaInst* pointer, bool is_mutable) : node(node), pointer(pointer), is_mutable(is_mutable) {}
            ~Binding() {}
        };

        struct Scope {
            std::unordered_map<std::string, Binding>& variables_scope;
            semantic::FunctionsAndTypesScope& functions_and_types_scope;
        };

        struct Scopes {
            std::vector<std::unordered_map<std::string, Binding>> variable_scopes;
            semantic::FunctionsAndTypesScopes functions_and_types_scopes;
        };

        Scopes scopes;
        std::unordered_map<std::string, ast::Type> type_bindings;
        std::unordered_map<std::string, llvm::Constant*> globals;

        // Constructor
        Context(ast::Ast& ast);

        // Scope management
        void add_scope();
        void add_scope(ast::BlockNode& block);
        Scope current_scope();
        void delete_binding(llvm::Value* pointer, ast::Type type);
        void remove_scope();
        Binding get_binding(std::string identifier);

        // Name mangling
        std::string get_mangled_type_name(std::filesystem::path module, std::string identifier);
        std::string get_mangled_function_name(std::filesystem::path module, std::string identifier, std::vector<ast::Type> args, ast::Type return_type, bool is_extern);

        // Types
        llvm::Type* as_llvm_type(ast::Type type);
        llvm::StructType* get_struct_type(ast::TypeNode* type_definition);
        bool has_struct_type(ast::Node* expression);
        bool has_array_type(ast::Node* expression);
        bool has_collection_type(ast::Node* expression);
        bool has_boxed_type(ast::Node* expression);
        CollectionAsArguments get_collection_as_argument(ast::Type type);
        CollectionAsArguments get_struct_type_as_argument(llvm::StructType* struct_type);
        llvm::FunctionType* get_function_type(std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern_and_variadic);
        std::vector<llvm::Type*> as_llvm_types(std::vector<ast::Type> types);
        std::vector<ast::Type> get_types(std::vector<ast::CallArgumentNode*> nodes);
        llvm::TypeSize get_type_size(llvm::Type* type);

        // Codegen helpers
        ast::FunctionNode* get_function(ast::CallNode* call);
        void store_fields(ast::Node* expression, llvm::Value* struct_allocation);
        void store_array_elements(ast::Node* expression, llvm::Value* array_allocation);
        llvm::Value* get_field_pointer(ast::FieldAccessNode& node);
        llvm::Value* get_index_access_pointer(ast::CallNode& node);
        llvm::Constant* get_global_string(std::string str);
        llvm::AllocaInst* create_allocation(std::string name, llvm::Type* type);
        llvm::AllocaInst* copy_expression_to_memory(llvm::Value* pointer, ast::Node* expression);
        llvm::Value* get_pointer_to(ast::Node* expression);
        llvm::Value* create_heap_allocation(ast::Type type);

        // Codegen
        void codegen(ast::Ast& ast);
        llvm::Value* codegen(ast::Node* node);
        llvm::Value* codegen(ast::BlockNode& node);
        llvm::Value* codegen(ast::TypeNode& node) {return nullptr;}
        void codegen_types_prototypes(std::vector<ast::TypeNode*> types);
        void codegen_types_bodies(std::vector<ast::TypeNode*> functions);
        llvm::Value* codegen(ast::FunctionArgumentNode& node) {return nullptr;}
        llvm::Value* codegen(ast::FunctionNode& node) {return nullptr;}
        void codegen_function_prototypes(std::vector<ast::FunctionNode*> functions);
        void codegen_function_prototypes(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern, bool is_extern_and_variadic);
        void codegen_function_bodies(std::vector<ast::FunctionNode*> functions);
        void codegen_function_bodies(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, ast::Node* function_body);
        llvm::Value* codegen(ast::InterfaceNode& node);
        llvm::Value* codegen(ast::DeclarationNode& node);
        llvm::Value* codegen(ast::AssignmentNode& node);
        llvm::Value* codegen(ast::ReturnNode& node);
        llvm::Value* codegen(ast::BreakNode& node);
        llvm::Value* codegen(ast::ContinueNode& node);
        llvm::Value* codegen(ast::IfElseNode& node);
        llvm::Value* codegen(ast::WhileNode& node);
        llvm::Value* codegen(ast::UseNode& node) {return nullptr;}
        llvm::Value* codegen(ast::LinkWithNode& node) {return nullptr;}
        std::vector<llvm::Value*> codegen_args(ast::FunctionNode* function, std::vector<ast::CallArgumentNode*> args);
        llvm::Value* codegen(ast::CallArgumentNode& node) {return nullptr;}
        llvm::Value* codegen_size_function(std::variant<llvm::Value*, llvm::AllocaInst*> pointer, ast::Type type);
        llvm::Value* codegen_print_struct_function(ast::Type arg_type, llvm::Value* arg_pointer);
        llvm::Value* codegen_call(ast::CallNode& node, std::optional<llvm::Value*> allocation);
        llvm::Value* codegen(ast::CallNode& node);
        llvm::Value* codegen(ast::StructLiteralNode& node);
        llvm::Value* codegen(ast::FloatNode& node);
        llvm::Value* codegen(ast::IntegerNode& node);
        llvm::Value* codegen(ast::IdentifierNode& node);
        llvm::Value* codegen(ast::BooleanNode& node);
        llvm::Value* codegen(ast::StringNode& node);
        llvm::Value* codegen(ast::InterpolatedStringNode& node);
        llvm::Value* codegen(ast::ArrayNode& node);
        llvm::Value* codegen(ast::FieldAccessNode& node);
        llvm::Value* codegen(ast::AddressOfNode& node);
        llvm::Value* codegen(ast::DereferenceNode& node);
        llvm::Value* codegen(ast::NewNode& node);
    };
};

#endif