#include <fstream>
#include <sstream>
#include <iostream>

#include "parser.hpp"
#include "codegen.hpp"

// Prototypes
// ----------
std::string get_executable_name(std::string path);


// Implementantions
// ----------------
int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage:\n");
		printf("    diamond source_file_path\n");
		exit(EXIT_FAILURE);
	}

	// Read file
	std::ifstream in;
	in.open(argv[1]);
	std::stringstream stream;
	stream << in.rdbuf();
	std::string source_file = stream.str();

	// Parse
	auto ast = parse::program(Source(argv[1], source_file.begin(), source_file.end()));
	ast.print();

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
