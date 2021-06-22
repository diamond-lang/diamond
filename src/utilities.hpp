#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "types.hpp"

namespace utilities {
	Result<std::string, Error> read_file(std::string path);
	bool file_exists(std::string path);
}

#endif
