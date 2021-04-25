#include "types.hpp"

namespace errors {
	std::string usage();
	std::string expecting_number(Source source);
	std::string file_couldnt_be_found(std::string path);
}
