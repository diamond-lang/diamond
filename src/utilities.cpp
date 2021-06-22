#include <iostream>
#include <fstream>

#include "utilities.hpp"
#include "errors.hpp"

Result<std::string, Error> utilities::read_file(std::string path) {
	std::string file = "";
	std::ifstream f(path);
	if (f.is_open()) {
		std::string line;
		while (std::getline(f, line)) {
			file += line + '\n';
		}
		f.close();
		return Result<std::string, Error>(file);;
	}
	else {
		return Result<std::string, Error>(Error(errors::file_couldnt_be_found(path)));
	}
}

bool utilities::file_exists(std::string path) {
	std::ifstream f(path);
	if (f.is_open()) {
		f.close();
		return true;
	}
	else {
		return false;
	}
}
