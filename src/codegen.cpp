#include <iostream>
#include <cstdio>
#include <unordered_map>
#include <assert.h>

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
#include "llvm/Support/TargetRegistry.h"
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
    struct Context {
        ast::Ast& ast;

        llvm::LLVMContext* context;
        llvm::Module* module;
        llvm::IRBuilder<>* builder;
        llvm::legacy::FunctionPassManager* function_pass_manager;
        llvm::BasicBlock* current_entry_block = nullptr; // Needed for doing stack allocations
        llvm::BasicBlock* last_after_while_block = nullptr; // Needed for break
        llvm::BasicBlock* last_while_block = nullptr; // Needed for continue

        std::vector<std::unordered_map<std::string, llvm::AllocaInst*>> scopes;
        std::unordered_map<std::string, ast::Type> type_bindings;
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
        llvm::Value* codegen(ast::FunctionNode& node) {return nullptr;}
        void codegen_function_prototypes(std::vector<ast::FunctionNode*> functions);
        void codegen_function_prototypes(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args_names, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern);
        void codegen_function_bodies(std::vector<ast::FunctionNode*> functions);
        void codegen_function_bodies(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args_names, std::vector<ast::Type> args_types, ast::Type return_type, ast::Node* function_body);
        llvm::Value* codegen(ast::AssignmentNode& node);
        llvm::Value* codegen(ast::FieldAssignmentNode& node);
        llvm::Value* codegen(ast::ReturnNode& node);
        llvm::Value* codegen(ast::BreakNode& node);
        llvm::Value* codegen(ast::ContinueNode& node);
        llvm::Value* codegen(ast::IfElseNode& node);
        llvm::Value* codegen(ast::WhileNode& node);
        llvm::Value* codegen(ast::UseNode& node) {return nullptr;}
        llvm::Value* codegen(ast::CallArgumentNode& node) {return nullptr;}
        llvm::Value* codegen(ast::CallNode& node);
        llvm::Value* codegen(ast::FloatNode& node);
        llvm::Value* codegen(ast::IntegerNode& node);
        llvm::Value* codegen(ast::IdentifierNode& node);
        llvm::Value* codegen(ast::BooleanNode& node);
        llvm::Value* codegen(ast::StringNode& node);
        llvm::Value* codegen(ast::FieldAccessNode& node);

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
            else if (type == ast::Type("bool"))    return llvm::Type::getInt1Ty(*(this->context));
            else if (type == ast::Type("string"))  return llvm::Type::getInt8PtrTy(*(this->context));
            else if (type == ast::Type("void"))    return llvm::Type::getVoidTy(*(this->context));
            else if (type.type_definition)         return this->get_struct_type(type.type_definition);
            else {
                std::cout <<"type: " << type.to_str() << "\n";
                assert(false);
            }
        }

        llvm::StructType* get_struct_type(ast::TypeNode* type_definition) {
            return llvm::StructType::getTypeByName(*this->context, this->get_mangled_type_name(type_definition->module_path, type_definition->identifier->value));
        }

        bool has_struct_type(ast::Node* expression) {
            if (this->get_type(expression).type_definition) {
                return true;
            }
            return false;
        }

        bool is_struct_type(ast::Type type) {
            return type.type_definition != nullptr;
        }

        llvm::FunctionType* get_function_type(std::vector<ast::Type> args_types, ast::Type return_type) {
            // Get args types
            std::vector<llvm::Type*> llvm_args_types;

            if (return_type.type_definition) {
                llvm_args_types.push_back(this->as_llvm_type(return_type)->getPointerTo());
            }

            for (auto arg: args_types) {
                llvm::Type* llvm_type = this->as_llvm_type(arg);
                if (this->is_struct_type(arg)) {
                    llvm_args_types.push_back(llvm_type->getPointerTo());
                }
                else {
                    llvm_args_types.push_back(llvm_type);
                }
            }

            // Get return type
            llvm::Type* llvm_return_type = this->as_llvm_type(return_type);
            if (return_type.type_definition) {
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

        ast::Type get_type(ast::Node* node) {
            if (ast::get_type(node).is_type_variable()) {
                assert(this->type_bindings.find(ast::get_type(node).to_str()) != this->type_bindings.end());
                return this->type_bindings[ast::get_type(node).to_str()];
            }
            else {
                return ast::get_type(node);
            }
        }

        std::vector<ast::Type> get_types(std::vector<ast::Node*> nodes) {
            std::vector<ast::Type> types;
            for (auto& node: nodes) {
                types.push_back(this->get_type(node));
            }
            return types;
        }

        std::vector<ast::Type> get_types(std::vector<ast::CallArgumentNode*> nodes) {
            std::vector<ast::Type> types;
            for (auto& node: nodes) {
                types.push_back(this->get_type(node->expression));
            }
            return types;
        }

        llvm::AllocaInst* create_allocation(std::string name, llvm::Type* type) {
            assert(this->current_entry_block);
            llvm::IRBuilder<> block(this->current_entry_block, this->current_entry_block->begin());
            return block.CreateAlloca(type, 0, name.c_str());
        }

        void store_fields(ast::Node* expression, llvm::Value* struct_allocation) {
            // Get struct type
            llvm::StructType* struct_type = this->get_struct_type(ast::get_type(expression).type_definition);

            // If the struct expression is a call
            if (expression->index() == ast::Call) {
                // Get call
                auto& call = std::get<ast::CallNode>(*expression);

                // If call is a struct constructror
                bool is_constructor = call.identifier->value == call.type.to_str();
                if (is_constructor) {
                    // For each field
                    for (size_t i = 0; i < call.args.size(); i++) {
                        // Get pointer to the field
                        llvm::Value* ptr = this->builder->CreateStructGEP(struct_type, struct_allocation, i);

                        // If the field type has a struct type
                        if (this->has_struct_type(call.args[i]->expression)) {
                            this->store_fields(call.args[i]->expression, ptr);
                        }
                        else {
                            this->builder->CreateStore(this->codegen(call.args[i]->expression), ptr);
                        }
                    }
                }
                else {
                    assert(call.function);
                    std::string name = this->get_mangled_function_name(call.function->module_path, call.identifier->value, this->get_types(call.args), this->get_type((ast::Node*) &call), call.function->is_extern);
                    llvm::Function* function = this->module->getFunction(name);
                    assert(function);
                    
                    // Codegen args
                    std::vector<llvm::Value*> args;
                    args.push_back(struct_allocation);
                    for (size_t i = 0; i < call.args.size(); i++) {
                        args.push_back(this->codegen(call.args[i]->expression));
                    }

                    // Make call
                    this->builder->CreateCall(function, args, "calltmp");
                }
            }
            else if (expression->index() == ast::Identifier) {
                auto& identifier = std::get<ast::IdentifierNode>(*expression);
                this->builder->CreateMemCpy(struct_allocation, llvm::MaybeAlign(), this->get_binding(identifier.value), llvm::MaybeAlign(), this->get_type_size(struct_type));
            }
            else {
                assert(false);
            }
        }

        void add_scope() {
            this->scopes.push_back(std::unordered_map<std::string, llvm::AllocaInst*>());
        }

        std::unordered_map<std::string, llvm::AllocaInst*>& current_scope() {
            return this->scopes[this->scopes.size() - 1];
        }

        void remove_scope() {
            this->scopes.pop_back();
        }

        llvm::Value* get_binding(std::string identifier) {
            for (auto scope = this->scopes.rbegin(); scope != this->scopes.rend(); scope++) {
                if (scope->find(identifier) != scope->end()) {
                    return (*scope)[identifier];
                }
            }
            assert(false);
            return nullptr;
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
            llvm::Value* struct_ptr = this->get_binding(node.fields_accessed[0]->value);
            if (((llvm::AllocaInst*)struct_ptr)->getAllocatedType()->isPointerTy()) {
                struct_ptr = this->builder->CreateLoad(struct_ptr);
            }
            assert(this->get_type((ast::Node*) node.fields_accessed[0]).type_definition);
            llvm::StructType* struct_type = this->get_struct_type(this->get_type((ast::Node*) node.fields_accessed[0]).type_definition);

            // Get type definition
            ast::TypeNode* type_definition = this->get_type((ast::Node*) node.fields_accessed[0]).type_definition;

            for (size_t i = 1; i < node.fields_accessed.size() - 1; i++) {
                // Get pointer to accessed fieldd
                struct_ptr = this->builder->CreateStructGEP(struct_type, struct_ptr, get_index_of_field(node.fields_accessed[i]->value, type_definition));
                
                // Update current type definition
                type_definition = this->get_type((ast::Node*) node.fields_accessed[i]).type_definition;
                struct_type = this->get_struct_type(type_definition);
            }
            
            // Get pointer to accessed fieldd
            size_t last_element = node.fields_accessed.size() - 1;
            llvm::Value* ptr = this->builder->CreateStructGEP(struct_type, struct_ptr, get_index_of_field(node.fields_accessed[last_element]->value, type_definition));
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
    llvm_ir.module->print(llvm::errs(), nullptr);
}

// Generate object code
// --------------------
static std::string get_object_file_name(std::string executable_name);
static void link(std::string executable_name, std::string object_file_name);

void codegen::generate_object_code(ast::Ast& ast, std::string program_name) {
    codegen::Context llvm_ir(ast);
    llvm_ir.codegen(ast);

    // Generate object file
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
    link(utilities::get_executable_name(program_name), get_object_file_name(program_name));

    // Remove generated object file
    remove(get_object_file_name(program_name).c_str());
}

#ifdef __linux__
    static std::string get_object_file_name(std::string executable_name) {
        return executable_name + ".o";
    }

    static void link(std::string executable_name, std::string object_file_name) {
        std::string name = "-o" + executable_name;
        std::vector<const char*> args = {
            "lld",
            object_file_name.c_str(),
            name.c_str(),
            "-dynamic-linker",
            "/lib64/ld-linux-x86-64.so.2",
            "-L/usr/lib/x86_64-linux-gnu",
            "-lc",
            "/usr/lib/x86_64-linux-gnu/crt1.o",
            "/usr/lib/x86_64-linux-gnu/crti.o",
            "/usr/lib/x86_64-linux-gnu/crtn.o"
        };

        std::string output = "";
        std::string errors = "";
        llvm::raw_string_ostream output_stream(output);
        llvm::raw_string_ostream errors_stream(errors);

        bool result = lld::elf::link(args, false, output_stream, errors_stream);
    }
#elif _WIN32
    static std::string get_object_file_name(std::string executable_name) {
        return executable_name + ".obj";
    }

    static void link(std::string executable_name, std::string object_file_name) {
        std::string name = "-out:" + executable_name;
        std::vector<const char*> args = {
            "lld",
            object_file_name.c_str(),
            "-defaultlib:libcmt",
            "-defaultlib:oldnames",
            "-nologo",
            name.c_str()
        };

        std::string output = "";
        std::string errors = "";
        llvm::raw_string_ostream output_stream(output);
        llvm::raw_string_ostream errors_stream(errors);

        bool result = lld::coff::link(args, false, output_stream, errors_stream);
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
        if (function->generic) {
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

void codegen::Context::codegen_function_prototypes(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args_names, std::vector<ast::Type> args_types, ast::Type return_type, bool is_extern) {
    // Make function type
    llvm::FunctionType* function_type = this->get_function_type(args_types, return_type);

    // Create function
    std::string name = this->get_mangled_function_name(module_path, identifier, args_types, return_type, is_extern);
    llvm::Function* f = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, name, this->module);

    // Set args for names
    if (this->is_struct_type(return_type)) {
        for (unsigned int i = 0; i < args_names.size(); i++) {
            f->getArg(i + 1)->setName(args_names[i]->value);
        }
    }
    else {
        for (unsigned int i = 0; i < args_names.size(); i++) {
            f->getArg(i)->setName(args_names[i]->value);
        }
    }
}

void codegen::Context::codegen_function_bodies(std::vector<ast::FunctionNode*> functions) {
    for (auto& function: functions) {
        if (function->is_extern) return;

        if (function->generic) {
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

void codegen::Context::codegen_function_bodies(std::filesystem::path module_path, std::string identifier, std::vector<ast::FunctionArgumentNode*> args_names, std::vector<ast::Type> args_types, ast::Type return_type, ast::Node* function_body) {
    std::string name = this->get_mangled_function_name(module_path, identifier, args_types, return_type, false);
    llvm::Function* f = this->module->getFunction(name);

    // Create the body of the function
    llvm::BasicBlock *body = llvm::BasicBlock::Create(*(this->context), "entry", f);
    this->builder->SetInsertPoint(body);

    // Set current entry block
    this->current_entry_block = &f->getEntryBlock();

    // Add arguments to scope
    this->add_scope();

    if (return_type.type_definition) {
        for (unsigned int i = 0; i < args_names.size() + 1; i++) {
            std::string name = "$result";
            if (i != 0) {
                name = args_names[i - 1]->value;
            }

            // Create allocation for argument
            auto allocation = this->create_allocation(name, f->getArg(i)->getType());

            // Store initial value
            this->builder->CreateStore(f->getArg(i), allocation);

            // Add arguments to scope
            this->current_scope()[name] = allocation;
        }
    }
    else {
         for (unsigned int i = 0; i < args_names.size(); i++) {
            std::string name = args_names[i]->value;

            // Create allocation for argument
            auto allocation = this->create_allocation(name, f->getArg(i)->getType());

            // Store initial value
            this->builder->CreateStore(f->getArg(i), allocation);

            // Add arguments to scope
            this->current_scope()[name] = allocation;
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

llvm::Value* codegen::Context::codegen(ast::AssignmentNode& node) {
    if (!this->has_struct_type(node.expression)) {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        if (node.nonlocal) {
            // Store value
            this->builder->CreateStore(expr, this->get_binding(node.identifier->value));
        }
        else {
            // Create allocation if doesn't exists or if already exists, but it has a different type
            if (this->current_scope().find(node.identifier->value) == this->current_scope().end()
            ||  this->current_scope()[node.identifier->value]->getType() != expr->getType()) {
                this->current_scope()[node.identifier->value] = this->create_allocation(node.identifier->value, expr->getType());
            }

            // Store value
            this->builder->CreateStore(expr, this->current_scope()[node.identifier->value]);
        }
        return nullptr;
    }
    else {
        // Create allocation
        llvm::StructType* struct_type = this->get_struct_type(ast::get_type(node.expression).type_definition);
        this->current_scope()[node.identifier->value] = this->create_allocation(node.identifier->value, struct_type);

        // Store fields
        this->store_fields(node.expression, this->current_scope()[node.identifier->value]);
  
        // Return
        return nullptr;
    }
}

llvm::Value* codegen::Context::codegen(ast::FieldAssignmentNode& node) {
    if (!this->has_struct_type(node.expression)) {
        // Generate value of expression
        llvm::Value* expr = this->codegen(node.expression);

        // Get field pointer
        llvm::Value* ptr = this->get_field_pointer(*node.identifier);

        // Store value
        this->builder->CreateStore(expr, ptr);
    }
    else {
        // Get field pointer
        llvm::Value* ptr = this->get_field_pointer(*node.identifier);

        // Store fields
        this->store_fields(node.expression, ptr);
  
        // Return
        return nullptr;
    }

    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::ReturnNode& node) {
    if (node.expression.has_value()) {
        if (!this->has_struct_type(node.expression.value())) {
            // Generate value of expression
            llvm::Value* expr = this->codegen(node.expression.value());

            // Create return value
            this->builder->CreateRet(expr);
        }
        else {            
            // Store fields
            this->store_fields(node.expression.value(), this->builder->CreateLoad(this->get_binding("$result")));
            
            // Return
            this->builder->CreateRetVoid();
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

            // Jump to merge block if does not return (Type("") means the if does not return)
            if (ast::get_type(node.if_branch) == ast::Type("")) {
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

            // Jump to merge block if if does not return (Type("") means the block does not return)
            if (ast::get_type(node.if_branch) == ast::Type("")) {
                this->builder->CreateBr(merge_block);
            }

            // Create else block
            current_function->getBasicBlockList().push_back(else_block);
            this->builder->SetInsertPoint(else_block);
            this->codegen(node.else_branch.value());

            // Jump to merge block if else does not return (Type("") means the block does not return)
            if (ast::get_type(node.else_branch.value()) == ast::Type("")) {
                this->builder->CreateBr(merge_block);
            }

            // Create merge block
            if (ast::get_type(node.if_branch) == ast::Type("") || ast::get_type(node.else_branch.value()) == ast::Type("")) {
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

llvm::Value* codegen::Context::codegen(ast::CallNode& node) {
    // If type is struct
    if (node.type.type_definition) {
        // Create allocation
        llvm::StructType* struct_type = this->get_struct_type(node.type.type_definition);
        llvm::AllocaInst* allocation = this->create_allocation(node.identifier->value, struct_type);
            
        // Store fields
        this->store_fields((ast::Node*) &node, allocation);

        // Return
        return allocation;
    }

    // Codegen args
    std::vector<llvm::Value*> args;
    for (size_t i = 0; i < node.args.size(); i++) {
        if (this->has_struct_type(node.args[i]->expression)) {
            // Create allocation
            llvm::StructType* struct_type = this->get_struct_type(this->get_type(node.args[i]->expression).type_definition);
            llvm::AllocaInst* allocation = this->create_allocation(node.function->args[i]->value, struct_type);

            // Store fields
            this->store_fields(node.args[i]->expression, allocation);

            // Add to args
            args.push_back(allocation);
        }
        else {
            args.push_back(this->codegen(node.args[i]->expression));
        }
    }

    if (node.args.size() == 2) {
        if (node.identifier->value == "+") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFAdd(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateAdd(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateAdd(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "-") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFSub(args[0], args[1], "subtmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateSub(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateSub(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "*") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFMul(args[0], args[1], "multmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateMul(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateMul(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "/") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFDiv(args[0], args[1], "divtmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateSDiv(args[0], args[1], "divtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
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
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateICmpULT(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateICmpULT(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "<=") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpULE(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateICmpULE(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateICmpULE(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == ">") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUGT(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateICmpUGT(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateICmpUGT(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == ">=") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUGE(args[0], args[1], "cmptmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateICmpUGE(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
                return this->builder->CreateICmpUGE(args[0], args[1], "addtmp");
            }
        }
        if (node.identifier->value == "==") {
            if (args[0]->getType()->isDoubleTy() && args[1]->getType()->isDoubleTy()) {
                return this->builder->CreateFCmpUEQ(args[0], args[1], "eqtmp");
            }
            if (args[0]->getType()->isIntegerTy(1) && args[1]->getType()->isIntegerTy(1)) {
                return this->builder->CreateICmpEQ(args[0], args[1], "eqtmp");
            }
            if (args[0]->getType()->isIntegerTy(64) && args[1]->getType()->isIntegerTy(64)) {
                return this->builder->CreateICmpEQ(args[0], args[1], "addtmp");
            }
            if (args[0]->getType()->isIntegerTy(32) && args[1]->getType()->isIntegerTy(32)) {
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
            if (args[0]->getType()->isIntegerTy(64)) {
                return this->builder->CreateNeg(args[0], "negation");
            }
            if (args[0]->getType()->isIntegerTy(32)) {
                return this->builder->CreateNeg(args[0], "negation");
            }
        }
        if (node.identifier->value == "not") {
            return this->builder->CreateNot(args[0], "not");
        }
    }
    if (node.identifier->value == "print") {
        if (args[0]->getType()->isDoubleTy()) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(this->get_global_string("%g\n"));
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
        if (args[0]->getType()->isIntegerTy(64)) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(this->get_global_string("%d\n"));
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
        if (args[0]->getType()->isIntegerTy(32)) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(this->get_global_string("%d\n"));
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
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
        if (this->get_type(node.args[0]->expression) == ast::Type("string")) {
            std::vector<llvm::Value*> printArgs;
            printArgs.push_back(args[0]);
            this->builder->CreateCall(this->module->getFunction("printf"), printArgs);
            return nullptr;
        }
    }

    // Get function
    assert(node.function);
    std::string name = this->get_mangled_function_name(node.function->module_path, node.identifier->value, this->get_types(node.args), this->get_type((ast::Node*) &node), node.function->is_extern);
    llvm::Function* function = this->module->getFunction(name);
    assert(function);

    // Make call
    return this->builder->CreateCall(function, args, "calltmp");
}

llvm::Value* codegen::Context::codegen(ast::FloatNode& node) {
    return llvm::ConstantFP::get(*(this->context), llvm::APFloat(node.value));
}

llvm::Value* codegen::Context::codegen(ast::IntegerNode& node) {
    if (this->get_type((ast::Node*) &node) == ast::Type("float64"))
        return llvm::ConstantFP::get(*(this->context), llvm::APFloat((double)node.value));
    else if (this->get_type((ast::Node*) &node) == ast::Type("int64")) {
        return llvm::ConstantInt::get(*(this->context), llvm::APInt(64, node.value, true));
    }
    else if (this->get_type((ast::Node*) &node) == ast::Type("int32")) {
        return llvm::ConstantInt::get(*(this->context), llvm::APInt(32, node.value, true));
    }
    
    assert(false);
    return nullptr;
}

llvm::Value* codegen::Context::codegen(ast::IdentifierNode& node) {
    if (this->has_struct_type((ast::Node*) &node)) {
        return this->get_binding(node.value);
    }
    return this->builder->CreateLoad(this->get_binding(node.value), node.value.c_str());
}

llvm::Value* codegen::Context::codegen(ast::BooleanNode& node) {
    return llvm::ConstantInt::getBool(*(this->context), node.value);
}

llvm::Value* codegen::Context::codegen(ast::StringNode& node) {
    return this->get_global_string(node.value);
}

llvm::Value* codegen::Context::codegen(ast::FieldAccessNode& node) {
    return this->builder->CreateLoad(this->get_field_pointer(node));
}
