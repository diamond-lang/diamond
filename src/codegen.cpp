#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <assert.h>

#include "semantic.hpp"

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

#include "codegen.hpp"
#include "utilities.hpp"

namespace codegen {
    struct StructType {
        std::vector<llvm::Type*> types;
        llvm::StructType* struct_type;
    };

    struct Context {
        ast::Ast& ast;

        llvm::LLVMContext* context;
        llvm::Module* module;
        llvm::IRBuilder<>* builder;
        llvm::legacy::FunctionPassManager* function_pass_manager;
        llvm::BasicBlock* current_entry_block = nullptr; // Needed for doing stack allocations
        llvm::BasicBlock* last_after_while_block = nullptr; // Needed for break
        llvm::BasicBlock* last_while_block = nullptr; // Needed for continue


        struct Binding {
            llvm::AllocaInst* pointer = nullptr;
            bool is_mutable = false;
        };

        std::vector<std::unordered_map<std::string, Binding>> scopes;
        std::unordered_map<size_t, ast::Type> type_bindings;
        std::unordered_map<std::string, llvm::Constant*> globals;

        Context(ast::Ast& ast) : ast(ast) {
            // Create llvm context
            this->context = new llvm::LLVMContext();
            this->module = new llvm::Module("My cool jit", *(this->context));
            this->builder = new llvm::IRBuilder(*(this->context));

            // Add function pass optimizations
            this->function_pass_manager = new llvm::legacy::FunctionPassManager(this->module);
            this->function_pass_manager->add(llvm::createPromoteMemoryToRegisterPass());
            this->function_pass_manager->add(llvm::createInstructionCombiningPass());
            this->function_pass_manager->add(llvm::createReassociatePass());
            this->function_pass_manager->add(llvm::createGVNPass());
            this->function_pass_manager->add(llvm::createCFGSimplificationPass());
            this->function_pass_manager->doInitialization();
        }

        void codegen(ast::Ast& ast);
        llvm::Value* codegen(ast::Node* node);
        llvm::Value* codegen(ast::BlockNode& node);
        llvm::Value* codegen(ast::TypeNode& node) {return nullptr;}
        void codegen_types_prototypes(std::vector<ast::TypeNode*> types);
        void codegen_types_bodies(std::vector<ast::TypeNode*> functions);
        llvm::Value* codegen(ast::FunctionArgumentNode& node) {return nullptr;}
        llvm::Value* codegen(ast::FunctionNode& node) {return nullptr;}
        void codegen_function_prototypes(std::vector<ast::FunctionNode*> functions);
        void codegen_function_prototypes(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern);
        void codegen_function_bodies(std::vector<ast::FunctionNode*> functions);
        void codegen_function_bodies(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, ast::Node* function_body);
        llvm::Value* codegen(ast::DeclarationNode& node);
        llvm::Value* codegen(ast::AssignmentNode& node);
        llvm::Value* codegen(ast::FieldAssignmentNode& node);
        llvm::Value* codegen(ast::DereferenceAssignmentNode& node);
        llvm::Value* codegen(ast::IndexAssignmentNode& node);
        llvm::Value* codegen(ast::ReturnNode& node);
        llvm::Value* codegen(ast::BreakNode& node);
        llvm::Value* codegen(ast::ContinueNode& node);
        llvm::Value* codegen(ast::IfElseNode& node);
        llvm::Value* codegen(ast::WhileNode& node);
        llvm::Value* codegen(ast::UseNode& node) {return nullptr;}
        llvm::Value* codegen(ast::LinkWithNode& node) {return nullptr;}
        std::vector<llvm::Value*> codegen_args(ast::FunctionNode* function, std::vector<ast::CallArgumentNode*> args);
        llvm::Value* codegen(ast::CallArgumentNode& node) {return nullptr;}
        llvm::Value* codegen(ast::CallNode& node);
        llvm::Value* codegen(ast::StructLiteralNode& node);
        llvm::Value* codegen(ast::FloatNode& node);
        llvm::Value* codegen(ast::IntegerNode& node);
        llvm::Value* codegen(ast::IdentifierNode& node);
        llvm::Value* codegen(ast::BooleanNode& node);
        llvm::Value* codegen(ast::StringNode& node);
        llvm::Value* codegen(ast::ArrayNode& node);
        llvm::Value* codegen(ast::FieldAccessNode& node);
        llvm::Value* codegen(ast::AddressOfNode& node);
        llvm::Value* codegen(ast::DereferenceNode& node);

        std::string get_mangled_type_name(std::filesystem::path module, std::string identifier) {
            return module.string() + "::" + identifier;
        }

        std::string get_mangled_function_name(std::filesystem::path module, std::string identifier, std::vector<ast::Type> args, ast::Type return_type, bool is_extern) {
            if (is_extern) {
                return identifier;
            }

            std::string name = module.string() + "::" + identifier;
            for (size_t i = 0; i < args.size(); i++) {
                name += "_" + args[i].to_str();
            }
            return name + "_" + return_type.to_str();
        }

        llvm::Type* as_llvm_type(ast::Type type) {
            if      (type == ast::Type("float64")) return llvm::Type::getDoubleTy(*(this->context));
            else if (type == ast::Type("int64"))   return llvm::Type::getInt64Ty(*(this->context));
            else if (type == ast::Type("int32"))   return llvm::Type::getInt32Ty(*(this->context));
            else if (type == ast::Type("int8"))    return llvm::Type::getInt8Ty(*(this->context));
            else if (type == ast::Type("bool"))    return llvm::Type::getInt1Ty(*(this->context));
            else if (type == ast::Type("string"))  return llvm::Type::getInt8PtrTy(*(this->context));
            else if (type == ast::Type("void"))    return llvm::Type::getVoidTy(*(this->context));
            else if (type.is_pointer())            return this->as_llvm_type(type.as_nominal_type().parameters[0])->getPointerTo();
            else if (type.is_array())              return llvm::ArrayType::get(this->as_llvm_type(type.as_nominal_type().parameters[0]), type.get_array_size());
            else if (type.is_nominal_type() && type.as_nominal_type().type_definition) {
                return this->get_struct_type(type.as_nominal_type().type_definition);
            }
            else {
                std::cout <<"type: " << type.to_str() << "\n";
                assert(false);
            }
        }

        llvm::StructType* get_struct_type(ast::TypeNode* type_definition) {
            return llvm::StructType::getTypeByName(*this->context, this->get_mangled_type_name(type_definition->module_path, type_definition->identifier->value));
        }

        bool has_struct_type(ast::Node* expression) {
            auto concrete_type = ast::get_concrete_type(expression, this->type_bindings);
            return concrete_type.is_struct_type();
        }

         bool has_array_type(ast::Node* expression) {
            auto concrete_type = ast::get_concrete_type(expression, this->type_bindings);
            return concrete_type.is_array();
        }

