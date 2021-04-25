#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <string>

namespace utilities {
	struct ReadFileResult {
		std::string file;
		bool error = false;
		std::string error_message;

		ReadFileResult(std::string file) : file(file) {}
		ReadFileResult(bool error, std::string error_message) : error(error), error_message(error_message) {}
	};

	ReadFileResult read_file(std::string path);
}

#endif
