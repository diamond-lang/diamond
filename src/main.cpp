#include <fstream>
#include <sstream>
#include <iostream>

#include "errors.hpp"
#include "utilities.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

#ifdef _WIN32
#include <Windows.h>

void enable_colored_text_and_unicode() {
	// Colored text
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(handle, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	// Unicode
	SetConsoleOutputCP(65001);
}
#endif

enum CommandType {
	BuildCommand,
	RunCommand,
	EmitCommand
};

struct Command {
	std::string file;
	CommandType type;
	std::vector<std::string> options;

	Command(std::string file, CommandType type) : file(file), type(type) {}
	Command(std::string file, CommandType type, std::vector<std::string> options) : file(file), type(type), options(options) {}
	~Command() {}
};

void print_usage_and_exit();
void check_usage(int argc, char *argv[]);
Command get_command(int argc, char *argv[]);

int main(int argc, char *argv[]) {
	#ifdef _WIN32
		enable_colored_text_and_unicode();
	#endif

	auto tokens = lexer::lex(std::filesystem::path(argv[2]));

	if (tokens.is_error()) {
		for (size_t i = 0; i < tokens.get_error().size(); i++) {
			std::cout << tokens.get_error()[i].message << "\n";
		}
	}
	else {
		for (size_t i = 0; i < tokens.get_value().size(); i++) {
			std::cout << tokens.get_value()[i].get_literal() << "\n";
		}
	}

	// // Check usage
	// check_usage(argc, argv);

	// // Get command line arguments
	// Command command = get_command(argc, argv);

	// std::string program_name = utilities::get_program_name(command.file);
	// bool executable_already_existed = utilities::file_exists(utilities::get_executable_name(program_name));

	// // Read file
	// Result<std::string, Error> result = utilities::read_file(command.file);
	// if (result.is_error()) {
	// 	std::cout << result.get_error().message;
	// 	exit(EXIT_FAILURE);
	// }
	// std::string file = result.get_value();

	// // Parse
	// auto parsing_result = parse::program(Source(command.file, file.begin(), file.end()));
	// if (parsing_result.is_error()) {
	// 	std::vector<Error> errors = parsing_result.get_errors();
	// 	for (size_t i = 0; i < errors.size(); i++) {
	// 		std::cout << errors[i].message << '\n';
	// 	}
	// 	exit(EXIT_FAILURE);
	// }
	// auto ast = parsing_result.get_value();

	// if (command.type == EmitCommand && command.options[0] == std::string("--ast")) {
	// 	ast->print();
	// 	return 0;
	// }

	// // Analyze
	// auto analyze_result = semantic::analyze(ast);
	// if (analyze_result.is_error()) {
	// 	std::vector<Error> error = analyze_result.get_errors();
	// 	for (size_t i = 0; i < error.size(); i++) {
	// 		std::cout << error[i].message << '\n';
	// 	}
	// 	exit(EXIT_FAILURE);
	// }

	// if (command.type == EmitCommand && command.options[0] == std::string("--ast-with-types")) {
	// 	ast->print();
	// 	return 0;
	// }

	// if (command.type == EmitCommand && command.options[0] == std::string("--ast-with-concrete-types")) {
	// 	ast->print_with_concrete_types();
	// 	return 0;
	// }

	// if (command.type == EmitCommand && command.options[0] == std::string("--llvm-ir")) {
	// 	print_llvm_ir(ast, program_name);
	// 	return 0;
	// }

	// // Generate executable
	// generate_executable(ast, program_name);

	// if (command.type == RunCommand) {
	// 	system(utilities::get_run_command(program_name).c_str());

	// 	if (!executable_already_existed) {
	// 		remove(utilities::get_executable_name(program_name).c_str());
	// 	}
	// }

	return 0;
}

void print_usage_and_exit() {
	std::cout << errors::usage();
	exit(EXIT_FAILURE);
}

void check_usage(int argc, char *argv[]) {
	if (argc < 3) {
		print_usage_and_exit();
	}
	if (argv[1] == std::string("run") && argc < 3) {
		print_usage_and_exit();
	}
	if (argv[1] == std::string("emit") && (argc < 4 || !(argv[2] == std::string("--llvm-ir") || argv[2] == std::string("--ast") || argv[2] == std::string("--ast-with-types") || argv[2] == std::string("--ast-with-concrete-types")))) {
		print_usage_and_exit();
	}
};

Command get_command(int argc, char *argv[]) {
	if (argv[1] == std::string("build")) {
		return Command(std::string(argv[2]), BuildCommand);
	}
	if (argv[1] == std::string("run")) {
		return Command(std::string(argv[2]), RunCommand);
	}
	if (argv[1] == std::string("emit")) {
		return Command(std::string(argv[3]), EmitCommand, std::vector<std::string>{argv[2]});
	}
	assert(false);
}