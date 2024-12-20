#include "tokens.hpp"
#include "lexer.hpp"
#include "errors.hpp"

#include <variant>

namespace lexer {
    struct Lexer {
        size_t line = 1;
        size_t column = 1;
        size_t start = 0;
        size_t current = 0;
        std::string source;
        std::vector<token::Token> tokens;
        std::vector<Error> errors;
    };

    bool atEnd(Lexer& lexer);
    void advance(Lexer& lexer);
    bool match(Lexer& lexer, std::string string);
    char peek(Lexer& lexer);
    char peekNext(Lexer& lexer);
    void advanceUntilNewLine(Lexer& lexer);
    void scanToken(Lexer& lexer);
    void scanNumber(Lexer& lexer);
    void scanInteger(Lexer& lexer);
    void scanIdentifierOrKeyword(Lexer& lexer);
    void scanString(Lexer& lexer);
    void addToken(Lexer& lexer, token::TokenKind kind);
}

// Lexing
// ------
std::variant<std::vector<token::Token>, std::vector<Error>> lexer::lex(std::string source) {
    // Create lexer
    Lexer lexer = Lexer {
        .source = source
    };

    // Tokenize
    while (!atEnd(lexer)) {
        scanToken(lexer);
    }

    lexer.tokens.push_back(token::Token{
        .kind = token::EndOfFile{},
        .line = lexer.line,
        .column = lexer.column
    });

    if (lexer.errors.size() > 0) {
        return lexer.errors;
    }
    return lexer.tokens;
}

void lexer::scanToken(Lexer& lexer) {
    lexer.start = lexer.current;

    if (match(lexer, "("))  return addToken(lexer, token::LeftParen{});
    if (match(lexer, ")"))  return addToken(lexer, token::RightParen{});
    if (match(lexer, "["))  return addToken(lexer, token::LeftBracket{});
    if (match(lexer, "]"))  return addToken(lexer, token::RightBracket{});
    if (match(lexer, "{"))  {
        // if (!source.onString.empty()) {
        //     source.onString.push(OnString{'{', false});
        // }
        return addToken(lexer, token::LeftCurly{});
    }
    if (match(lexer, "}")) {
        // if (!source.onString.empty()
        // && source.onString.top().character == '{') {
        //     if (source.onString.top().onString) {
        //         return get_string(source);
        //     }
        //     else {
        //         source.onString.pop();
        //         return Token(lexer, token::RightCurly, "}", source.line, source.column), lexer, 1);
        //     }
        // }

        return addToken(lexer, token::RightCurly{});
    }
    if (match(lexer, "+"))  return addToken(lexer, token::Plus{});
    if (match(lexer, "*"))  return addToken(lexer, token::Star{});
    if (match(lexer, "/"))  return addToken(lexer, token::Slash{});
    if (match(lexer, "%"))  return addToken(lexer, token::Modulo{});
    if (match(lexer, ":=")) return addToken(lexer, token::ColonEqual{});
    if (match(lexer, ":"))  return addToken(lexer, token::Colon{});
    if (match(lexer, ","))  return addToken(lexer, token::Comma{});
    if (match(lexer, "!=")) return addToken(lexer, token::NotEqual{});
    if (match(lexer, "==")) return addToken(lexer, token::EqualEqual{});
    if (match(lexer, "="))  return addToken(lexer, token::Equal{});
    if (match(lexer, ">=")) return addToken(lexer, token::GreaterEqual{});
    if (match(lexer, ">"))  return addToken(lexer, token::Greater{});
    if (match(lexer, "<=")) return addToken(lexer, token::LessEqual{});
    if (match(lexer, "<"))  return addToken(lexer, token::Less{});
    if (match(lexer, "&"))  return addToken(lexer, token::Ampersand{});
    if (peek(lexer) == '.' && isdigit(peekNext(lexer))) {
        return scanNumber(lexer);
    }
    if (match(lexer, ".")) {
        return addToken(lexer, token::Dot{});
    }
    if (match(lexer, "---")) {
        while (!(atEnd(lexer) || match(lexer, "---"))) {
            advance(lexer);
        }
        if (atEnd(lexer)) {
            lexer.errors.push_back(Error{std::string("Error: Unclosed block comment\n")});
            return;
        }
        return scanToken(lexer);
    }
    if (match(lexer, "--")) {
        advanceUntilNewLine(lexer);
        advance(lexer);
        return scanToken(lexer);
    }
    if (match(lexer, "-")) {
        return addToken(lexer, token::Minus{});
    }
    if (match(lexer, "_")) {
        return scanIdentifierOrKeyword(lexer);
    }
    if (match(lexer, " ")
    ||  match(lexer, "\t")) {
        return; 
    }
    if (match(lexer, "\r\n")
    ||  match(lexer, "\n")) {
        lexer.line += 1;
        lexer.column = 1;
        return addToken(lexer, token::NewLine{});
    }
    if (match(lexer, "\"")) {
        return scanString(lexer);
    }
    if (isdigit(peek(lexer))) return scanNumber(lexer);
    if (isalpha(peek(lexer))) return scanIdentifierOrKeyword(lexer);

    lexer.errors.push_back(Error{std::string("Error: Unrecognized character \"")});
}

