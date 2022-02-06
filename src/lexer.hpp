#include "types.hpp"

namespace lexer {
    Result<std::vector<token::Token>, Error> lex(std::filesystem::path path); 
};