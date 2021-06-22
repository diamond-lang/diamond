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
	if (argc < 2) {
		// Print usage
		std::cout << errors::usage();
		exit(EXIT_FAILURE);
	}

	// Read file
	Result<std::string, Error> result = utilities::read_file(argv[1]);
	if (result.is_error()) {
		std::cout << result.get_error().error_message;
		exit(EXIT_FAILURE);
	}
	std::string file = result.get_value();

	// Parse
	auto parsing_result = parse::program(Source(argv[1], file.begin(), file.end()));
	if (parsing_result.is_error()) {
		std::vector<Error> error = parsing_result.get_error();
		for (size_t i = 0; i < error.size(); i++) {
			std::cout << error[i].error_message << '\n';
		}
		exit(EXIT_FAILURE);
	}
	Ast::Program* ast = parsing_result.get_value();
	ast->print();

	delete ast;
	return 0;

	// Analyze
	analyze(ast);

	// Generate executable
	generate_executable(ast, get_executable_name(argv[1]));

	delete ast;
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
