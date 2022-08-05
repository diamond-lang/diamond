#include <fstream>
#include <sstream>
#include <iostream>

#include "errors.hpp"
#include "utilities.hpp"

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
std::string current_line(Location location);
std::string current_line(size_t line, std::filesystem::path file_path);
std::string underline_current_char(Location location);
std::string underline_current_line(Location location);
std::string underline_identifier(ast::IdentifierNode& identifier, std::filesystem::path file);

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
					   "        --tokens\n"
					   "        --ast\n"
					   "        --ast-with-types\n"
					   "        --ast-with-concrete-types\n"
					   "        --llvm-ir\n";
					  // "                 \n";
}

std::string errors::unexpected_character(Location location) {
	return make_header("Unexpected character\n\n") +
	       std::to_string(location.line) + "| " + current_line(location) + "\n" +
	       underline_current_char(location);
}

std::string errors::unexpected_indent(Location location) {
	return make_header("Unexpected indent\n\n") +
	       std::to_string(location.line) + "| " + current_line(location) + "\n" +
	       underline_current_char(location);
}

std::string errors::expecting_statement(Location location) {
	return make_header("Expecting a statement\n\n") +
	       std::to_string(location.line) + "| " + current_line(location) + "\n" +
	       underline_current_line(location);
}

std::string errors::expecting_new_indentation_level(Location location) {
	return make_header("Expecting new indentation level\n\n") +
		   std::to_string(location.line - 1) + "| " + current_line(location.line - 1, location.file) + "\n" +
	       std::to_string(location.line) + "| " + current_line(location) + "\n" +
	       underline_current_char(location);
}

std::string errors::undefined_variable(ast::IdentifierNode& identifier, std::filesystem::path file) {
	return make_header("Undefined variable\n\n") +
	       std::to_string(identifier.line) + "| " + current_line(identifier.line, file) + "\n" +
	       underline_identifier(identifier, file);
}

std::string errors::reassigning_immutable_variable(ast::IdentifierNode& identifier, ast::AssignmentNode& assignment, std::filesystem::path file) {
	return make_header("Trying to reassign immutable variable\n\n") +
	       std::to_string(identifier.line) + "| " + current_line(identifier.line, file) + "\n" +
	       underline_identifier(identifier, file) + "\n" +
	       "Previously defined here:\n\n" +
	       std::to_string(assignment.line) + "| " + current_line(assignment.line, file);
}

static std::string format_args(std::vector<ast::CallArgumentNode*> args) {
	std::string result = "";
	if (args.size() >= 1) result += ast::get_type(args[0]->expression).to_str();
	for (size_t i = 1; i < args.size(); i++) {
		result += ", " + ast::get_type(args[i]->expression).to_str();
	}
	return result;
}

std::string errors::undefined_function(ast::CallNode& call, std::filesystem::path file) {
	auto& identifier = call.identifier->value;
	return make_header("Undefined function\n\n") +
	       identifier + "(" + format_args(call.args) + ") is not defined.\n\n" +
	       std::to_string(call.line) + "| " + current_line(call.line, file) + "\n" +
	       underline_identifier(*call.identifier, file);
}

std::string errors::unhandled_return_value(ast::CallNode& call, std::filesystem::path file) {
	return make_header("Unhandled return value\n\n") +
	       std::to_string(call.line) + "| " + current_line(call.line, file) + "\n" +
	       underline_identifier(*call.identifier, file);
}

std::string errors::file_couldnt_be_found(std::filesystem::path path) {
	return make_header("File not found\n\n") +
	       "\"" + path.string() + "\"" + " couldn't be found." + "\n";
}

std::string current_line(Location location) {return current_line(location.line, location.file);}
std::string current_line(size_t line, std::filesystem::path file_path) {
	if (file_path == "") return "";

	// Read file
	std::string file = utilities::read_file(file_path);

	// Get line
	std::string result = "";
	for (auto it = file.begin(); it != file.end(); it++) {
		if ((*it) == '\n') {
			line--;
			continue;
		}

		if (line == 1) {
			result += *it;
		}
	}
	return result;
}

std::string underline_current_char(Location location) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(location.line).size(); i++) {
		result += ' '; // Add space for line number
	}
	result += "  "; // Add space for | character after number

	std::string line = current_line(location);
	size_t column = location.column - 1;
	for (auto it = line.begin(); it != line.end(); it++) {
		if (column <= 0)    break;
		if (*it == '\t') result += *it;
		else             result += ' ';
		column -= 1;
	}
	result += make_red("^");
	return result;
}

std::string underline_current_line(Location location) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(location.line).size(); i++) {
		result += ' '; // Add space for line number
	}
	result += "  "; // Add space for | and space after

	std::string line = current_line(location);
	for (auto it = line.begin(); it != line.end(); it++) {
		result += make_red("^");
	}
	return result;
}

std::string underline_identifier(ast::IdentifierNode& identifier, std::filesystem::path file) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(identifier.line).size(); i++) {
		result += " "; // Add space for line number
	}
	result += " "; // Add space for |
	result += " "; // Add space for space after |

	std::string line = current_line(identifier.line, file);
	size_t col = 1;
	for (auto it = line.begin(); it != line.end(); it++) {
		if (col == identifier.column) break;
		if (*it == '\t')            result += '\t';
		else                        result += " ";
		col += 1;
	}
	for (size_t i = 0; i < identifier.value.size(); i++) {
		result += make_red("^");
	}
	return result;
}

std::string make_header(std::string str)         {return make_bright_cyan(str);}
std::string make_bold(std::string str)           {return "\x1b[1m" + str + "\x1b[0m";}
std::string make_underline(std::string str)      {return "\x1b[4m" + str + "\x1b[0m";}
std::string make_cyan(std::string str)           {return "\x1b[36m" + str + "\x1b[0m";}
std::string make_bright_cyan(std::string str)    {return "\x1b[96m" + str + "\x1b[0m";}
std::string make_red(std::string str)            {return "\x1b[31m" + str + "\x1b[0m";}
std::string make_magenta(std::string str)        {return "\x1b[35m" + str + "\x1b[0m";}
std::string make_bright_magenta(std::string str) {return "\x1b[95m" + str + "\x1b[0m";}
