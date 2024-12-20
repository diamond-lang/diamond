#ifndef TOKENS_HPP
#define TOKENS_HPP

#include <string>
#include <cstring>
#include <vector>

namespace token {
    struct LeftParen {};
    struct RightParen {};
    struct LeftBracket {};
    struct RightBracket {};
    struct LeftCurly {};
    struct RightCurly {};
    struct Comma {};
    struct Plus {};
    struct Slash {};
    struct Modulo {};
    struct Star {};
    struct Minus {};
    struct Colon {};
    struct Ampersand {};
    struct Dot {};
    struct Not {};
    struct NotEqual {};
    struct Greater {};
    struct GreaterEqual {};
    struct Less {};
    struct LessEqual {};
    struct ColonEqual {};
    struct Equal {};
    struct EqualEqual {};
    struct Be {};
    struct Integer {std::string literal;};
    struct Float {std::string literal;};
    struct Identifier {std::string literal;};
    struct String {std::string literal;};
    struct StringLeft {};
    struct StringMiddle {};
    struct StringRight {};
    struct If {};
    struct Else {};
    struct While {};
    struct Function {};
    struct Interface {};
    struct Builtin {};
    struct Type {};
    struct Case {};
    struct True {};
    struct False {};
    struct Or {};
    struct And {};
    struct Use {};
    struct Break {};
    struct Continue {};
    struct Return {};
    struct Mut {};
    struct New {};
    struct Include {};
    struct Extern {};
    struct LinkWith {};
    struct NewLine {};
    struct EndOfFile {};

    using TokenKind = std::variant<
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
        Interface,
        Builtin,
        Type,
        Case,
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
    >;

    struct Token {
        TokenKind kind;
        size_t line;
        size_t column;

        std::string get_literal();
    };

    size_t getIndex(TokenKind kind);
    void print(std::vector<Token> tokens);
};

#endif
