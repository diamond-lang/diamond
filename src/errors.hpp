#include "types.hpp"

namespace errors {
	std::string usage();
	std::string unexpected_character(Source source);
	std::string unexpected_indent(Source source);
	std::string expecting_statement(Source source);
	std::string expecting_block_or_expression(Source source);
	std::string expecting_new_indentation_level(Source source);
	std::string undefined_variable(std::shared_ptr<Ast::Identifier> identifier);
	std::string reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment);
	std::string undefined_function(std::shared_ptr<Ast::Call> call);
	std::string unhandled_return_value(std::shared_ptr<Ast::Call> call);
	std::string file_couldnt_be_found(std::string path);
}
