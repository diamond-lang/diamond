#ifndef INTRINSICS_HPP
#define INTRINSICS_HPP

#include "types.hpp"
#include <unordered_map>

extern std::unordered_map<std::string, std::pair<std::vector<Type>, Type>> interfaces;
extern std::unordered_map<std::string, std::vector<std::pair<std::vector<Type>, Type>>> intrinsics;

#endif