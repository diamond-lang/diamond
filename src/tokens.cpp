#include <iostream>

#include "tokens.hpp"

void token::print(std::vector<Token> tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == String) {
            std::cout << "\"" << tokens[i].get_literal() << "\"\n";
        }
        else {
            std::cout << tokens[i].get_literal() << "\n";
        }
    }
}