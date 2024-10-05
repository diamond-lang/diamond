#include "intrinsics.hpp"
#include "../utilities.hpp"

Set<std::string> primitive_types = Set<std::string>({
    "Int8",
    "Int32",
    "Int64",
    "Float64",
    "Bool",
    "String",
    "None"
});

extern Set<std::filesystem::path> std_libs = Set<std::filesystem::path>({
    utilities::get_folder_of_executable() + "/std/std" + ".dmd"
});