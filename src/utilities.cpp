#include <iostream>
#include <fstream>
#include <filesystem>

#include "utilities.hpp"
#include "errors.hpp"

std::string utilities::read_file(std::filesystem::path path) {
    std::string file = "";
    std::ifstream f(path);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            file += line + '\n';
        }
        f.close();
        return file;
    }
    else {
        return "";
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

std::string utilities::get_program_name(std::filesystem::path path) {
    return path.stem().string();
}

#ifdef __linux__
    std::string utilities::get_executable_name(std::string program_name) {
        return program_name;
    }

    std::string utilities::get_run_command(std::string program_name) {
        return "./" + utilities::get_executable_name(program_name);
    }
#elif __APPLE__
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
