#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <filesystem>
#include <string>

#include "shared.hpp"
#include "errors.hpp"

namespace utilities {
    std::string read_file(std::filesystem::path path);
    bool file_exists(std::string path);
    std::string get_program_name(std::filesystem::path path);
    std::string get_executable_name(std::string program_name);
    std::string get_run_command(std::string path);
    std::string to_str(Set<ast::Type> set);
    std::string get_program_name();
    std::string get_folder_of_executable();
}

#endif
