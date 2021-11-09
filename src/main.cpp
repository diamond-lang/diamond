#include <fstream>
#include <sstream>
#include <iostream>

#include "errors.hpp"
#include "utilities.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

#ifdef _WIN32
#include <Windows.h>

void enable_colored_text() {
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(handle, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
#endif

int main(int argc, char *argv[]) {
	#ifdef _WIN32
		enable_colored_text();
	#endif

	// Get command line arguments
	if (argc < 2 || (argv[1] == std::string("run") && argc < 3)) {
		// Print usage
		std::cout << errors::usage();
		exit(EXIT_FAILURE);
	}
	bool run = false;
	std::string file_path;
	if (argv[1] == std::string("run")) {
		run = true;
		file_path = argv[2];
	} else {
		file_path = argv[1];
	}
	std::string program_name = utilities::get_program_name(file_path);
	bool executable_already_existed = utilities::file_exists(utilities::get_executable_name(program_name));

	// Read file
	Result<std::string, Error> result = utilities::read_file(file_path);
	if (result.is_error()) {
		std::cout << result.get_error().message;
		exit(EXIT_FAILURE);
	}
	std::string file = result.get_value();

	// Parse
	auto parsing_result = parse::program(Source(file_path, file.begin(), file.end()));
	if (parsing_result.is_error()) {
		std::vector<Error> errors = parsing_result.get_errors();
		for (size_t i = 0; i < errors.size(); i++) {
			std::cout << errors[i].message << '\n';
		}
		exit(EXIT_FAILURE);
	}
	auto ast = parsing_result.get_value();
	if (!run) ast->print();

	// Analyze
	auto analyze_result = semantic::analyze(ast);
	if (analyze_result.is_error()) {
		std::vector<Error> error = analyze_result.get_errors();
		for (size_t i = 0; i < error.size(); i++) {
			std::cout << error[i].message << '\n';
		}
		exit(EXIT_FAILURE);
	}

	// Generate executable
	generate_executable(ast, program_name);

	if (run) {
		system(utilities::get_run_command(program_name).c_str());

		if (!executable_already_existed) {
			remove(utilities::get_executable_name(program_name).c_str());
		}
	}

	return 0;
}
