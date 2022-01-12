#include "intrinsics.hpp"

std::unordered_map<std::string, std::vector<std::pair<std::vector<Type>, Type>>> intrinsics = {
    {"+", {
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"*", {
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"/", {
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"-", {
        {{Type("float64")}, Type("float64")},
        {{Type("int64")}, Type("int64")},
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")},
    }},
    {"<", {
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"<=", {
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {">", {
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {">=", {
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"==", {
        {{Type("bool"), Type("bool")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"!=", {
        {{Type("bool"), Type("bool")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"not", {
        {{Type("bool")}, Type("bool")}
    }},
    {"print", {
        {{Type("float64")}, Type("void")},
        {{Type("int64")}, Type("void")},
        {{Type("bool")}, Type("void")}
    }}
};