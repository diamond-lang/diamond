#include <vector>
#include <filesystem>

#include "shared.hpp"
#include "errors.hpp"
#include "tokens.hpp"

namespace lexer {
    Result<std::vector<token::Token>, Errors> lex(std::filesystem::path path); 
};