        StructType get_struct_type_as_argument(llvm::StructType* struct_type) {
            std::vector<llvm::Type*> sub_types;
            if (this->get_type_size(struct_type) <= 16) {
                auto fields = struct_type->elements();

                size_t i = 0;
                while (i < fields.size()) {
                    ast::Type type = ast::Type(ast::NoType{});
                    size_t bytes = 0;
                    while (bytes < 8) {
                        if (fields[i]->isDoubleTy()) {
                            type = ast::Type("float64");
                            bytes += 8;
                        }
                        else if (fields[i]->isIntegerTy(64)) {
                            type = ast::Type("int64");
                            bytes += 8;
                        }
                        else if (fields[i]->isIntegerTy(32)) {
                            type = ast::Type("int64");
                            bytes += 4;
                        }
                        else if (fields[i]->isIntegerTy(8)) {
                            type = ast::Type("int64");
                            bytes += 4;
                        }
                        else if (fields[i]->isIntegerTy(1)) {
                            type = ast::Type("int64");
                            bytes += 1;
                        }
                        else if (fields[i]->isPointerTy()) {
                            type = ast::Type("int64");
                            bytes += 8;
                        }
                        else {
                            assert(false);
                        }
                        i += 1;
                    }

                    sub_types.push_back(this->as_llvm_type(type));
                }
            }
            else {
                sub_types.push_back(struct_type->getPointerTo());
            }

            StructType result;
            result.types = sub_types;
            result.struct_type = llvm::StructType::get((*this->context), sub_types);
            return result;
        }

        llvm::FunctionType* get_function_type(std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type) {
            // Get args types
            std::vector<llvm::Type*> llvm_args_types;

            if (return_type.is_struct_type() || return_type.is_array()) {
                llvm_args_types.push_back(this->as_llvm_type(return_type)->getPointerTo());
            }

            for (size_t i = 0; i < args.size(); i++) {
                llvm::Type* llvm_type = this->as_llvm_type(args_types[i].type);

                if (args_types[i].is_struct_type() && !args[i]->is_mutable) {
                    auto new_args = this->get_struct_type_as_argument((llvm::StructType*) llvm_type).types;
                    llvm_args_types.insert(llvm_args_types.end(), new_args.begin(), new_args.end());
                }
                else if (args_types[i].is_array()) {
                    llvm_args_types.push_back(llvm_type->getPointerTo());
                }
                else if (args[i]->is_mutable) {
                    llvm_args_types.push_back(llvm_type->getPointerTo());
                }
                else {
                    llvm_args_types.push_back(llvm_type);
                }
            }

            // Get return type
            llvm::Type* llvm_return_type = this->as_llvm_type(return_type);
            if (return_type.is_struct_type() || return_type.is_array()) {
                llvm_return_type = llvm::Type::getVoidTy(*this->context);
            }

            return llvm::FunctionType::get(llvm_return_type, llvm::ArrayRef(llvm_args_types), false);
        }

        std::vector<llvm::Type*> as_llvm_types(std::vector<ast::Type> types) {
            std::vector<llvm::Type*> llvm_types;
            for (size_t i = 0; i < types.size(); i++) {
                llvm_types.push_back(this->as_llvm_type(types[i]));
            }
            return llvm_types;
        }

        std::vector<ast::Type> get_types(std::vector<ast::Node*> nodes) {
            std::vector<ast::Type> types;
            for (auto& node: nodes) {
                types.push_back(ast::get_concrete_type(node, this->type_bindings));
            }
            return types;
        }

        std::vector<ast::Type> get_types(std::vector<ast::CallArgumentNode*> nodes) {
            std::vector<ast::Type> types;
            for (auto& node: nodes) {
                types.push_back(ast::get_concrete_type(node->expression, this->type_bindings));
            }
            return types;
        }

        llvm::AllocaInst* create_allocation(std::string name, llvm::Type* type) {
            assert(this->current_entry_block);
            llvm::IRBuilder<> block(this->current_entry_block, this->current_entry_block->begin());
            return block.CreateAlloca(type, 0, name.c_str());
        }

        ast::FunctionNode* get_function(ast::CallNode* call) {
            ast::FunctionNode* result = nullptr;
            for (auto function: call->functions) {
                if (semantic::are_arguments_compatible(*function, *call, ast::get_types(function->args), this->get_types(call->args))) {
                    result = function;
                    break;
                }
            }
            assert(result != nullptr);
            return result;
        }

        size_t get_index_of_field(ast::TypeNode* definition, std::string field_name) {
            for (size_t i = 0; i < definition->fields.size(); i++) {
                if (definition->fields[i]->value == field_name) {
                    return i;
                }
            }

            assert(false);
            return 0;
        }

        void store_fields(ast::Node* expression, llvm::Value* struct_allocation) {
            // Get struct type
            llvm::StructType* struct_type = this->get_struct_type(ast::get_type(expression).as_nominal_type().type_definition);

            if (expression->index() == ast::StructLiteral) {
                // Get call
                auto& struct_literal = std::get<ast::StructLiteralNode>(*expression);

                // For each field
                for (auto field: struct_literal.fields) {
                    // Get pointer to the field
                    llvm::Value* ptr = this->builder->CreateStructGEP(
                        struct_type,
                        struct_allocation,
                        get_index_of_field(
                            ast::get_type(expression).as_nominal_type().type_definition,
                            field.first->value
                        )
                    );

                    // If the field type has a struct type
                    if (this->has_struct_type(field.second)) {
                        this->store_fields(field.second, ptr);
                    }
                    else {
                        this->builder->CreateStore(this->codegen(field.second), ptr);
                    }
                }
            }
            else if (expression->index() == ast::Call) {
                // Get call
                auto& call = std::get<ast::CallNode>(*expression);

                // Get function
                ast::FunctionNode* function = this->get_function(&call);
                std::string name = this->get_mangled_function_name(function->module_path, call.identifier->value, this->get_types(call.args), ast::get_concrete_type((ast::Node*) &call, this->type_bindings), function->is_extern);
                llvm::Function* llvm_function = this->module->getFunction(name);
                assert(llvm_function);
                
                // Codegen args
                std::vector<llvm::Value*> args = this->codegen_args(function, call.args);
                
                // Add return type as arg (because its a struct)
                args.insert(args.begin(), struct_allocation);

                // Make call
                this->builder->CreateCall(llvm_function, args, "calltmp");
            }
            else if (expression->index() == ast::Identifier) {
                auto& identifier = std::get<ast::IdentifierNode>(*expression);
                this->builder->CreateMemCpy(struct_allocation, llvm::MaybeAlign(), this->get_binding(identifier.value).pointer, llvm::MaybeAlign(), this->get_type_size(struct_type));
            }
            else {
                assert(false);
            }
        }

