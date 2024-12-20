#include <iostream>
#include <cassert>
#include <variant>

#include "tokens.hpp"


#define switch_all_cases(expression, scoped_switch) \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic error \"-Wswitch-enum\"") \
switch(expression) scoped_switch \
_Pragma("GCC diagnostic pop") \

void sayHi(token::Token token) {
    switch_all_cases(token.kind.index(), {
        case 0: {}
    });
}

std::string getLiteral(token::LeftParen token) {return "(";}
std::string getLiteral(token::RightParen token) {return ")";}
std::string getLiteral(token::LeftBracket token) {return "[";}
std::string getLiteral(token::RightBracket token) {return "]";}
std::string getLiteral(token::LeftCurly token) {return "{";}
std::string getLiteral(token::RightCurly token) {return "}";}
std::string getLiteral(token::Comma token) {return ",";}
std::string getLiteral(token::Plus token) {return "+";}
std::string getLiteral(token::Slash token) {return "";}
std::string getLiteral(token::Modulo token) {return "%";}
std::string getLiteral(token::Star token) {return "*";}
std::string getLiteral(token::Minus token) {return "-";}
std::string getLiteral(token::Colon token) {return ":";}
std::string getLiteral(token::Ampersand token) {return "&";}
std::string getLiteral(token::Dot token) {return ".";}
std::string getLiteral(token::Not token) {return "not";}
std::string getLiteral(token::NotEqual token) {return "!=";}
std::string getLiteral(token::Greater token) {return ">";}
std::string getLiteral(token::GreaterEqual token) {return ">=";}
std::string getLiteral(token::Less token) {return "<";}
std::string getLiteral(token::LessEqual token) {return "<=";}
std::string getLiteral(token::ColonEqual token) {return ":=";}
std::string getLiteral(token::Equal token) {return "=";}
std::string getLiteral(token::EqualEqual token) {return "==";}
std::string getLiteral(token::Be token) {return "be";}
std::string getLiteral(token::Integer token) {return token.literal;}
std::string getLiteral(token::Float token) {return token.literal;}
std::string getLiteral(token::Identifier token) {return token.literal;}
std::string getLiteral(token::String token) {return token.literal;}
std::string getLiteral(token::StringLeft token) {return "token::StringLeft";}
std::string getLiteral(token::StringMiddle token) {return "token::StringMiddle";}
std::string getLiteral(token::StringRight token) {return "token::StringRight";}
std::string getLiteral(token::If token) {return "if";}
std::string getLiteral(token::Else token) {return "else";}
std::string getLiteral(token::While token) {return "while";}
std::string getLiteral(token::Function token) {return "function";}
std::string getLiteral(token::Interface token) {return "interface";}
std::string getLiteral(token::Builtin token) {return "builtin";}
std::string getLiteral(token::Type token) {return "type";}
std::string getLiteral(token::Case token) {return "case";}
std::string getLiteral(token::True token) {return "true";}
std::string getLiteral(token::False token) {return "false";}
std::string getLiteral(token::Or token) {return "or";}
std::string getLiteral(token::And token) {return "and";}
std::string getLiteral(token::Use token) {return "use";}
std::string getLiteral(token::Break token) {return "break";}
std::string getLiteral(token::Continue token) {return "continue";}
std::string getLiteral(token::Return token) {return "return";}
std::string getLiteral(token::Mut token) {return "mut";}
std::string getLiteral(token::New token) {return "new";}
std::string getLiteral(token::Include token) {return "include";}
std::string getLiteral(token::Extern token) {return "extern";}
std::string getLiteral(token::LinkWith token) {return "linkWith";}
std::string getLiteral(token::NewLine token) {return "\\n";}
std::string getLiteral(token::EndOfFile token) {return "\\0";}

std::string token::Token::get_literal() {
    return std::visit([](auto& kind) {
        return getLiteral(kind);
    }, this->kind);
}

