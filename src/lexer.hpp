#include <vector>
#include <variant>

#include "shared.hpp"
#include "errors.hpp"
#include "tokens.hpp"

namespace lexer {
    std::variant<std::vector<token::Token>, std::vector<Error>> lex(std::string source);
};