        void store_array_elements(ast::Node* expression, llvm::Value* array_allocation) {
            llvm::Type* array_type = this->as_llvm_type(ast::get_concrete_type(expression, type_bindings));

            if (expression->index() == ast::Array) {
                auto& array = std::get<ast::ArrayNode>(*expression);

                // Store array elements
                for (size_t i = 0; i < array.elements.size(); i++) {
                    // Get pointer to the element
                    llvm::Value* index = llvm::ConstantInt::get(*(this->context), llvm::APInt(64, i, true));
                    llvm::Value* ptr = this->builder->CreateGEP(array_type, array_allocation, {llvm::ConstantInt::get(*(this->context), llvm::APInt(64, 0, true)), index}, "", true);

                    // Store element
                    this->builder->CreateStore(this->codegen(array.elements[i]), ptr);
                }
            }
            else if (expression->index() == ast::Call) {
                // Get call
                auto& call = std::get<ast::CallNode>(*expression);

                // Get function
                ast::FunctionNode* function = this->get_function(&call);
                std::string name = this->get_mangled_function_name(function->module_path, call.identifier->value, this->get_types(call.args), ast::get_concrete_type((ast::Node*) &call, this->type_bindings), function->is_extern);
                llvm::Function* llvm_function = this->module->getFunction(name);
                assert(llvm_function);
                
                // Codegen args
                std::vector<llvm::Value*> args = this->codegen_args(function, call.args);
                
                // Add return type as arg (because its a struct)
                args.insert(args.begin(), array_allocation);

                // Make call
                this->builder->CreateCall(llvm_function, args, "calltmp");
            }
            else if (expression->index() == ast::Identifier) {
                auto& identifier = std::get<ast::IdentifierNode>(*expression);
                this->builder->CreateMemCpy(array_allocation, llvm::MaybeAlign(), this->get_binding(identifier.value).pointer, llvm::MaybeAlign(), this->get_type_size(array_type));
            }
            else {
                assert(false);
            }
        }

        void add_scope() {
            this->scopes.push_back(std::unordered_map<std::string, Binding>());
        }

        std::unordered_map<std::string, Binding>& current_scope() {
            return this->scopes[this->scopes.size() - 1];
        }

        void remove_scope() {
            this->scopes.pop_back();
        }

        Binding get_binding(std::string identifier) {
            for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
                if (scope->find(identifier) != scope->end()) {
                    return (*scope)[identifier];
                }
            }
            assert(false);
            return Binding{};
        }

        size_t get_index_of_field(std::string field, ast::TypeNode* type_definition) {
            for (size_t i = 0; i < type_definition->fields.size(); i++) {
                if (field == type_definition->fields[i]->value) {
                    return i;
                }
            }
            assert(false);
            return 0;
        }

        llvm::Value* get_field_pointer(ast::FieldAccessNode& node) {
            // There should be at least 2 identifiers in fields accessed. eg: circle.radius
            assert(node.fields_accessed.size() >= 2);

            // Get struct allocation and type
            assert(ast::get_concrete_type((ast::Node*) node.fields_accessed[0], this->type_bindings).as_nominal_type().type_definition);
            llvm::Value* struct_ptr = this->get_binding(node.fields_accessed[0]->value).pointer;
            llvm::StructType* struct_type = this->get_struct_type(ast::get_concrete_type((ast::Node*) node.fields_accessed[0], this->type_bindings).as_nominal_type().type_definition);
            if (((llvm::AllocaInst*)struct_ptr)->getAllocatedType()->isPointerTy()) {
                struct_ptr = this->builder->CreateLoad(struct_type->getPointerTo(), struct_ptr);
            }

            // Get type definition
            ast::TypeNode* type_definition = ast::get_concrete_type((ast::Node*) node.fields_accessed[0], this->type_bindings).as_nominal_type().type_definition;

            for (size_t i = 1; i < node.fields_accessed.size() - 1; i++) {
                // Get pointer to accessed fieldd
                struct_ptr = this->builder->CreateStructGEP(struct_type, struct_ptr, get_index_of_field(node.fields_accessed[i]->value, type_definition));

                // Update current type definition
                type_definition = ast::get_concrete_type((ast::Node*) node.fields_accessed[i], this->type_bindings).as_nominal_type().type_definition;
                struct_type = this->get_struct_type(type_definition);
            }
            
            // Get pointer to accessed fieldd
            size_t last_element = node.fields_accessed.size() - 1;
            llvm::Value* ptr = this->builder->CreateStructGEP(struct_type, struct_ptr, get_index_of_field(node.fields_accessed[last_element]->value, type_definition));
            return ptr;
        }

        llvm::Value* get_index_access_pointer(ast::CallNode& node) {
            llvm::Type* array_type = this->as_llvm_type(ast::get_concrete_type(node.args[0]->expression, this->type_bindings));
            llvm::Value* index = this->builder->CreateSub(this->codegen(node.args[1]->expression), llvm::ConstantInt::get(*(this->context), llvm::APInt(64, 1, true)));
            llvm::Value* array_ptr = this->get_binding(std::get<ast::IdentifierNode>(*node.args[0]->expression).value).pointer;
            if (((llvm::AllocaInst*) array_ptr)->getAllocatedType()->isPointerTy()) {
                array_ptr = this->builder->CreateLoad(array_type->getPointerTo(), array_ptr);
            }
            llvm::Value* ptr = this->builder->CreateGEP(array_type, array_ptr, {llvm::ConstantInt::get(*(this->context), llvm::APInt(64, 0, true)), index}, "", true);
            return ptr;
        }

        llvm::Constant* get_global_string(std::string str) {
            for (auto it = this->globals.begin(); it != this->globals.end(); it++) {
                if (it->first == str) {
                    return it->second;
                }
            }

            this->globals[str] = this->builder->CreateGlobalStringPtr(str);
            return this->globals[str];
        }

        llvm::TypeSize get_type_size(llvm::Type* type) {
            llvm::DataLayout dl = llvm::DataLayout(this->module);
            return dl.getTypeAllocSize(type);
        }
        
    };
};

// Print LLVM IR
// -------------
void codegen::print_llvm_ir(ast::Ast& ast, std::string program_name) {
    codegen::Context llvm_ir(ast);
    llvm_ir.codegen(ast);
    llvm_ir.module->print(llvm::outs(), nullptr);
}


// Print assembly
// --------------
void codegen::print_assembly(ast::Ast& ast, std::string program_name) {
    codegen::Context llvm_ir(ast);
    llvm_ir.codegen(ast);

    // Select target
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    llvm_ir.module->setDataLayout(TargetMachine->createDataLayout());
    llvm_ir.module->setTargetTriple(TargetTriple);

    // Generate assembly
    llvm::legacy::PassManager pass;
    
    auto& llvm_outs = llvm::outs();
    if (TargetMachine->addPassesToEmitFile(pass, llvm_outs, nullptr, llvm::CGFT_AssemblyFile)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*(llvm_ir.module));
    llvm_outs.flush();
}


// Generate object code
// --------------------
static std::string get_object_file_name(std::string executable_name);
static void link(std::string executable_name, std::string object_file_name, std::vector<std::string> link_directives);

