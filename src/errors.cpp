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
std::string get_call_stack(std::vector<ast::CallInCallStack> call_stack);

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

std::string errors::reassigning_immutable_variable(ast::IdentifierNode& identifier, ast::DeclarationNode& declaration, std::filesystem::path file) {
    return make_header("Trying to reassign immutable variable\n\n") +
           std::to_string(identifier.line) + "| " + current_line(identifier.line, file) + "\n" +
           underline_identifier(identifier, file) + "\n" +
           "Previously defined here:\n\n" +
           std::to_string(declaration.line) + "| " + current_line(declaration.line, file);
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

std::string errors::ambiguous_what_function_to_call(ast::CallNode& call, std::filesystem::path file, std::vector<ast::FunctionNode*> functions, std::vector<ast::CallInCallStack> call_stack) {
    std::string result = make_header("Ambiguous call\n\n");
    result += std::to_string(call.line) + "| " + current_line(call.line, file) + "\n";
    result += underline_identifier(*call.identifier, file) + "\n";
    result +=  "Here '" + call.identifier->value + "' could refer to:\n";
    result += list_functions(functions);
    if (call_stack.size() > 0) {
        result += get_call_stack(call_stack);
    }
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

std::string errors::unexpected_argument_type(ast::CallNode& call, std::filesystem::path file, size_t arg_index, ast::Type type_received, std::vector<ast::Type> expected_types, std::vector<ast::CallInCallStack> call_stack) {
    std::string result = make_header("Type mismatch\n\n");
    result += "The type of the " + ordinal(arg_index + 1) + " argument of '" + call.identifier->value + "' doesn't match with what I expect." + "\n\n";
    result += get_lines(call.line, call.end_line, file) + "\n";
    result += "The type received is:\n\n";
    result +=  type_received.to_str() + "\n\n";
    result += "But possible types are:\n\n";
    for (size_t i = 0; i < expected_types.size(); i++) {
        result += expected_types[i].to_str();
        if (i + 1 != expected_types.size()) result += " or ";
    }
    result += "\n";
    if (call_stack.size() > 0) {
        result += get_call_stack(call_stack);
    }
    return result;
}

std::string errors::unexpected_return_type(ast::CallNode& call, std::filesystem::path file, ast::Type type_received, std::vector<ast::Type> expected_types, std::vector<ast::CallInCallStack> call_stack) {
    std::string result = make_header("Type mismatch\n\n");
    result += "The return type of '" + call.identifier->value + "' doesn't match with what I expect." + "\n\n";
    result += get_lines(call.line, call.end_line, file) + "\n";
    result += "The specified type is:\n\n";
    result +=  type_received.to_str() + "\n\n";
    result += "But possible types are:\n\n";
    for (size_t i = 0; i < expected_types.size(); i++) {
        result += expected_types[i].to_str();
        if (i + 1 != expected_types.size()) result += " or ";
    }
    result += "\n";
    if (call_stack.size() > 0) {
        result += get_call_stack(call_stack);
    }
    return result;
}

std::string errors::unexpected_argument_types(ast::CallNode& call, std::filesystem::path file, size_t arg_index, std::vector<ast::Type> types_received, std::vector<ast::Type> expected_types, std::vector<ast::CallInCallStack> call_stack) {
    std::string result = make_header("Type mismatch\n\n");
    result += "The type of the " + ordinal(arg_index + 1) + " argument of '" + call.identifier->value + "' doesn't match with what I expect." + "\n\n";
    result += get_lines(call.line, call.end_line, file) + "\n";
    result += "The type received is:\n\n";
    for (size_t i = 0; i < types_received.size(); i++) {
        result += types_received[i].to_str();
        if (i + 1 != types_received.size()) result += " or ";
    }
    result += "\n\n";
    result += "But possible types are:\n\n";
    for (size_t i = 0; i < expected_types.size(); i++) {
        result += expected_types[i].to_str();
        if (i + 1 != expected_types.size()) result += " or ";
    }
    result += "\n";
    if (call_stack.size() > 0) {
        result += get_call_stack(call_stack);
    }
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

std::string get_call_stack(std::vector<ast::CallInCallStack> call_stack) {
    std::string result = "";
    result += "\nCalled from:\n";

    size_t max_number_of_digits = 1;
    size_t max_path_length = 1;

    for (auto call: call_stack) {
        if (number_of_digits(call.call->line) > max_number_of_digits) {
            max_number_of_digits = number_of_digits(call.call->line);
        }

        auto relative_path_length = call.file.lexically_relative(std::filesystem::current_path()).string().size();
        if (call.file.string().size() > max_path_length) {
            max_path_length = relative_path_length;
        }
    }

    for (auto call_it = call_stack.rbegin(); call_it != call_stack.rend(); call_it++) {
        auto& call = *call_it;
        result += "    ";

        auto relative_path = call.file.lexically_relative(std::filesystem::current_path()).string();
        size_t relative_path_length = relative_path.size();
        for (size_t i = 0; i < max_path_length - relative_path_length; i++) {
            result += " ";
        }

        result += relative_path + ": ";

        size_t digits = number_of_digits(call.call->line);
        for (size_t i = 0; i < max_number_of_digits - digits; i++) {
            result += " ";
        }
        result += std::to_string(call.call->line) + "| " + call.identifier + "(";
        for (size_t i = 0; i < call.args.size(); i++) {
            result += call.function->args[i]->identifier->value + ": ";
            result += call.args[i].to_str();
            if (i + 1 != call.args.size()) result += ", ";
        }
        result += ")\n";
    }

    return result;
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
