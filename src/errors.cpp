#include <fstream>
#include <sstream>
#include <iostream>

#include "errors.hpp"

// Prototypes
// ----------
std::string make_header(std::string str);
std::string make_bold(std::string str);
std::string make_underline(std::string str);
std::string make_cyan(std::string str);
std::string make_bright_cyan(std::string str);
std::string make_red(std::string str);
std::string make_magenta(std::string str);
std::string make_bright_magenta(std::string str);

// Implementantions
// ----------------
std::string errors::usage() {
	return make_header("diamond build [program file]\n") +
	                 "    Creates a native executable from the program.\n\n" +
	       make_header("diamond run [program file]\n") +
	                 "    Runs the program.\n\n" +
		   make_header("diamond emit [options] [program file]\n") +
		               "    This command emits intermediary representations of\n"
					   "    the program. Is useful for debugging the compiler.\n\n"
					   "    The options are:\n"
					   "        --ast\n"
					   "        --ast-with-types\n"
					   "        --ast-with-concrete-types\n"
					   "        --llvm-ir\n";
}

std::string errors::file_couldnt_be_found(std::filesystem::path path) {
	return make_header("File not found\n\n") +
	       "\"" + path.string() + "\"" + " couldn't be found." + "\n";
}

// std::string errors::unexpected_character(Source source) {
// 	return make_header("Unexpected character\n\n") +
// 	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
// 	       underline_current_char(source);
// }

// std::string errors::unexpected_indent(Source source) {
// 	return make_header("Unexpected indent\n\n") +
// 	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
// 	       underline_current_char(source);
// }

// std::string errors::expecting_statement(Source source) {
// 	return make_header("Expecting a statement\n\n") +
// 	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
// 	       underline_current_line(source);
// }

// std::string errors::expecting_new_indentation_level(Source source) {
// 	return make_header("Expecting new indentation level\n\n") +
// 		   std::to_string(source.line - 1) + "| " + current_line(source.line - 1, source.file) + "\n" +
// 	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
// 	       underline_current_char(source);
// }

// std::string errors::undefined_variable(std::shared_ptr<Ast::Identifier> identifier) {
// 	return make_header("Undefined variable\n\n") +
// 	       std::to_string(identifier->line) + "| " + current_line(identifier) + "\n" +
// 	       underline_identifier(identifier);
// }

// std::string errors::reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment) {
// 	return make_header("Trying to reassign immutable variable\n\n") +
// 	       std::to_string(identifier->line) + "| " + current_line(identifier) + "\n" +
// 	       underline_identifier(identifier) + "\n" +
// 	       "Previously defined here:\n\n" +
// 	       std::to_string(assignment->line) + "| " + current_line(assignment);
// }

// std::string format_args(std::vector<std::shared_ptr<Ast::Expression>> args) {
// 	std::string result = "";
// 	if (args.size() >= 1) result += args[0]->type.to_str();
// 	for (size_t i = 1; i < args.size(); i++) {
// 		result += ", " + args[i]->type.to_str();
// 	}
// 	return result;
// }

// std::string errors::undefined_function(std::shared_ptr<Ast::Call> call) {
// 	return make_header("Undefined function\n\n") +
// 	       call->identifier->value + "(" + format_args(call->args) + ") is not defined.\n\n" +
// 	       std::to_string(call->line) + "| " + current_line(call) + "\n" +
// 	       underline_identifier(call->identifier);
// }

// std::string errors::unhandled_return_value(std::shared_ptr<Ast::Call> call) {
// 	return make_header("Unhandled return value\n\n") +
// 	       std::to_string(call->line) + "| " + current_line(call) + "\n" +
// 	       underline_identifier(call->identifier);
// }

std::string make_header(std::string str)         {return make_bright_cyan(str);}
std::string make_bold(std::string str)           {return "\x1b[1m" + str + "\x1b[0m";}
std::string make_underline(std::string str)      {return "\x1b[4m" + str + "\x1b[0m";}
std::string make_cyan(std::string str)           {return "\x1b[36m" + str + "\x1b[0m";}
std::string make_bright_cyan(std::string str)    {return "\x1b[96m" + str + "\x1b[0m";}
std::string make_red(std::string str)            {return "\x1b[31m" + str + "\x1b[0m";}
std::string make_magenta(std::string str)        {return "\x1b[35m" + str + "\x1b[0m";}
std::string make_bright_magenta(std::string str) {return "\x1b[95m" + str + "\x1b[0m";}
