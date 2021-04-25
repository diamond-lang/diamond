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
std::string make_red(std::string str);
std::string make_magenta(std::string str);
std::string current_line(Source source);
std::string underline_current_char(Source source);


// Implementantions
// ----------------
std::string errors::usage() {
	return make_header("diamond [program file]\n") +
	                 "    Compiles a program.\n\n" +
	       make_header("diamond run [program file]\n") +
	                 "    Runs a program.\n\n";
}

std::string errors::expecting_number(Source source) {
	return make_header("Expecting number\n\n") +
	       std::to_string(source.line) + "| " + current_line(source) + "\n" +
	       underline_current_char(source);
}

std::string errors::file_couldnt_be_found(std::string path) {
	return make_header("File not found\n\n") +
	       "\"" + path + "\"" + " couldn't be found." + "\n";
}

std::string current_line(Source source) {
	// Read file
	std::string file = utilities::read_file(source.file).file;

	// Get line
	size_t line = source.line;
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

std::string make_header(std::string str)    {return make_cyan(str);}
std::string make_bold(std::string str)      {return "\u001b[1m" + str + "\x1b[0m";}
std::string make_underline(std::string str) {return "\u001b[4m" + str + "\x1b[0m";}
std::string make_cyan(std::string str)      {return "\u001b[36m" + str + "\x1b[0m";}
std::string make_red(std::string str)       {return "\x1b[31m" + str + "\x1b[0m";}
std::string make_magenta(std::string str)   {return "\u001b[35;1m" + str + "\x1b[0m";}
