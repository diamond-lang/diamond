#include <cctype>
#include <iostream>
#include <cassert>

#include "tokens.hpp"
#include "lexer.hpp"
#include "errors.hpp"

// Prototypes and definitions
// --------------------------
namespace lexer {
    struct Source {
        size_t line;
        size_t column;
        size_t index;
        std::filesystem::path path;
        FILE* file_pointer;
        
        Source() {}
        Source(std::filesystem::path path, FILE* file_pointer) : line(1), column(1), index(0), path(path), file_pointer(file_pointer) {} 
    };

    char current(Source source) {
        char c = fgetc(source.file_pointer);
        if (c == EOF) fseek(source.file_pointer, 0, SEEK_END);
        else          fseek(source.file_pointer, -1, SEEK_CUR);
        return c;
    }

    void advance(Source& source) {
        if (current(source) == '\n') {
            source.line += 1;
            source.column = 1;
        }
        else {
            source.column++;
        }
        source.index++;
        fgetc(source.file_pointer);
    }

    void advance(Source& source, unsigned int offset) {
        while (offset != 0) {
            advance(source);
            offset--;
        }
    }

    char prev(Source source) {
        int result = fseek(source.file_pointer, -1, SEEK_CUR);
        if (result < 0) return '\n';
        char c = current(source);
        advance(source);
        return c;
    }

    char peek(Source source, unsigned int offset = 1);
    char peek(Source source, unsigned int offset) {
        unsigned int aux = offset;
        while (aux > 0) {
            char c = fgetc(source.file_pointer);
            if (c == EOF) {
                fseek(source.file_pointer, -(int)(offset - aux), SEEK_CUR);
            }
            aux--;
        }
        char c = current(source);
        fseek(source.file_pointer, -(int)offset, SEEK_CUR);
        return c;
    }

    bool at_end(Source source) {
        return current(source) == EOF;
    }

    bool match(Source source, std::string to_match) {
        bool match = true;
        int i = 0;
        while (!at_end(source) && i < to_match.size()) {
            if (current(source) != to_match[i]) {
                match = false;
                break;
            }
            advance(source);
            i++;
        }
        if (at_end(source) && i != to_match.size()) {
            match = false;
        }
        fseek(source.file_pointer, -i, SEEK_CUR);
        return match;
    }

    Result<token::Token, Error> get_token(Source& source);
    Result<token::Token, Error> advance(token::Token token, Source& source, unsigned int offset);
    void advance_until_new_line(Source& source);
    Result<token::Token, Error> get_string(Source& source);
    Result<token::Token, Error> get_number(Source& source);
    Result<token::Token, Error> get_identifier(Source& source);
    std::string get_integer(Source& source);
    Result<token::Token, Error> get_indent(Source& source);
}

// Lexing
// ------
Result<std::vector<token::Token>, Errors> lexer::lex(std::filesystem::path path) {
     std::vector<token::Token> tokens;
     Errors errors;

    // Read file
    FILE* file_pointer = fopen(path.c_str(), "r");
    if (!file_pointer) {
        errors.push_back(errors::file_couldnt_be_found(path));
        return Result<std::vector<token::Token>, Errors>(errors);
    }

    // Create source
    Source source(path, file_pointer);
   
    // Tokenize 
    while (!at_end(source)) {
        auto result = get_token(source);
        if (result.is_ok() && result.get_value() != token::EndOfFile) {
            tokens.push_back(result.get_value());
        }
        else if (result.is_error()) {
            errors.push_back(result.get_error());
        }
    }

    // Close file
    fclose(file_pointer);

    if (errors.size() != 0) return Result<std::vector<token::Token>, Errors>(errors);
    else                    return Result<std::vector<token::Token>, Errors>(tokens);
}

