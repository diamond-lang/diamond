#include <iostream>
#include <cassert>

#include "errors.hpp"
#include "lexer.hpp"
#include "utilities.hpp"
#include "parser.hpp"

// Definitions and prototypes
// --------------------------
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
	std::filesystem::path file;
	CommandType type;
	std::vector<std::string> options;

	Command(std::string file, CommandType type) : file(file), type(type) {}
	Command(std::string file, CommandType type, std::vector<std::string> options) : file(file), type(type), options(options) {}
	~Command() {}
};

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
	if (argv[1] == std::string("emit") && (argc < 4 || !(argv[2] == std::string("--llvm-ir") || argv[2] == std::string("--ast") || argv[2] == std::string("--ast-with-types") || argv[2] == std::string("--ast-with-concrete-types") || argv[2] == std::string("--tokens")))) {
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

void print_errors_and_exit(std::vector<Error> errors) {
	for (size_t i = 0; i < errors.size(); i++) {
		std::cout << errors[i].value << "\n";
	}
	exit(EXIT_FAILURE);
}

void build(Command command) {
	// Get program name
	std::string program_name = utilities::get_program_name(command.file);

	// Lex
	auto lexing_result = lexer::lex(std::filesystem::path(command.file));
	if (lexing_result.is_error()) print_errors_and_exit(lexing_result.get_error());
	auto tokens = lexing_result.get_value();

	// Parse
	auto parsing_result = parse::program(tokens, command.file);
	if (parsing_result.is_error()) print_errors_and_exit(parsing_result.get_errors());
	auto ast = parsing_result.get_value();

	// // Analyze
	// auto analyze_result = semantic::analyze(ast);
	// if (analyze_result.is_error()) print_errors_and_exit(analyze_result.get_error());

	// // Generate executable
	// generate_executable(ast, program_name);
}

void run(Command command) {
	// Get program name
	std::string program_name = utilities::get_program_name(command.file);

	// Check if executable already existed
	bool already_existed = utilities::file_exists(utilities::get_executable_name(program_name));

	// Lex
	auto lexing_result = lexer::lex(std::filesystem::path(command.file));
	if (lexing_result.is_error()) print_errors_and_exit(lexing_result.get_error());
	auto tokens = lexing_result.get_value();

	// Parse
	auto parsing_result = parse::program(tokens, command.file);
	if (parsing_result.is_error()) print_errors_and_exit(parsing_result.get_errors());
	auto ast = parsing_result.get_value();

	// // Analyze
	// auto analyze_result = semantic::analyze(ast);
	// if (analyze_result.is_error()) print_errors_and_exit(analyze_result.get_error());

	// // Generate executable
	// generate_executable(ast, program_name);

	// // Run executable
	// system(utilities::get_run_command(program_name).c_str());

	// if (!already_existed) {
	// 	remove(utilities::get_executable_name(program_name).c_str());
	// }
}

void emit(Command command) {
	// Get program name
	std::string program_name = utilities::get_program_name(command.file);

	// Lex
	auto lexing_result = lexer::lex(std::filesystem::path(command.file));
	if (lexing_result.is_error()) print_errors_and_exit(lexing_result.get_error());
	auto tokens = lexing_result.get_value();

	// Emit tokens
	if (command.options[0] == std::string("--tokens")) {
		token::print(tokens);
		return;
	}

	// Parse
	auto parsing_result = parse::program(tokens, command.file);
	if (parsing_result.is_error()) print_errors_and_exit(parsing_result.get_errors());
	auto ast = parsing_result.get_value();

	// Emit AST
	if (command.options[0] == std::string("--ast")) {
		print(ast);
		return;
	}

	// // Analyze
	// auto analyze_result = semantic::analyze(ast);
	// if (analyze_result.is_error()) print_errors_and_exit(analyze_result.get_error());

	// // Emit AST with types
	// if (command.options[0] == std::string("--ast-with-types")) {
	// 	ast->print();
	// 	return;
	// }

	// // Emit AST with concrete types
	// if (command.options[0] == std::string("--ast-with-concrete-types")) {
	// 	ast->print_with_concrete_types();
	// 	return;
	// }

	// // Emit LLVM-IR
	// if (command.options[0] == std::string("--llvm-ir")) {
	// 	print_llvm_ir(ast, program_name);
	// 	return;
	// }
}

// Main
// ----
int main(int argc, char *argv[]) {
	#ifdef _WIN32
		enable_colored_text_and_unicode();
	#endif

	// Check usage
	check_usage(argc, argv);

	// Get command line arguments
	Command command = get_command(argc, argv);

	// Execute command
	if (command.type == BuildCommand) {
		build(command);
	}
	if (command.type == RunCommand) {
		run(command);
	}
	if (command.type == EmitCommand) {
		emit(command);
	}

	return 0;
}