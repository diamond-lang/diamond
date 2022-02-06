#include <iostream>

#include "lexer.hpp"
#include "errors.hpp"

Result<std::vector<token::Token>, Error> lexer::lex(std::filesystem::path path) {
     std::vector<token::Token> tokens;

    // Read file
    FILE* file_pointer = fopen(path.c_str(), "r");
    if (!file_pointer) {
        return Result<std::vector<token::Token>, Error>(errors::file_couldnt_be_found(path));
    }

    // Create source
    Source source(path, file_pointer);
   
    // Tokenize 
    while (!at_end(source)) {
       char c = current(source);
       std::cout << c;
       advance(source);
    }

    // Close file
    fclose(file_pointer);

    return Result<std::vector<token::Token>, Error>(tokens);
}