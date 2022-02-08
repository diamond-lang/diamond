#include <cctype>
#include <iostream>
#include <cassert>

#include "tokens.hpp"
#include "lexer.hpp"
#include "errors.hpp"

// Prototypes and definitions
// --------------------------
struct Source {
	size_t line;
	size_t col;
	size_t index;
	std::filesystem::path path;
	FILE* file_pointer;
	
	Source() {}
	Source(std::filesystem::path path, FILE* file_pointer) : line(1), col(1), index(0), path(path), file_pointer(file_pointer) {} 
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
		source.col = 1;
	}
	else {
		source.col++;
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
        if (result.is_ok()) tokens.push_back(result.get_value());
        else                errors.push_back(result.get_error());
    }

    // Close file
    fclose(file_pointer);

    if (errors.size() != 0) return Result<std::vector<token::Token>, Errors>(errors);
    else                    return Result<std::vector<token::Token>, Errors>(tokens);
}

Result<token::Token, Error> get_token(Source& source) {
    if (at_end(source))      return advance(token::Token(token::EndOfFile), source, 1);
    if (match(source, "("))  return advance(token::Token(token::LeftParen, "("), source, 1);
    if (match(source, ")"))  return advance(token::Token(token::RightParen, ")"), source, 1);
    if (match(source, "["))  return advance(token::Token(token::LeftBracket, "["), source, 1);
    if (match(source, "]"))  return advance(token::Token(token::RightBracket, "]"), source, 1);
    if (match(source, "{"))  return advance(token::Token(token::LeftCurly, "{"), source, 1);
    if (match(source, "}"))  return advance(token::Token(token::RightCurly, "}"), source, 1);
    if (match(source, "+"))  return advance(token::Token(token::Plus, "+"), source, 1);
    if (match(source, "*"))  return advance(token::Token(token::Star, "*"), source, 1);
    if (match(source, "/"))  return advance(token::Token(token::Slash, "/"), source, 1);
    if (match(source, "%"))  return advance(token::Token(token::Modulo, "%"), source, 1);
    if (match(source, ":"))  return advance(token::Token(token::Colon, ":"), source, 1);
    if (match(source, ","))  return advance(token::Token(token::Comma, ","), source, 1);
    if (match(source, "!=")) return advance(token::Token(token::NotEqual, "!="), source, 2);
    if (match(source, "==")) return advance(token::Token(token::EqualEqual, "=="), source, 2);
    if (match(source, "="))  return advance(token::Token(token::Equal, "="), source, 1);
    if (match(source, ">=")) return advance(token::Token(token::GreaterEqual, ">="), source, 2);
    if (match(source, ">"))  return advance(token::Token(token::Greater, ">"), source, 1);
    if (match(source, "<=")) return advance(token::Token(token::LessEqual, "<="), source, 2);
    if (match(source, "<"))  return advance(token::Token(token::Less, "<"), source, 1);
    if (match(source, ".") && isdigit(peek(source))) {
        return get_number(source);
    }
    if (match(source, ".")) {
        return advance(token::Token(token::Dot, "."), source, 1);
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
        return advance(token::Token(token::Less), source, 1);
    }
    if (match(source, " "))  {
        advance(source);
        return get_token(source);
    }
    if (match(source, "\t")) {
        advance(source);
        return get_token(source);
    }
    if (match(source, "\n"))      return advance(token::Token(token::Newline, "\\n"), source, 1);
    if (match(source, "\""))      return get_string(source);
    if (isdigit(current(source))) return get_number(source);
    if (isalpha(current(source))) return get_identifier(source);

    std::cout << current(source) << "\n";
    return Result<token::Token, Error>(std::string("Error: Unrecognized symbol\n"));
}

Result<token::Token, Error> advance(token::Token token, Source& source, unsigned int offset) {
    advance(source, offset);
    return Result<token::Token, Error>(token);
}

void advance_until_new_line(Source& source) {
    while (current(source) != '\n') {
        advance(source);
    }
    advance(source);
}

Result<token::Token, Error> get_string(Source& source) {
    std::string literal = "\"";
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
        literal += "\"";
        return Result<token::Token, Error>(token::Token(token::String, literal));
    }
}

Result<token::Token, Error> get_identifier(Source& source) {
    std::string literal = "";

    while (!(at_end(source) || match(source, "\n"))) {
        if (!(isalnum(current(source)) || current(source) == '_')) break;
        literal += current(source);
        advance(source);
    }

    if (literal == "if")       return token::Token(token::If, "if");
    if (literal == "else")     return token::Token(token::Else, "else");
    if (literal == "while")    return token::Token(token::While, "while");
    if (literal == "function") return token::Token(token::Function, "function");
    if (literal == "be")       return token::Token(token::Be, "be");

    return token::Token(token::Identifier, literal);
}

Result<token::Token, Error> get_number(Source& source) {
    std::string literal = "";
    
    // eg: .8
    if (match(source, ".")) {
        advance(source);
        literal += "." + get_integer(source);
        return token::Token(token::Float, literal);
    }

    literal += get_integer(source);

    // eg: 6.7
    if (match(source, ".")) {
        advance(source);
        literal += "." + get_integer(source);
        return token::Token(token::Float, literal);
    
    // eg: 16
    } else {
        return token::Token(token::Integer, literal);
    }
}

std::string get_integer(Source& source) {
    std::string integer = "";
    while (!(at_end(source) || match(source, "\n")) && isdigit(current(source))) {
        integer += current(source);
        advance(source);
    }
    return integer;
}