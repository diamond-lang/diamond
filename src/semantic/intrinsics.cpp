#include "intrinsics.hpp"

Set<std::string> primitive_types = Set<std::string>({
    "int8",
    "int32",
    "int64",
    "float64",
    "bool",
    "string",
    "void"
});

std::unordered_map<std::string, std::vector<intrinsics::Prototype>> intrinsic_functions = {
    {"+", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("int8")}
    }},
    {"*", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("int8")}
    }},
    {"/", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("int8")}
    }},
    {"%", {
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("int8")}
    }},
    {"-", {
        {{ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8")}, ast::Type("int8")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("int32")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("int8")}
    }},
    {"<", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {"<=", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {">", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {">=", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {"==", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {"!=", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")},
        {{ast::Type("int32"), ast::Type("int32")}, ast::Type("bool")},
        {{ast::Type("int8"), ast::Type("int8")}, ast::Type("bool")}
    }},
    {"not", {
        {{ast::Type("bool")}, ast::Type("bool")}
    }},
    {"and", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")}
    }},
    {"or", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")}
    }},
    {"print", {
        {{ast::Type("float64")}, ast::Type("void")},
        {{ast::Type("int64")}, ast::Type("void")},
        {{ast::Type("int32")}, ast::Type("void")},
        {{ast::Type("int8")}, ast::Type("void")},
        {{ast::Type("bool")}, ast::Type("void")},
        {{ast::Type("string")}, ast::Type("void")}
    }},
    {"[]", {
        {{ast::Type("array", {ast::Type(ast::FinalTypeVariable("t"))}), ast::Type("int64")}, ast::Type(ast::FinalTypeVariable("t"))},
        {{intrinsics::Type(ast::Type("array", {ast::Type(ast::FinalTypeVariable("t"))}), true), ast::Type("int64")}, intrinsics::Type(ast::Type(ast::FinalTypeVariable("t")), true)}
    }},
    {"size", {
        {{ast::Type("array", {ast::Type(ast::FinalTypeVariable("t"))})}, ast::Type("int64")},
    }}
};