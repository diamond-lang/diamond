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
std::string get_lines(size_t start_line, size_t end_line, std::filesystem::path file_path);
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
                       "        --llvm-ir\n"
                       "        --obj\n";
}

std::string errors::generic_error(Location location, std::string message) {
    return make_header(message + "\n\n") +
            (location.line > 1 ? std::to_string(location.line - 1) + "| " + current_line(Location{location.line - 1, location.column, location.file}) + "\n" : "") +
            std::to_string(location.line) + "| " + current_line(location) + "\n" +
            underline_current_char(location);
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

std::string errors::undefined_function(ast::CallNode& call, std::filesystem::path file) {
    auto& identifier = call.identifier->value;
    return make_header("Undefined function\n\n") +
           identifier + " is not defined.\n\n" +
           std::to_string(call.line) + "| " + current_line(call.line, file) + "\n" +
           underline_identifier(*call.identifier, file);
}

std::string format_args(std::vector<ast::Type> args) {
    std::string result = "";
    if (args.size() >= 1) result += args[0].to_str();
    for (size_t i = 1; i < args.size(); i++) {
        result += ", " + args[i].to_str();
    }
    return result;
}

std::string errors::undefined_function(ast::CallNode& call, std::vector<ast::Type> args, std::filesystem::path file) {
    auto& identifier = call.identifier->value;
    return make_header("Undefined function\n\n") +
           identifier + "(" + format_args(args) + ") is not defined.\n\n" +
           std::to_string(call.line) + "| " + current_line(call.line, file) + "\n" +
           underline_identifier(*call.identifier, file);
}

static std::string list_functions(std::vector<ast::FunctionNode*> functions) {
    std::string result = "";
    for (auto function: functions) {
        result += "    " + std::to_string(function->line) + "| " + current_line(function->line, function->module_path) + "\n";
    }
    return result;
}

std::string errors::ambiguous_what_function_to_call(ast::CallNode& call, std::filesystem::path file, std::vector<ast::FunctionNode*> functions) {
    std::string result = make_header("Type mismatch\n\n");
    result += std::to_string(call.line) + "| " + current_line(call.line, file) + "\n";
    result += underline_identifier(*call.identifier, file) + "\n";
    result +=  "Here '" + call.identifier->value + "'  can refer to:\n";
    result += list_functions(functions);
    return result;
}

std::string ordinal(size_t number) {
    std::string suffix = "th";
    if (number % 100 < 11 || number % 100 > 13) {
        if (number % 10 == 1) {suffix = "st";}
        else if (number % 10 == 2) {suffix = "nd";}
        else if (number % 10 == 3) {suffix = "rd";}
    }
    return  std::to_string(number) + suffix;
}

std::string errors::unexpected_type(ast::CallNode& call, std::filesystem::path file, size_t arg_index, std::vector<ast::Type> expected_types) {
    std::string result = make_header("Type mismatch\n\n");
    result += "The type of the " + ordinal(arg_index + 1) + " argument of '" + call.identifier->value + "' doesn't match with what I expect." + "\n\n";
    result += get_lines(call.line, call.end_line, file) + "\n";
    result += "The type received is:\n\n";
    result += call.args[arg_index]->type.to_str() + "\n\n";
    result += "But possible types are:\n\n";
    for (size_t i = 0; i < expected_types.size(); i++) {
        result += expected_types[i].to_str();
        if (i + 1 != expected_types.size()) result += " or ";
    }
    result += "\n";
    return result;
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

int number_of_digits(size_t number) {
    int digits = 0;
    while (number != 0) {
        number /= 10;
        digits++;
    }
    return digits;
}

std::string get_lines(size_t start_line, size_t end_line, std::filesystem::path file_path) {
    assert(start_line <= end_line);
    std::string result = "";
    size_t digits_end = number_of_digits(end_line);

    size_t current = start_line;
    while (current <= end_line) {
        size_t digits = number_of_digits(current);
        for (size_t i = 0; i < digits_end - digits; i++) {
            result += " ";
        }
        result += std::to_string(current) + "│ " + current_line(current, file_path) + "\n";
        current++;
    }
    return result;
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
