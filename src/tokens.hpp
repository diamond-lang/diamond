#ifndef TOKENS_HPP
#define TOKENS_HPP

#include <string>
#include <cstring>
#include <vector>

namespace token {
    enum TokenVariant {
        LeftParen,
        RightParen,
        LeftBracket,
        RightBracket,
        LeftCurly,
        RightCurly,
        Comma,
        Plus,
        Slash,
        Modulo,
        Star,
        Minus,
        Colon,
        Ampersand,
        Dot,
        Not,
        NotEqual,
        Greater,
        GreaterEqual,
        Less,
        LessEqual,
        ColonEqual,
        Equal,
        EqualEqual,
        Be,
        Integer,
        Float,
        Identifier,
        String,
        StringLeft,
        StringMiddle,
        StringRight,
        If,
        Else,
        While,
        Function,
        Type,
        True,
        False,
        Or,
        And,
        Use,
        Break,
        Continue,
        Return,
        Mut,
        New,
        Include,
        Extern,
        LinkWith,
        NewLine,
        EndOfFile
    };

    struct Token {
        token::TokenVariant variant;
        std::string str;
        const char* static_str = nullptr;
        size_t line;
        size_t column;

        Token() {}
        Token(token::TokenVariant variant, size_t line, size_t column) : variant(variant), line(line), column(column) {}
        Token(token::TokenVariant variant, std::string literal, size_t line, size_t column) : variant(variant), str(literal), line(line), column(column) {}
        Token(token::TokenVariant variant, const char* literal, size_t line, size_t column) : variant(variant), static_str(literal), line(line), column(column) {}
        ~Token() {}

        std::string get_literal() {
            if (static_str) return std::string(this->static_str);
            else            return this->str;
        }

        bool operator==(const TokenVariant variant) const {
            return this->variant == variant;
        }
        bool operator!=(const TokenVariant variant) const {
            return !(*this == variant);
        }
    };

    void print(std::vector<Token> tokens);
};

#endif