void codegen::generate_object_code(ast::Ast& ast, std::string program_name) {
    codegen::Context llvm_ir(ast);
    llvm_ir.codegen(ast);

    // Select target
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        llvm::errs() << Error;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    llvm_ir.module->setDataLayout(TargetMachine->createDataLayout());
    llvm_ir.module->setTargetTriple(TargetTriple);

    // Generate object code
    std::string object_file_name = get_object_file_name(program_name);

    std::error_code EC;
    llvm::raw_fd_ostream dest(object_file_name, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
    }

    pass.run(*(llvm_ir.module));
    dest.flush();
}

// Generate executable
// -------------------
void codegen::generate_executable(ast::Ast& ast, std::string program_name) {
    codegen::generate_object_code(ast, program_name);

    // Link
    link(utilities::get_executable_name(program_name), get_object_file_name(program_name), ast.link_with);

    // Remove generated object file
    remove(get_object_file_name(program_name).c_str());
}

static std::string get_object_file_name(std::string executable_name) {
    return executable_name + ".o";
}

static std::string get_folder_of_executable() {
    return std::filesystem::canonical("/proc/self/exe").parent_path().string();
}

#ifdef __linux__
static void link(std::string executable_name, std::string object_file_name, std::vector<std::string> link_directives) {
    std::string name = "-o" + executable_name;

    // Link using a native C compiler
    if (link_directives.size() > 0) {
        std::string build_command = "cc";
        build_command += " " + object_file_name;
        build_command += " " + name;

        // Add linker directives
        for (auto& directive: link_directives) {
            build_command += " " + directive;
        }

        // Add -no-pie (it doesn't work without it for some reason)
        build_command += " -no-pie";

        // Execute command
        system(build_command.c_str());
    }
    // Link using lld
    else {
         std::vector<std::string> args = {
            "lld",
            object_file_name,
            name,
            get_folder_of_executable() + "/deps/musl/libc.a",
            get_folder_of_executable() + "/deps/musl/crt1.o",
            get_folder_of_executable() + "/deps/musl/crti.o",
            get_folder_of_executable() + "/deps/musl/crtn.o"
        };

        std::string output = "";
        std::string errors = "";
        llvm::raw_string_ostream output_stream(output);
        llvm::raw_string_ostream errors_stream(errors);

        std::vector<const char*> args_as_c_strings;
        for (auto& arg: args) {
            args_as_c_strings.push_back(arg.c_str());
        }
        bool result = lld::elf::link(args_as_c_strings, output_stream, errors_stream, false, false);
        if (result == false) {
            std::cout << errors;
        } 
    }
}
#elif __APPLE__

#include <stdio.h>
#include <sys/sysctl.h>

static void link(std::string executable_name, std::string object_file_name, std::vector<std::string> link_directives) {
    // Link using a native C compiler
    if (link_directives.size() > 0) {
        std::string build_command = "cc";
        build_command += " " + object_file_name;
        build_command += " -o " + executable_name;

        // Add linker directives
        for (auto& directive: link_directives) {
            build_command += " " + directive;
        }

        // Execute command
        system(build_command.c_str());
    }
    
    // Link using lld
    else {
        // Get macos version, modified from https://stackoverflow.com/a/69176800 with https://creativecommons.org/licenses/by-sa/4.0/ license.
        char osversion[32];
        size_t osversion_len = sizeof(osversion) - 1;
        int osversion_name[] = { CTL_KERN, KERN_OSRELEASE };

        if (sysctl(osversion_name, 2, osversion, &osversion_len, NULL, 0) == -1) {
            printf("sysctl() failed\n");
            exit(EXIT_FAILURE);
        }

        uint32_t major, minor;
        if (sscanf(osversion, "%u.%u", &major, &minor) != 2) {
            printf("sscanf() failed\n");
            exit(EXIT_FAILURE);
        }

        if (major >= 20) {
            major -= 9;
        } 
        else {
            major -= 4;
        }

        std::string libclang_rtx_location = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/";
        if (std::filesystem::exists(libclang_rtx_location)) {
            for(auto& entry: std::filesystem::recursive_directory_iterator(libclang_rtx_location)) {
                libclang_rtx_location = entry.path().string();
                break;
            }
            libclang_rtx_location += "/lib/darwin/libclang_rt.osx.a";
        }
        else {
            const char *command = "cc --print-file-name=libclang_rt.osx.a";
            FILE *fpipe =  (FILE*)popen(command, "r");
            if (!fpipe) {
                perror("popen() failed.");
                exit(EXIT_FAILURE);
            }
            
            char c;
            libclang_rtx_location = "";
            while (fread(&c, sizeof c, 1, fpipe)) {
                if (c == '\n') break;
                libclang_rtx_location += c;
            }
        }

        // Create link args
        std::vector<std::string> args = {
            "lld",
            object_file_name,
            "-o",
            executable_name,
            "-arch",
            "arm64",
            "-platform_version",
            "macos",
            std::to_string(major) + ".0.0",
            std::to_string(major) + ".0",
            "-syslibroot",
            "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk",
            "-L/usr/local/lib",
            "-lSystem",
            libclang_rtx_location
        };

        std::string output = "";
        std::string errors = "";
        llvm::raw_string_ostream output_stream(output);
        llvm::raw_string_ostream errors_stream(errors);

        std::vector<const char*> args_as_c_strings;
        for (auto& arg: args) {
            args_as_c_strings.push_back(arg.c_str());
        }
        bool result = lld::macho::link(args_as_c_strings, output_stream, errors_stream, false, false);
        if (result == false) {
            std::cout << errors;
        } 
    }
}
#endif

// Codegeneration
// --------------
void codegen::Context::codegen(ast::Ast& ast) {
    ast::BlockNode* node = (ast::BlockNode*) ast.program;

    // Declare printf
    std::vector<llvm::Type*> args;
    args.push_back(llvm::Type::getInt8PtrTy(*(this->context)));
    llvm::FunctionType *printfType = llvm::FunctionType::get(this->builder->getInt32Ty(), args, true); // `true` specifies the function as variadic
    llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", this->module);

    // Codegen types
    for (auto it = ast.modules.begin(); it != ast.modules.end(); it++) {
        this->codegen_types_prototypes(it->second->types);
    }
    for (auto it = ast.modules.begin(); it != ast.modules.end(); it++) {
        this->codegen_types_bodies(it->second->types);
    }

    // Codegen functions
    for (auto it = ast.modules.begin(); it != ast.modules.end(); it++) {
        this->codegen_function_prototypes(it->second->functions);
    }
    for (auto it = ast.modules.begin(); it != ast.modules.end(); it++) {
        this->codegen_function_bodies(it->second->functions);
    }

    // Crate main function
    llvm::FunctionType* mainType = llvm::FunctionType::get(this->builder->getInt32Ty(), false);
    llvm::Function* main = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", this->module);
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*(this->context), "entry", main);
    this->builder->SetInsertPoint(entry);

    // Set current entry block
    this->current_entry_block = &main->getEntryBlock();

    // Add new scope
    this->add_scope();

    // Codegen statements
    this->codegen((ast::Node*) ast.program);

    // Create return statement
    this->builder->CreateRet(llvm::ConstantInt::get(*(this->context), llvm::APInt(32, 0)));
}

