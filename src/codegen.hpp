#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "ast.hpp"

namespace codegen {
    void generate_executable(ast::Ast& ast, std::string program_name);
    void print_llvm_ir(ast::Ast& ast, std::string program_name);
    void generate_object_code(ast::Ast& ast, std::string program_name);
}

#endif
