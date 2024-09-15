#include "intrinsics.hpp"
#include "../utilities.hpp"

Set<std::string> primitive_types = Set<std::string>({
    "int8",
    "int32",
    "int64",
    "float64",
    "bool",
    "string",
    "void"
});

extern Set<std::filesystem::path> std_libs = Set<std::filesystem::path>({
    utilities::get_folder_of_executable() + "/std/std" + ".dmd"
});