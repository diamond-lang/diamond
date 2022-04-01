#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <string>
#include <filesystem>

#include "parser.hpp"
#include "ast.hpp"

namespace errors {
	std::string usage();
	std::string unexpected_character(Location location);
	std::string unexpected_indent(Location location);
	std::string expecting_statement(Location location);
	std::string expecting_new_indentation_level(Location location);
	std::string undefined_variable(ast::IdentifierNode& identifier, std::filesystem::path file);
	std::string reassigning_immutable_variable(ast::IdentifierNode& identifier, ast::AssignmentNode& assignment, std::filesystem::path file);
	std::string undefined_function(ast::CallNode& call, std::filesystem::path file);
	std::string unhandled_return_value(ast::CallNode& call, std::filesystem::path file);
	std::string file_couldnt_be_found(std::string path);
}

#endif
