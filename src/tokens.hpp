#ifndef TOKENS_HPP
#define TOKENS_HPP

#include <string>
#include <cstring>

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
		Dot,
		Not,
		NotEqual,
		Greater,
		GreaterEqual,
		Less,
		LessEqual,
		Equal,
		EqualEqual,
		Be,
		Integer,
		Float,
		Identifier,
		String,
		If,
		Else,
		While,
		Function,
		Or,
		And,
		Newline,
		Indent,
		EndOfFile
	};

	struct Token {
		token::TokenVariant variant;
		std::string str;
		const char* static_str = nullptr;
        size_t length;

		Token(token::TokenVariant variant) : variant(variant) {} 
		Token(token::TokenVariant variant, std::string literal) : variant(variant), str(literal), length(literal.size()) {}
		Token(token::TokenVariant variant, const char* literal) : variant(variant), static_str(literal), length(strlen(literal)) {}
        Token(token::TokenVariant variant, size_t length) : variant(variant), length(length) {} 

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
};

#endif