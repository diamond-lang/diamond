#include <fstream>
#include <sstream>
#include <iostream>

#include "errors.hpp"
#include "utilities.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"

// Prototypes
// ----------
std::string get_executable_name(std::string path);


// Implementantions
// ----------------
int main(int argc, char *argv[]) {
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
	bool existed = utilities::file_exists(get_executable_name(file_path));

	// Read file
	Result<std::string, Error> result = utilities::read_file(file_path);
	if (result.is_error()) {
		std::cout << result.get_error().error_message;
		exit(EXIT_FAILURE);
	}
	std::string file = result.get_value();

	// Parse
	auto parsing_result = parse::program(Source(file_path, file.begin(), file.end()));
	if (parsing_result.is_error()) {
		std::vector<Error> error = parsing_result.get_error();
		for (size_t i = 0; i < error.size(); i++) {
			std::cout << error[i].error_message << '\n';
		}
		exit(EXIT_FAILURE);
	}
	auto ast = parsing_result.get_value();
	if (!run) ast->print();

	// Analyze
	auto analyze_result = semantic::analyze(ast);
	if (analyze_result.is_error()) {
		std::vector<Error> error = analyze_result.get_error();
		for (size_t i = 0; i < error.size(); i++) {
			std::cout << error[i].error_message << '\n';
		}
		exit(EXIT_FAILURE);
	}

	// Generate executable
	generate_executable(ast, get_executable_name(file_path));

	if (run) {
		std::string command = "./" + get_executable_name(file_path);
		system(command.c_str());

		if (!existed) {
			command = "rm " + get_executable_name(file_path);
			system(command.c_str());
		}
	}

	return 0;
}

std::string get_executable_name(std::string path) {
	size_t i = path.length() - 1;
	while (path[i] != '/' && i > 0) {
		i--;
	}
	if (i > 0) i++;
	std::string name = "";
	while (i < path.length() && path[i] != '.') {
		name += path[i];
		i++;
	}
	return name;
}