llvm::Value* codegen::Context::codegen(ast::Node* node) {
    return std::visit([this](auto& variant) {return this->codegen(variant);}, *node);
}

llvm::Value* codegen::Context::codegen(ast::BlockNode& node) {
    this->add_scope();

    for (size_t i = 0; i < node.statements.size(); i++) {
        this->codegen(node.statements[i]);
    }

    this->remove_scope();

    return nullptr;
}

void codegen::Context::codegen_types_prototypes(std::vector<ast::TypeNode*> types) {
    for (auto type: types) {
        (void) llvm::StructType::create(*this->context, this->get_mangled_type_name(type->module_path, type->identifier->value));
    }
}

void codegen::Context::codegen_types_bodies(std::vector<ast::TypeNode*> types) {
    for (auto type: types) {
        llvm::StructType* struct_type = this->get_struct_type(type);
        
        std::vector<llvm::Type*> fields;
        for (auto field: type->fields) {
            fields.push_back(as_llvm_type(field->type));
        }

        struct_type->setBody(fields);
    }
}

void codegen::Context::codegen_function_prototypes(std::vector<ast::FunctionNode*> functions) {
    for (auto& function: functions) {
        if (function->state != ast::FunctionCompletelyTyped) {
            for (auto& specialization: function->specializations) {
                this->codegen_function_prototypes(
                    function->module_path,
                    function->identifier->value,
                    function->args,
                    specialization.args,
                    specialization.return_type,
                    function->is_extern
                );
            }
        }
        else {
            this->codegen_function_prototypes(
                function->module_path,
                function->identifier->value,
                function->args,
                ast::get_types(function->args),
                function->return_type,
                function->is_extern
            );
        }
    }
}

void codegen::Context::codegen_function_prototypes(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern) {
    // Make function type
    llvm::FunctionType* function_type = this->get_function_type(args, args_types, return_type);

    // Create function
    std::string name = this->get_mangled_function_name(module_path, identifier, args_types, return_type, is_extern);
    llvm::Function* f = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, name, this->module);

    size_t offset = 0;
    
    if (return_type.is_struct_type() || return_type.is_array()) {
        f->getArg(0)->setName("$result");
        offset += 1;
    }

    // Name arguments
    for (size_t i = 0; i < args.size(); i++) {
        std::string name = args[i]->identifier->value;
        if (args_types[i].is_struct_type() && !args[i]->is_mutable) {
            auto struct_type = (llvm::StructType*) this->as_llvm_type(args_types[i]);
            auto new_args = this->get_struct_type_as_argument(struct_type);

            if (new_args.types.size() > 1) {
                // Store values on struct
                for (size_t j = 0; j < new_args.types.size(); j++) {
                    f->getArg(i + offset + j)->setName(name);
                }
                offset += new_args.types.size() - 1;

                continue;
            }
        }
        else {
            f->getArg(i + offset)->setName(name);
        }
    }
}

void codegen::Context::codegen_function_bodies(std::vector<ast::FunctionNode*> functions) {
    for (auto& function: functions) {
        if (function->is_extern) continue;

        if (function->state != ast::FunctionCompletelyTyped) {
            for (auto& specialization: function->specializations) {
                this->type_bindings = specialization.type_bindings;

                this->codegen_function_bodies(
                    function->module_path,
                    function->identifier->value,
                    function->args,
                    specialization.args,
                    specialization.return_type,
                    function->body
                );

                this->type_bindings = {};
            }
        }
        else {
            this->codegen_function_bodies(
                function->module_path,
                function->identifier->value,
                function->args,
                ast::get_types(function->args),
                function->return_type,
                function->body
            );
        }
    }
}

void codegen::Context::codegen_function_bodies(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args, std::vector<ast::Type> args_types, ast::Type return_type, ast::Node* function_body) {
    std::string name = this->get_mangled_function_name(module_path, identifier, args_types, return_type, false);
    llvm::Function* f = this->module->getFunction(name);

    // Create the body of the function
    llvm::BasicBlock *body = llvm::BasicBlock::Create(*(this->context), "entry", f);
    this->builder->SetInsertPoint(body);

    // Set current entry block
    this->current_entry_block = &f->getEntryBlock();

    // Add arguments to scope
    this->add_scope();

    size_t offset = 0;
    
    if (return_type.is_struct_type() || return_type.is_array()) {
        // Create allocation for argument
        auto allocation = this->create_allocation("$result", f->getArg(0)->getType());

        // Store initial value
        this->builder->CreateStore(f->getArg(0), allocation);

        // Add arguments to scope
        this->current_scope()["$result"] = Binding{allocation};

        offset += 1;
    }

    for (size_t i = 0; i < args.size(); i++) {
        std::string name = args[i]->identifier->value;
        if (args_types[i].is_struct_type() && !args[i]->is_mutable) {
            auto struct_type = (llvm::StructType*) this->as_llvm_type(args_types[i]);
            auto new_args = this->get_struct_type_as_argument(struct_type);

            auto struct_allocation = this->create_allocation(name, struct_type);

            if (new_args.types.size() > 1) {
                // Store values on struct
                for (size_t j = 0; j < new_args.types.size(); j++) {
                    llvm::Value* field_ptr = this->builder->CreateStructGEP(
                        new_args.struct_type,
                        this->builder->CreateBitCast(
                            struct_allocation,
                            new_args.struct_type->getPointerTo()
                        ), 
                        j
                    );
                    this->builder->CreateStore(f->getArg(i + offset + j), field_ptr);
                }
                offset += new_args.types.size() - 1;

                // Add struct to scope
                this->current_scope()[name] = Binding{struct_allocation};
            }
            else {
                // Create allocation for argument
                auto allocation = this->create_allocation(name, f->getArg(i + offset)->getType());

                // Store initial value
                this->builder->CreateStore(f->getArg(i + offset), allocation);

                // Add arguments to scope
                this->current_scope()[name] = Binding{allocation};
            }
        }
        else {
            // Create allocation for argument
            auto allocation = this->create_allocation(name, f->getArg(i + offset)->getType());

            // Store initial value
            this->builder->CreateStore(f->getArg(i + offset), allocation);

            // Add arguments to scope
            this->current_scope()[name] = Binding{allocation, args[i]->is_mutable};
        }
    }

    // Codegen body
    if (ast::is_expression(function_body) && return_type != ast::Type("void")) {
        llvm::Value* result = this->codegen(function_body);
        if (result) {
            this->builder->CreateRet(result);
        }
    }
    else {
        this->codegen(function_body);
        if (return_type == ast::Type("void")) {
            this->builder->CreateRetVoid();
        }
    }

    // Verify function
    llvm::verifyFunction(*f);

    // Run optimizations
    this->function_pass_manager->run(*f);

    // Remove scope
    this->remove_scope();
}

