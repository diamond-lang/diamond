#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "ast.hpp"

void generate_executable(std::shared_ptr<Ast::Program> program, std::string program_name);
void print_llvm_ir(std::shared_ptr<Ast::Program> program, std::string program_name);

#endif