size_t token::getIndex(TokenKind kind) {
    return kind.index();
}

std::string toString(token::LeftParen token) {return "token::LeftParen";}
std::string toString(token::RightParen token) {return "token::RightParen";}
std::string toString(token::LeftBracket token) {return "token::LeftBracket";}
std::string toString(token::RightBracket token) {return "token::RightBracket";}
std::string toString(token::LeftCurly token) {return "token::LeftCurly";}
std::string toString(token::RightCurly token) {return "token::RightCurly";}
std::string toString(token::Comma token) {return "token::Comma";}
std::string toString(token::Plus token) {return "token::Plus";}
std::string toString(token::Slash token) {return "token::Slash";}
std::string toString(token::Modulo token) {return "token::Modulo";}
std::string toString(token::Star token) {return "token::Star";}
std::string toString(token::Minus token) {return "token::Minus";}
std::string toString(token::Colon token) {return "token::Colon";}
std::string toString(token::Ampersand token) {return "token::Ampersand";}
std::string toString(token::Dot token) {return "token::Dot";}
std::string toString(token::Not token) {return "token::Not";}
std::string toString(token::NotEqual token) {return "token::NotEqual";}
std::string toString(token::Greater token) {return "token::Greater";}
std::string toString(token::GreaterEqual token) {return "token::GreaterEqual";}
std::string toString(token::Less token) {return "token::Less";}
std::string toString(token::LessEqual token) {return "token::LessEqual";}
std::string toString(token::ColonEqual token) {return "token::ColonEqual";}
std::string toString(token::Equal token) {return "token::Equal";}
std::string toString(token::EqualEqual token) {return "token::EqualEqual";}
std::string toString(token::Be token) {return "token::Be";}
std::string toString(token::Integer token) {return "token::Integer";}
std::string toString(token::Float token) {return "token::Float";}
std::string toString(token::Identifier token) {return "token::Identifier";}
std::string toString(token::String token) {return "token::String";}
std::string toString(token::StringLeft token) {return "token::StringLeft";}
std::string toString(token::StringMiddle token) {return "token::StringMiddle";}
std::string toString(token::StringRight token) {return "token::StringRight";}
std::string toString(token::If token) {return "token::If";}
std::string toString(token::Else token) {return "token::Else";}
std::string toString(token::While token) {return "token::While";}
std::string toString(token::Function token) {return "token::Function";}
std::string toString(token::Interface token) {return "token::Interface";}
std::string toString(token::Builtin token) {return "token::Builtin";}
std::string toString(token::Type token) {return "token::Type";}
std::string toString(token::Case token) {return "token::Case";}
std::string toString(token::True token) {return "token::True";}
std::string toString(token::False token) {return "tokeb::False";}
std::string toString(token::Or token) {return "token::Or";}
std::string toString(token::And token) {return "token::And";}
std::string toString(token::Use token) {return "token::Use";}
std::string toString(token::Break token) {return "token::Break";}
std::string toString(token::Continue token) {return "token::Continue";}
std::string toString(token::Return token) {return "token::Return";}
std::string toString(token::Mut token) {return "token::Mut";}
std::string toString(token::New token) {return "token::New";}
std::string toString(token::Include token) {return "token::Include";}
std::string toString(token::Extern token) {return "token::Extern";}
std::string toString(token::LinkWith token) {return "token::LinkWith";}
std::string toString(token::NewLine token) {return "token::NewLine";}
std::string toString(token::EndOfFile token) {return "token::EndOfFile";}

void token::print(std::vector<token::Token> tokens) {
    for (auto token: tokens) {
        auto literal = token.get_literal();
        std::visit([&token, &literal](auto& kind) {
            std::cout << toString(kind) << "{"
                      << "literal: " << literal
                      << ", line: " << token.line
                      << ", column: " << token.column
                      << "}\n";
            return;
        }, token.kind);
    }
}