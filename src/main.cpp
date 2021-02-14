#include <fstream>
#include <sstream>
#include <iostream>

#include "parser.hpp"

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

	return 0;
}
