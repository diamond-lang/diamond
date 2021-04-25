#include <iostream>
#include <fstream>

#include "utilities.hpp"
#include "errors.hpp"

utilities::ReadFileResult utilities::read_file(std::string path) {
	std::string file = "";
	std::ifstream f(path);
	if (f.is_open()) {
		std::string line;
		while (std::getline(f, line)) {
			file += line + '\n';
		}
		f.close();
		return utilities::ReadFileResult(file);;
	}
	else {
		return utilities::ReadFileResult(true, errors::file_couldnt_be_found(path));
	}
}
