#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "types.hpp"

namespace utilities {
	Result<std::string, Error> read_file(std::string path);
	bool file_exists(std::string path);
	std::string get_program_name(std::string path);
	std::string get_executable_name(std::string program_name);
	std::string get_run_command(std::string path);
}

#endif