llvm::Value* codegen::Context::codegen(ast::DeclarationNode& node) {
    if (this->has_struct_type(node.expression)) {
        // Create allocation
        llvm::StructType* struct_type = this->get_struct_type(ast::get_type(node.expression).as_nominal_type().type_definition);
        this->current_scope()[node.identifier->value] = Binding{this->create_allocation(node.identifier->value, struct_type)};

        // Store fields
        this->store_fields(node.expression, this->current_scope()[node.identifier->value].pointer);
  
        // Return
        return nullptr;
    }
    else if (this->has_array_type(node.expression)) {
        // Create allocation
        llvm::Type* array_type = this->as_llvm_type(ast::get_concrete_type(node.expression, this->type_bindings));
        this->current_scope()[node.identifier->value] = Binding{this->create_allocation("array", array_type)};
            
        // Store array elements
        this->store_array_elements(node.expression, this->current_scope()[node.identifier->value].pointer);

        // Return
        return nullptr;
    }
    else {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        // Create allocation if doesn't exists or if already exists, but it has a different type
        if (this->current_scope().find(node.identifier->value) == this->current_scope().end()
        ||  this->current_scope()[node.identifier->value].pointer->getType() != expr->getType()) {
            this->current_scope()[node.identifier->value] = Binding{this->create_allocation(node.identifier->value, expr->getType())};
        }

        // Store value
        this->builder->CreateStore(expr, this->current_scope()[node.identifier->value].pointer);

        return nullptr;
    }
}

llvm::Value* codegen::Context::codegen(ast::AssignmentNode& node) {
    // Codegen pointer
    llvm::Value* pointer = this->get_binding(node.identifier->value).pointer;
    if (this->get_binding(node.identifier->value).is_mutable) {
        pointer = this->builder->CreateLoad(
            pointer->getType(),
            pointer
        );
    }

    if (this->has_struct_type(node.expression)) {
        // Store fields
        this->store_fields(node.expression, pointer);
  
        // Return
        return nullptr;
    }
    else if (this->has_array_type(node.expression)) {            
        // Store array elements
        this->store_array_elements(node.expression, pointer);

        // Return
        return nullptr;
    }
    else if (this->get_binding(node.identifier->value).is_mutable) {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        // Codegen pointer
        auto pointer = this->builder->CreateLoad(
            this->get_binding(node.identifier->value).pointer->getType(),
            this->get_binding(node.identifier->value).pointer
        );

        // Create store
        this->builder->CreateStore(expr, pointer);

        // Return
        return nullptr;
    }
    else {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        // Store value
        this->builder->CreateStore(expr, pointer);

        return nullptr;
    }
}

llvm::Value* codegen::Context::codegen(ast::FieldAssignmentNode& node) {
    if (this->has_struct_type(node.expression)) {
        // Get field pointer
        llvm::Value* ptr = this->get_field_pointer(*node.identifier);

        // Store fields
        this->store_fields(node.expression, ptr);
  
        // Return
        return nullptr;
    }
    else if (this->has_array_type(node.expression)) {
        assert(false);
    }
    else {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        // Get field pointer
        llvm::Value* ptr = this->get_field_pointer(*node.identifier);

        // Store value
        this->builder->CreateStore(expr, ptr);

        return nullptr;
    }
}

llvm::Value* codegen::Context::codegen(ast::DereferenceAssignmentNode& node) {
    // Codegen expression
    llvm::Value* expression = this->codegen(node.expression);

    // Codegen pointer
    auto pointer = this->codegen(node.identifier->expression);

    if (this->has_struct_type(node.expression)) {
        // Store fields
        this->store_fields(node.expression, pointer);
    }
    else if (this->has_array_type(node.expression)) {
        assert(false);
    }
    else {
        this->builder->CreateStore(expression, pointer);
    }

    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::IndexAssignmentNode& node) {
    // Codegen expression
    llvm::Value* expression = this->codegen(node.expression);

    // Codegen pointer
    auto pointer = this->codegen(node.index_access->args[0]->expression);

    if (this->has_struct_type(node.expression)) {
        // Store fields
        this->store_fields(node.expression, pointer);
    }
    else if (this->has_array_type(node.expression)) {
        assert(false);
    }
    else {
        this->builder->CreateStore(expression, pointer);
    }

    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        if (this->has_struct_type(node.expression.value())) {
            // Store fields
            this->store_fields(
                node.expression.value(),
                this->builder->CreateLoad(
                    this->as_llvm_type(ast::get_concrete_type(node.expression.value(), this->type_bindings))->getPointerTo(),
                    this->get_binding("$result").pointer
                )
            );
            
            // Return
            this->builder->CreateRetVoid();
        }
        else if (this->has_array_type(node.expression.value())) {
            assert(false);
        }
        else {
            // Generate value of expression
            llvm::Value* expr = this->codegen(node.expression.value());

            // Create return value
            this->builder->CreateRet(expr);
        }
    }
    else {
        this->builder->CreateRetVoid();
    }
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::BreakNode& node) {
    assert(this->last_after_while_block);
    this->builder->CreateBr(this->last_after_while_block);
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::ContinueNode& node) {
    assert(this->last_while_block);
    this->builder->CreateBr(this->last_while_block);
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::IfElseNode& node) {
    if (ast::is_expression((ast::Node*) &node) && ast::get_type((ast::Node*) &node) == ast::Type("void")) {
        llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
        llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
        llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*(this->context), "merge");

        // Jump to if block or else block depending of the condition
        this->builder->CreateCondBr(this->codegen(node.condition), block, else_block);

        // Create if block
        this->builder->SetInsertPoint(block);
        this->codegen(node.if_branch);

        // Jump to merge block
        this->builder->CreateBr(merge_block);

        // Create else block
        current_function->getBasicBlockList().push_back(else_block);
        this->builder->SetInsertPoint(else_block);
        this->codegen(node.else_branch.value());

        // Jump to merge block
        this->builder->CreateBr(merge_block);

        // Create merge block
        current_function->getBasicBlockList().push_back(merge_block);
        this->builder->SetInsertPoint(merge_block);
    }
    else if (ast::is_expression((ast::Node*) &node)) {
        llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
        llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
        llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*(this->context), "merge");

        // Jump to if block or else block depending of the condition
        this->builder->CreateCondBr(this->codegen(node.condition), block, else_block);

        // Create if block
        this->builder->SetInsertPoint(block);
        auto expr = this->codegen(node.if_branch);
        this->builder->CreateBr(merge_block);

        block = this->builder->GetInsertBlock();

        // Create else block
        current_function->getBasicBlockList().push_back(else_block);
        this->builder->SetInsertPoint(else_block);
        auto else_expr = this->codegen(node.else_branch.value());
        this->builder->CreateBr(merge_block);

        else_block = this->builder->GetInsertBlock();

        // Create merge block
        current_function->getBasicBlockList().push_back(merge_block);
        this->builder->SetInsertPoint(merge_block);
        llvm::PHINode* phi_node = this->builder->CreatePHI(expr->getType(), 2, "iftmp");
        phi_node->addIncoming(expr, block);
        phi_node->addIncoming(else_expr, else_block);

        return phi_node;
    }
    else {
        llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
        llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
        llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(*(this->context), "merge");

        // If theres no else block
        if (!node.else_branch.has_value()) {
            // Jump to if block or merge block depending of the condition
            this->builder->CreateCondBr(this->codegen(node.condition), block, merge_block);

            // Create if block
            this->builder->SetInsertPoint(block);
            this->codegen(node.if_branch);

            // Jump to merge block if does not return (NoType) means the if does not return)
            if (ast::get_type(node.if_branch) == ast::Type(ast::NoType{})) {
                this->builder->CreateBr(merge_block);
            }

            // Create merge block
            current_function->getBasicBlockList().push_back(merge_block);
            this->builder->SetInsertPoint(merge_block);
        }
        else {
            // Jump to if block or else block depending of the condition
            this->builder->CreateCondBr(this->codegen(node.condition), block, else_block);

            // Create if block
            this->builder->SetInsertPoint(block);
            this->codegen(node.if_branch);

            // Jump to merge block if if does not return (NoType means the block does not return)
            if (ast::get_type(node.if_branch) == ast::Type(ast::NoType{})) {
                this->builder->CreateBr(merge_block);
            }

            // Create else block
            current_function->getBasicBlockList().push_back(else_block);
            this->builder->SetInsertPoint(else_block);
            this->codegen(node.else_branch.value());

            // Jump to merge block if else does not return (NoType means the block does not return)
            if (ast::get_type(node.else_branch.value()) == ast::Type(ast::NoType{})) {
                this->builder->CreateBr(merge_block);
            }

            // Create merge block
            if (ast::get_type(node.if_branch) == ast::Type(ast::NoType{}) || ast::get_type(node.else_branch.value()) == ast::Type(ast::NoType{})) {
                current_function->getBasicBlockList().push_back(merge_block);
                this->builder->SetInsertPoint(merge_block);
            }
        }
    }

    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::WhileNode& node) {
    llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *loop_block = llvm::BasicBlock::Create(*(this->context), "loop", current_function);
    llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*(this->context), "then");
    this->last_after_while_block = then_block;
    this->last_while_block = loop_block;

    // Jump to loop block if condition is true
    this->builder->CreateCondBr(this->codegen(node.condition), loop_block, then_block);

    // Codegen loop block
    this->builder->SetInsertPoint(loop_block);
    this->codegen(node.block);

    // Jump to loop block again if condition is true
    this->builder->CreateCondBr(this->codegen(node.condition), loop_block, then_block);

    // Insert then block
    current_function->getBasicBlockList().push_back(then_block);
    this->builder->SetInsertPoint(then_block);

    return nullptr;
}