Result<token::Token, Error> lexer::get_token(Source& source) {
    if (at_end(source))      return advance(token::Token(token::EndOfFile, "EndOfFile", source.line, source.column), source, 1);
    if (match(source, "("))  return advance(token::Token(token::LeftParen, "(", source.line, source.column), source, 1);
    if (match(source, ")"))  return advance(token::Token(token::RightParen, ")", source.line, source.column), source, 1);
    if (match(source, "["))  return advance(token::Token(token::LeftBracket, "[", source.line, source.column), source, 1);
    if (match(source, "]"))  return advance(token::Token(token::RightBracket, "]", source.line, source.column), source, 1);
    if (match(source, "{"))  return advance(token::Token(token::LeftCurly, "{", source.line, source.column), source, 1);
    if (match(source, "}"))  return advance(token::Token(token::RightCurly, "}", source.line, source.column), source, 1);
    if (match(source, "+"))  return advance(token::Token(token::Plus, "+", source.line, source.column), source, 1);
    if (match(source, "*"))  return advance(token::Token(token::Star, "*", source.line, source.column), source, 1);
    if (match(source, "/"))  return advance(token::Token(token::Slash, "/", source.line, source.column), source, 1);
    if (match(source, "%"))  return advance(token::Token(token::Modulo, "%", source.line, source.column), source, 1);
    if (match(source, ":"))  return advance(token::Token(token::Colon, ":", source.line, source.column), source, 1);
    if (match(source, ","))  return advance(token::Token(token::Comma, ",", source.line, source.column), source, 1);
    if (match(source, "!=")) return advance(token::Token(token::NotEqual, "!=", source.line, source.column), source, 2);
    if (match(source, "==")) return advance(token::Token(token::EqualEqual, "==", source.line, source.column), source, 2);
    if (match(source, "="))  return advance(token::Token(token::Equal, "=", source.line, source.column), source, 1);
    if (match(source, ">=")) return advance(token::Token(token::GreaterEqual, ">=", source.line, source.column), source, 2);
    if (match(source, ">"))  return advance(token::Token(token::Greater, ">", source.line, source.column), source, 1);
    if (match(source, "<=")) return advance(token::Token(token::LessEqual, "<=", source.line, source.column), source, 2);
    if (match(source, "<"))  return advance(token::Token(token::Less, "<", source.line, source.column), source, 1);
    if (match(source, ".") && isdigit(peek(source))) {
        return get_number(source);
    }
    if (match(source, ".")) {
        return advance(token::Token(token::Dot, ".", source.line, source.column), source, 1);
    }
    if (match(source, "---")) {
        advance(source, 3);
        while (!(at_end(source) || match(source, "---"))) {
            advance(source);
        }
        if (at_end(source)) {
            return Result<token::Token, Error>(std::string("Error: Unclosed block comment\n"));
        }
     
        advance(source, 3);
        return get_token(source);
    }
    if (match(source, "--")) {
        advance_until_new_line(source);
        return get_token(source);
    }
    if (match(source, "-")) {
        return advance(token::Token(token::Less, "-", source.line, source.column), source, 1);
    }
    if (match(source, " "))  {
        advance(source);
        return get_token(source);
    }
    if (match(source, "\t")) {
        advance(source);
        return get_token(source);
    }
    if (match(source, "\n"))      return advance(token::Token(token::NewLine, "\\n", source.line, source.column), source, 1);
    if (match(source, "\""))      return get_string(source);
    if (isdigit(current(source))) return get_number(source);
    if (isalpha(current(source))) return get_identifier(source);

    std::cout << current(source) << "\n";
    return Result<token::Token, Error>(std::string("Error: Unrecognized symbol\n"));
}

Result<token::Token, Error> lexer::advance(token::Token token, Source& source, unsigned int offset) {
    advance(source, offset);
    return Result<token::Token, Error>(token);
}

void lexer::advance_until_new_line(Source& source) {
    while (!at_end(source) && current(source) != '\n') {
        advance(source);
    }
    if (!at_end(source)) advance(source);
}

Result<token::Token, Error> lexer::get_string(Source& source) {
    std::string literal = "";
    size_t line = source.line;
    size_t column = source.column;

    advance(source);
    while (!(at_end(source) || match(source, "\n") || match(source, "\""))) {
        literal += current(source);
        advance(source);
    }

    if (at_end(source) || match(source, "\n")) {
        return Result<token::Token, Error>(std::string("Error: Unclosed string\n"));
    }
    else {
        advance(source);
        return Result<token::Token, Error>(token::Token(token::String, literal, line, column));
    }
}

Result<token::Token, Error> lexer::get_identifier(Source& source) {
    std::string literal = "";
    size_t line = source.line;
    size_t column = source.column;

    while (!(at_end(source) || match(source, "\n"))) {
        if (!(isalnum(current(source)) || current(source) == '_')) break;
        literal += current(source);
        advance(source);
    }

    if (literal == "if")       return token::Token(token::If, "if", line, column);
    if (literal == "else")     return token::Token(token::Else, "else", line, column);
    if (literal == "while")    return token::Token(token::While, "while", line, column);
    if (literal == "function") return token::Token(token::Function, "function", line, column);
    if (literal == "be")       return token::Token(token::Be, "be", line, column);
    if (literal == "nonlocal") return token::Token(token::NonLocal, "nonlocal", line, column);
    if (literal == "true")     return token::Token(token::True, "true", line, column);
    if (literal == "false")    return token::Token(token::False, "false", line, column);
    if (literal == "and")      return token::Token(token::And, "and", line, column);
    if (literal == "or")       return token::Token(token::Or, "or", line, column);
    if (literal == "use")      return token::Token(token::Use, "use", line, column);
    if (literal == "include")  return token::Token(token::Include, "include", line, column);
    if (literal == "break")    return token::Token(token::Break, "break", line, column);
    if (literal == "continue") return token::Token(token::Continue, "continue", line, column);
    if (literal == "return")   return token::Token(token::Return, "return", line, column);
    if (literal == "not")      return token::Token(token::Not, "not", line, column);

    return token::Token(token::Identifier, literal, line, column);
}

Result<token::Token, Error> lexer::get_number(Source& source) {
    std::string literal = "";
    size_t line = source.line;
    size_t column = source.column;
    
    // eg: .8
    if (match(source, ".")) {
        advance(source);
        literal += "." + get_integer(source);
        return token::Token(token::Float, literal, line, column);
    }

    literal += get_integer(source);

    // eg: 6.7
    if (match(source, ".")) {
        advance(source);
        literal += "." + get_integer(source);
        return token::Token(token::Float, literal, line, column);
    
    // eg: 16
    } else {
        return token::Token(token::Integer, literal, line, column);
    }
}

std::string lexer::get_integer(Source& source) {
    std::string integer = "";
    while (!(at_end(source) || match(source, "\n")) && isdigit(current(source))) {
        integer += current(source);
        advance(source);
    }
    return integer;
}

void lexer::print(std::vector<token::Token> tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == token::String) {
            std::cout << tokens[i].get_literal() << "\n";
        }
        else {
            std::cout << "\"" << tokens[i].get_literal() << "\"\n";
        }
    }
}