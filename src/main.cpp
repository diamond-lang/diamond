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
	utilities::ReadFileResult result = utilities::read_file(argv[1]);
	if (result.error) {
		std::cout << result.error_message;
		exit(EXIT_FAILURE);
	}
	std::string file = result.file;

	// Parse
	auto ast = parse::program(Source(argv[1], file.begin(), file.end()));
	ast.print();

	// Analyze
	analyze(&ast);

	// Generate executable
	//generate_executable(ast, get_executable_name(argv[1]));

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
