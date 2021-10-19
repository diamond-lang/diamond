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
std::string current_line(Source source);
std::string current_line(std::shared_ptr<Ast::Node> node);
std::string current_line(size_t line, std::string file_path);
std::string underline_current_char(Source source);
std::string underline_current_line(Source source);
std::string underline_identifier(std::shared_ptr<Ast::Identifier> identifier);

// Implementantions
// ----------------
std::string errors::usage() {
	return make_header("diamond [program file]\n") +
	                 "    Compiles a program.\n\n" +
	       make_header("diamond run [program file]\n") +
	                 "    Runs a program.\n\n";
}

std::string errors::unexpected_character(Source source) {
	return make_header("Unexpected character\n\n") +
	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
	       underline_current_char(source);
}

std::string errors::expecting_statement(Source source) {
	return make_header("Expecting a statement\n\n") +
	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
	       underline_current_line(source);
}

std::string errors::undefined_variable(std::shared_ptr<Ast::Identifier> identifier) {
	return make_header("Undefined variable\n\n") +
	       std::to_string(identifier->line) + "| " + current_line(identifier) + "\n" +
	       underline_identifier(identifier);
}

std::string errors::reassigning_immutable_variable(std::shared_ptr<Ast::Identifier> identifier, std::shared_ptr<Ast::Assignment> assignment) {
	return make_header("Trying to reassign immutable variable\n\n") +
	       std::to_string(identifier->line) + "| " + current_line(identifier) + "\n" +
	       underline_identifier(identifier) + "\n" +
	       "Previously defined here:\n\n" +
	       std::to_string(assignment->line) + "| " + current_line(assignment);
}

std::string format_args(std::vector<std::shared_ptr<Ast::Expression>> args) {
	std::string result = "";
	if (args.size() >= 1) result += args[0]->type.to_str();
	for (size_t i = 1; i < args.size(); i++) {
		result += ", " + args[i]->type.to_str();
	}
	return result;
}

std::string errors::undefined_function(std::shared_ptr<Ast::Call> call) {
	return make_header("Undefined function\n\n") +
	       call->identifier->value + "(" + format_args(call->args) + ") is not defined.\n\n" +
	       std::to_string(call->line) + "| " + current_line(call) + "\n" +
	       underline_identifier(call->identifier);
}

std::string errors::file_couldnt_be_found(std::string path) {
	return make_header("File not found\n\n") +
	       "\"" + path + "\"" + " couldn't be found." + "\n";
}

std::string current_line(Source source)   {return current_line(source.line, source.file);}
std::string current_line(std::shared_ptr<Ast::Node> node) {return current_line(node->line, node->file);}
std::string current_line(size_t line, std::string file_path) {
	if (file_path == "") return "";

	// Read file
	std::string file = utilities::read_file(file_path).get_value();

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

std::string underline_current_char(Source source) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(source.line).size(); i++) {
		result += ' '; // Add space for line number
	}
	result += "  "; // Add space for | character after number

	std::string line = current_line(source);
	size_t col = source.col - 1;
	for (auto it = line.begin(); it != line.end(); it++) {
		if (col <= 0)    break;
		if (*it == '\t') result += *it;
		else             result += ' ';
		col -= 1;
	}
	result += make_red("^");
	return result;
}

std::string underline_current_line(Source source) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(source.line).size(); i++) {
		result += ' '; // Add space for line number
	}
	result += "  "; // Add space for | and space after

	std::string line = current_line(source);
	for (auto it = line.begin(); it != line.end(); it++) {
		result += make_red("^");	
	}
	return result;
}

std::string underline_identifier(std::shared_ptr<Ast::Identifier> identifier) {
	std::string result = "";
	for (size_t i = 0; i < std::to_string(identifier->line).size(); i++) {
		result += " "; // Add space for line number
	}
	result += " "; // Add space for |
	result += " "; // Add space for space after |

	std::string line = current_line(identifier);
	size_t col = 1;
	for (auto it = line.begin(); it != line.end(); it++) {
		if (col == identifier->col) break;
		if (*it == '\t')            result += '\t';
		else                        result += " ";
		col += 1;
	}
	for (size_t i = 0; i < identifier->value.size(); i++) {
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
