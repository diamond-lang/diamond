#ifndef INTRINSICS_HPP
#define INTRINSICS_HPP

#include <vector>
#include <unordered_map>

#include "../ast.hpp"

namespace intrinsics {
    struct Type {
        ast::Type type;
        bool is_mutable = false;

        Type(ast::Type type) : type(type) {}
        Type(std::string type) : type(ast::Type(type)) {}
        Type(std::string type, bool is_mutable) : type(ast::Type(type)), is_mutable(is_mutable) {}
        Type(ast::Type type, bool is_mutable) : type(type), is_mutable(is_mutable) {}
    };

    struct Prototype {
        std::vector<Type> arguments;
        Type return_type;
    };
}

extern std::unordered_map<std::string, std::vector<intrinsics::Prototype>> intrinsic_functions;

#endif