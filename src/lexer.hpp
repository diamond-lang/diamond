#include "types.hpp"

namespace lexer {
    Result<std::vector<token::Token>, Errors> lex(std::filesystem::path path); 
};