std::vector<llvm::Value*> codegen::Context::codegen_args(ast::FunctionNode* function, std::vector<ast::CallArgumentNode*> args) {
    std::vector<llvm::Value*> result;
    for (size_t i = 0; i < args.size(); i++) {
        if (this->has_struct_type(args[i]->expression) && !args[i]->is_mutable) {
            // Create allocation
            llvm::StructType* struct_type = this->get_struct_type(ast::get_concrete_type(args[i]->expression, this->type_bindings).as_nominal_type().type_definition);
            llvm::AllocaInst* allocation = this->create_allocation(function->args[i]->identifier->value, struct_type);

            // Store fields
            this->store_fields(args[i]->expression, allocation);

            auto new_args = this->get_struct_type_as_argument(struct_type);
            if (new_args.types.size() > 1) {
                for (size_t j = 0; j < new_args.types.size(); j++) {
                    llvm::Value* field_ptr = this->builder->CreateStructGEP(
                        new_args.struct_type,
                        this->builder->CreateBitCast(
                            allocation,
                            new_args.struct_type->getPointerTo()
                        ),
                        j
                    );
                    result.push_back(this->builder->CreateLoad(
                        new_args.types[j],
                        field_ptr
                    ));
                }
            }
            else {
                // Add to args
                result.push_back(allocation);
            }
        }
        else if (this->has_array_type(args[i]->expression) && !args[i]->is_mutable) {
            // Create allocation
            llvm::Type* array_type = this->as_llvm_type(ast::get_concrete_type(args[i]->expression, this->type_bindings));
            llvm::AllocaInst* allocation = this->create_allocation(function->args[i]->identifier->value, array_type);

            // Store array elements
            this->store_array_elements(args[i]->expression, allocation);

            // Add to args
            result.push_back(allocation);
        }
        else if (args[i]->is_mutable) {
            if (args[i]->expression->index() == ast::Identifier) {
                // Add to args
                result.push_back(this->get_binding(((ast::IdentifierNode*) args[i]->expression)->value).pointer);
            }
            else if (args[i]->expression->index() == ast::FieldAccess) {
                // Add to args
                result.push_back(this->get_field_pointer(std::get<ast::FieldAccessNode>(*args[i]->expression)));
            }
            else if (args[i]->expression->index() == ast::Call && std::get<ast::CallNode>(*args[i]->expression).identifier->value == "[]") {
                // Add to args
                result.push_back(this->get_index_access_pointer(std::get<ast::CallNode>(*args[i]->expression)));
            }
            else {
                assert(false);
            }
        }
        else {
            // Add to args
            result.push_back(this->codegen(args[i]->expression));
        }
    }

    return result;
}

