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
	// std::string undefined_variable(std::shared_ptr<Ast::Identifier> identifier);
	// std::string reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment);
	// std::string undefined_function(std::shared_ptr<Ast::Call> call);
	// std::string unhandled_return_value(std::shared_ptr<Ast::Call> call);
	std::string file_couldnt_be_found(std::string path);
}

#endif