void lexer::scanString(Lexer& lexer) {
    std::string literal = "";
    size_t line = lexer.line;
    size_t column = lexer.column;
    bool isRight = false;

    // if (peek(lexer) == '\"') {
    //     source.onString.push(OnString{current(source), true});
    // }
    // else if (current(source) == '}') {
    //     source.onString.pop();
    //     isRight = true;
    // }
    // else {
    //     assert(false);
    // }

    advance(lexer);
    while (!(atEnd(lexer) || match(lexer, "\n"))) {
        if (match(lexer, "\\n")) {
            literal += "\n";
        }
        else if (match(lexer, "\\\"")) {
            literal += "\"";
        }
        else if (match(lexer, "\"")) {
            break;
        }
        else if (match(lexer, "\\{")) {
            literal += "{";
        }
        else if (match(lexer, "{")) {
            break;
        }
        else {
            literal += peek(lexer);
            advance(lexer);
        }
    }

    if (atEnd(lexer) || match(lexer, "\n")) {
        lexer.errors.push_back(Error{std::string("Error: Unclosed string\n")});
        return;
    }
    else {
        // if (match(source, "{")) {
        //     source.onString.push(OnString{current(source), true});
        //     advance(source);
        //     if (isRight) {
        //         return Result<token::Token, Error>(token::Token(token::StringMiddle, literal, line, column));
        //     }
        //     else {
        //         return Result<token::Token, Error>(token::Token(token::StringLeft, literal, line, column));
        //     }
        // }
        /*else*/ if (match(lexer, "\"")) {
            //source.onString.pop();
            // if (isRight) {
            //     return Result<token::Token, Error>(token::Token(token::StringRight, literal, line, column));
            // }
            // else {
            return addToken(lexer, token::String{.literal = literal});
            //}
        }
        else {
            assert(false);
        }
    }
}

void lexer::scanIdentifierOrKeyword(Lexer& lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);
    auto literal = lexer.source.substr(lexer.start, lexer.current - lexer.start);

    if (literal == "if")        return addToken(lexer, token::If{});
    if (literal == "else")      return addToken(lexer, token::Else{});
    if (literal == "while")     return addToken(lexer, token::While{});
    if (literal == "function")  return addToken(lexer, token::Function{});
    if (literal == "interface") return addToken(lexer, token::Interface{});
    if (literal == "builtin")   return addToken(lexer, token::Builtin{});
    if (literal == "type")      return addToken(lexer, token::Type{});
    if (literal == "case")      return addToken(lexer, token::Case{});
    if (literal == "be")        return addToken(lexer, token::Be{});
    if (literal == "true")      return addToken(lexer, token::True{});
    if (literal == "false")     return addToken(lexer, token::False{});
    if (literal == "and")       return addToken(lexer, token::And{});
    if (literal == "or")        return addToken(lexer, token::Or{});
    if (literal == "use")       return addToken(lexer, token::Use{});
    if (literal == "include")   return addToken(lexer, token::Include{});
    if (literal == "break")     return addToken(lexer, token::Break{});
    if (literal == "continue")  return addToken(lexer, token::Continue{});
    if (literal == "return")    return addToken(lexer, token::Return{});
    if (literal == "mut")       return addToken(lexer, token::Mut{});
    if (literal == "new")       return addToken(lexer, token::New{});
    if (literal == "not")       return addToken(lexer, token::Not{});
    if (literal == "extern")    return addToken(lexer, token::Extern{});
    if (literal == "link_with") return addToken(lexer, token::LinkWith{});

    return addToken(lexer, token::Identifier{.literal = literal});
}

void lexer::scanNumber(Lexer& lexer) {
    // consume digits
    while (isdigit(peek(lexer))) advance(lexer);

    // eg: .8
    if (peek(lexer) == '.' && isdigit(peekNext(lexer))) {
        // consume "."
        advance(lexer);

        // consume digits
        while (isdigit(peek(lexer))) advance(lexer);
    }

    // Add token
    auto literal = lexer.source.substr(lexer.start, lexer.current - lexer.start);
    return addToken(lexer, token::Float{.literal = literal});
}

void lexer::scanInteger(Lexer& lexer) {
    while (isdigit(peek(lexer))) advance(lexer);
    auto literal = lexer.source.substr(lexer.start, lexer.current - lexer.start);
    return addToken(lexer, token::Integer{.literal = literal});
}

bool lexer::atEnd(Lexer& lexer) {
    return lexer.current >= lexer.source.length();
}

void lexer::advance(Lexer& lexer) {
    lexer.current += 1;
    lexer.column += 1;
}

bool lexer::match(Lexer& lexer, std::string string) {
    bool match = true;
    int i = 0;
    while (!atEnd(lexer) && i < string.size()) {
        if (peek(lexer) != string[i]) {
            match = false;
            lexer.current -= i;
            break;
        }
        advance(lexer);
        i++;
    }
    if (atEnd(lexer) && i != string.size()) {
        match = false;
        lexer.current -= i;
    }

    return match;
}

char lexer::peek(Lexer& lexer) {
    if (atEnd(lexer)) return '\0';
    return lexer.source[lexer.current];
}

char lexer::peekNext(Lexer& lexer) {
    if (lexer.current + 1 >= lexer.source.length()) return '\0';
    return lexer.source[lexer.current + 1];
}

void lexer::advanceUntilNewLine(Lexer& lexer) {
    while (peek(lexer) != '\n') {advance(lexer);}
}

void lexer::addToken(Lexer& lexer, token::TokenKind kind) {
    lexer.tokens.push_back(token::Token{
        .kind = kind,
        .line = lexer.line,
        .column = lexer.column - (lexer.current - lexer.start)
    });
}