llvm::Value* codegen::Context::codegen(ast::CallNode& node) {
    // If type is struct
    if (node.type.is_struct_type()) {
        // Create allocation
        llvm::StructType* struct_type = this->get_struct_type(node.type.as_nominal_type().type_definition);
        llvm::AllocaInst* allocation = this->create_allocation(node.identifier->value, struct_type);
            
        // Store fields
        this->store_fields((ast::Node*) &node, allocation);

        // Return
        return allocation;
    }
    else if (node.type.is_array()) {
        assert(false);
    }

    // Get function
    ast::FunctionNode* function = this->get_function(&node);

    // Codegen args
    std::vector<llvm::Value*> args; 
    if (node.identifier->value != "[]") {
        args = this->codegen_args(function, node.args);
    }
  
    // Intrinsics
    if (node.args.size() == 2) {
        if (node.identifier->value == "+") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFAdd(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateAdd(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "-") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFSub(args[0], args[1], "subtmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateSub(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "*") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFMul(args[0], args[1], "multmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateMul(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "/") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFDiv(args[0], args[1], "divtmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateSDiv(args[0], args[1], "divtmp");
            }
        }
        if (node.identifier->value == "%") {
            return this->builder->CreateSRem(args[0], args[1], "remtmp");
        }
        if (node.identifier->value == "<") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpULT(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateICmpULT(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "<=") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpULE(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateICmpULE(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == ">") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUGT(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateICmpUGT(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == ">=") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUGE(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateICmpUGE(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "==") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUEQ(args[0], args[1], "eqtmp");
            }
            if (args[0]->getType()->isIntegerTy() && args[1]->getType()->isIntegerTy()) {
                return this->builder->CreateICmpEQ(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "and") {
            return this->builder->CreateAnd(args[0], args[1], "andtmp");
        }
        if (node.identifier->value == "or") {
            return this->builder->CreateOr(args[0], args[1], "ortmp");
        }
    }
    if (node.args.size() == 1) {
        if (node.identifier->value == "-") {
            if (args[0]->getType()->isDoubleTy()) {
                return this->builder->CreateFNeg(args[0], "negation");
            }
            if (args[0]->getType()->isIntegerTy()) {
                return this->builder->CreateNeg(args[0], "negation");
            }
        }
        if (node.identifier->value == "not") {
            return this->builder->CreateNot(args[0], "not");
        }
    }
    if (node.identifier->value == "print") {
        if (args[0]->getType()->isIntegerTy(1)) {
            std::vector<llvm::Value*> printArgs;
            llvm::Function *current_function = this->builder->GetInsertBlock()->getParent();
            llvm::BasicBlock *then_block = llvm::BasicBlock::Create(*(this->context), "then", current_function);
            llvm::BasicBlock *else_block = llvm::BasicBlock::Create(*(this->context), "else");
            llvm::BasicBlock *merge = llvm::BasicBlock::Create(*(this->context), "ifcont");
            this->builder->CreateCondBr(args[0], then_block, else_block);

            // Create then branch
            this->builder->SetInsertPoint(then_block);
            printArgs.push_back(this->get_global_string("true\n"));
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            this->builder->CreateBr(merge);
            then_block = this->builder->GetInsertBlock();

            printArgs.clear();

            // Create else branch
            current_function->getBasicBlockList().push_back(else_block);
            this->builder->SetInsertPoint(else_block);
            printArgs.push_back(this->get_global_string("false\n"));
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            this->builder->CreateBr(merge);
            else_block = this->builder->GetInsertBlock();

            // Merge  block
            current_function->getBasicBlockList().push_back(merge);
            this->builder->SetInsertPoint(merge);
            return nullptr;
        }
        if (args[0]->getType()->isDoubleTy()) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(this->get_global_string("%g\n"));
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
        if (args[0]->getType()->isIntegerTy()) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(this->get_global_string("%d\n"));
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
        if (ast::get_concrete_type(node.args[0]->expression, this->type_bindings) == ast::Type("string")) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
    }
    if (node.identifier->value == "[]") {
        if (node.args.size() == 2) {
            return this->builder->CreateLoad(
                this->as_llvm_type(ast::get_concrete_type((ast::Node*)&node, this->type_bindings)),
                this->get_index_access_pointer(node)
            );
        }
    }

    // Get function
    std::string name = this->get_mangled_function_name(function->module_path, node.identifier->value, this->get_types(node.args), ast::get_concrete_type((ast::Node*) &node, this->type_bindings), function->is_extern);
    llvm::Function* llvm_function = this->module->getFunction(name);
    assert(llvm_function);

    // Make call
    return this->builder->CreateCall(llvm_function, args, "calltmp");
}

llvm::Value* codegen::Context::codegen(ast::StructLiteralNode& node) {
    // Create allocation
    llvm::StructType* struct_type = this->get_struct_type(node.type.as_nominal_type().type_definition);
    llvm::AllocaInst* allocation = this->create_allocation(node.identifier->value, struct_type);
        
    // Store fields
    this->store_fields((ast::Node*) &node, allocation);

    // Return
    return allocation;
}

llvm::Value* codegen::Context::codegen(ast::FloatNode& node) {
    return llvm::ConstantFP::get(*(this->context), llvm::APFloat(node.value));
}

llvm::Value* codegen::Context::codegen(ast::IntegerNode& node) {
    if (ast::get_concrete_type((ast::Node*) &node, this->type_bindings) == ast::Type("float64"))
        return llvm::ConstantFP::get(*(this->context), llvm::APFloat((double)node.value));
    else if (ast::get_concrete_type((ast::Node*) &node, this->type_bindings) == ast::Type("int64")) {
        return llvm::ConstantInt::get(*(this->context), llvm::APInt(64, node.value, true));
    }
    else if (ast::get_concrete_type((ast::Node*) &node, this->type_bindings) == ast::Type("int32")) {
        return llvm::ConstantInt::get(*(this->context), llvm::APInt(32, node.value, true));
    }
    else if (ast::get_concrete_type((ast::Node*) &node, this->type_bindings) == ast::Type("int8")) {
        return llvm::ConstantInt::get(*(this->context), llvm::APInt(8, node.value, true));
    }
    
    assert(false);
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::IdentifierNode& node) {
    if (this->has_struct_type((ast::Node*) &node) && !this->get_binding(node.value).is_mutable) {
        return this->get_binding(node.value).pointer;
    }
    else if (this->has_array_type((ast::Node*) &node) && !this->get_binding(node.value).is_mutable) {
        return this->get_binding(node.value).pointer;
    }
    else {
        llvm::Value* pointer = this->get_binding(node.value).pointer;

        if (this->get_binding(node.value).is_mutable) {
            pointer = this->builder->CreateLoad(
                pointer->getType(),
                pointer
            );
        }

        return this->builder->CreateLoad(
            this->as_llvm_type(ast::get_concrete_type((ast::Node*) &node, this->type_bindings)),
            pointer,
            node.value.c_str()
        );
    }
}

llvm::Value* codegen::Context::codegen(ast::BooleanNode& node) {
    return llvm::ConstantInt::getBool(*(this->context), node.value);
}

llvm::Value* codegen::Context::codegen(ast::StringNode& node) {
    return this->get_global_string(node.value);
}

llvm::Value* codegen::Context::codegen(ast::ArrayNode& node) {
    assert(false);
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::FieldAccessNode& node) {
    return this->builder->CreateLoad(
        this->as_llvm_type(ast::get_concrete_type((ast::Node*) &node, this->type_bindings)),
        this->get_field_pointer(node),
        node.fields_accessed[node.fields_accessed.size() - 1]->value
    );
}

llvm::Value* codegen::Context::codegen(ast::AddressOfNode& node) {
    return this->get_binding(((ast::IdentifierNode*) node.expression)->value).pointer;
}


llvm::Value* codegen::Context::codegen(ast::DereferenceNode& node) {
    auto pointer = this->codegen(node.expression);
    return this->builder->CreateLoad(this->as_llvm_type(ast::get_concrete_type((ast::Node*) &node, this->type_bindings)), pointer);;
}