#ifndef INTRINSICS_HPP
#define INTRINSICS_HPP

#include <vector>
#include <unordered_map>

#include "../ast.hpp"

extern std::unordered_map<std::string, std::vector<std::pair<std::vector<ast::Type>, ast::Type>>> intrinsic_functions;

#endif