#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <string>
#include <filesystem>

#include "shared.hpp"
#include "parser.hpp"
#include "ast.hpp"

namespace errors {
	std::string usage();
	std::string unexpected_character(parse::Source source);
	std::string unexpected_indent(parse::Source source);
	std::string expecting_statement(parse::Source source);
	std::string expecting_new_indentation_level(parse::Source source);
	std::string undefined_variable(std::shared_ptr<Ast::Identifier> identifier);
	std::string reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment);
	std::string undefined_function(std::shared_ptr<Ast::Call> call);
	std::string unhandled_return_value(std::shared_ptr<Ast::Call> call);
	std::string file_couldnt_be_found(std::string path);
}

#endif
