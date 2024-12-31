#include <variant>
#include <vector>

#include "shared.hpp"
#include "tokens.hpp"

namespace lexer {
    std::variant<std::vector<token::Token>, std::vector<Error>> lex(
        std::string source
    );
};