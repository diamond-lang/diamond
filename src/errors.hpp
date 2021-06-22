#include "types.hpp"

namespace errors {
	std::string usage();
	std::string expecting_number(Source source);
	std::string expecting_identifier(Source source);
	std::string unexpected_character(Source source);
	std::string undefined_variable(std::shared_ptr<Ast::Identifier> identifier);
	std::string reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment);
	std::string operation_not_defined_for(std::shared_ptr<Ast::Call> call, std::string left, std::string right);
	std::string file_couldnt_be_found(std::string path);
}
