#include <iostream>
#include <fstream>
#include <filesystem>

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

std::string utilities::get_program_name(std::string path) {
	return std::filesystem::path(path).stem().string();
}

#ifdef __linux__
	std::string utilities::get_executable_name(std::string program_name) {
		return program_name;
	}

	std::string utilities::get_run_command(std::string program_name) {
		return "./" + utilities::get_executable_name(program_name);
	}

#elif _WIN32
	std::string utilities::get_executable_name(std::string program_name) {
		return program_name + ".exe";
	}

	std::string utilities::get_run_command(std::string program_name) {
		return ".\\" + utilities::get_executable_name(program_name);
	}
#endif
