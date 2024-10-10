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

std::string utilities::to_str(Set<ast::Type> set) {
    std::string output = "{";
    for (size_t i = 0; i < set.elements.size(); i++) {
        output += set.elements[i].to_str();

        if (i + 1 != set.elements.size()) {
            output += ", ";
        }
    }
    output += "}";
    return output;
}

#ifdef __linux__
std::filesystem::path get_executable_path() {
    return std::filesystem::canonical("/proc/self/exe");
}

std::string utilities::get_executable_name(std::string program_name) {
    return program_name;
}

std::string utilities::get_run_command(std::string program_name) {
    return "./" + utilities::get_executable_name(program_name);
}

#elif __APPLE__
#include <mach-o/dyld.h>

std::filesystem::path get_executable_path() {
    // Get size of path
    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size);

    // Get string
    std::string path;
    path.resize(size + 1);
    _NSGetExecutablePath(&path[0], &size);

    // Return
    return std::filesystem::canonical(path);
}

std::string utilities::get_executable_name(std::string program_name) {
    return program_name;
}

std::string utilities::get_run_command(std::string program_name) {
    return "./" + utilities::get_executable_name(program_name);
}

#elif _WIN32
#include <Windows.h>

std::filesystem::path get_executable_path() {
    std::string path;
    while (true) {
        path.resize(path.size() + MAX_PATH);
        auto copied = GetModuleFileNameA(NULL, &path[0], path.size());
        if (copied + 1 < path.size()) break;
    }
    return std::filesystem::canonical(path);
}

std::string utilities::get_executable_name(std::string program_name) {
    return program_name + ".exe";
}

std::string utilities::get_run_command(std::string program_name) {
    return ".\\" + utilities::get_executable_name(program_name);
}
#endif

std::filesystem::path utilities::get_folder_of_executable() {
    return get_executable_path().parent_path();
}