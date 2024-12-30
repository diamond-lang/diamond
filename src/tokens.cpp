#include <iostream>
#include <cassert>
#include <variant>

#include "tokens.hpp"

namespace token {
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
    std::string getLiteral(token::StringLeft token) {return token.literal;}
    std::string getLiteral(token::StringMiddle token) {return token.literal;}
    std::string getLiteral(token::StringRight token) {return token.literal;}
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
}

std::string token::getLiteral(Token token) {
    return std::visit([](auto& kind) {
        return getLiteral(kind);
    }, token.kind);
}

namespace token {
    std::string toString(token::LeftParen token) {return "LeftParen";}
    std::string toString(token::RightParen token) {return "RightParen";}
    std::string toString(token::LeftBracket token) {return "LeftBracket";}
    std::string toString(token::RightBracket token) {return "RightBracket";}
    std::string toString(token::LeftCurly token) {return "LeftCurly";}
    std::string toString(token::RightCurly token) {return "RightCurly";}
    std::string toString(token::Comma token) {return "Comma";}
    std::string toString(token::Plus token) {return "Plus";}
    std::string toString(token::Slash token) {return "Slash";}
    std::string toString(token::Modulo token) {return "Modulo";}
    std::string toString(token::Star token) {return "Star";}
    std::string toString(token::Minus token) {return "Minus";}
    std::string toString(token::Colon token) {return "Colon";}
    std::string toString(token::Ampersand token) {return "Ampersand";}
    std::string toString(token::Dot token) {return "Dot";}
    std::string toString(token::Not token) {return "Not";}
    std::string toString(token::NotEqual token) {return "NotEqual";}
    std::string toString(token::Greater token) {return "Greater";}
    std::string toString(token::GreaterEqual token) {return "GreaterEqual";}
    std::string toString(token::Less token) {return "Less";}
    std::string toString(token::LessEqual token) {return "LessEqual";}
    std::string toString(token::ColonEqual token) {return "ColonEqual";}
    std::string toString(token::Equal token) {return "Equal";}
    std::string toString(token::EqualEqual token) {return "EqualEqual";}
    std::string toString(token::Be token) {return "Be";}
    std::string toString(token::Integer token) {return "Integer";}
    std::string toString(token::Float token) {return "Float";}
    std::string toString(token::Identifier token) {return "Identifier";}
    std::string toString(token::String token) {return "String";}
    std::string toString(token::StringLeft token) {return "StringLeft";}
    std::string toString(token::StringMiddle token) {return "StringMiddle";}
    std::string toString(token::StringRight token) {return "StringRight";}
    std::string toString(token::If token) {return "If";}
    std::string toString(token::Else token) {return "Else";}
    std::string toString(token::While token) {return "While";}
    std::string toString(token::Function token) {return "Function";}
    std::string toString(token::Interface token) {return "Interface";}
    std::string toString(token::Builtin token) {return "Builtin";}
    std::string toString(token::Type token) {return "Type";}
    std::string toString(token::Case token) {return "Case";}
    std::string toString(token::True token) {return "True";}
    std::string toString(token::False token) {return "False";}
    std::string toString(token::Or token) {return "Or";}
    std::string toString(token::And token) {return "And";}
    std::string toString(token::Use token) {return "Use";}
    std::string toString(token::Break token) {return "Break";}
    std::string toString(token::Continue token) {return "Continue";}
    std::string toString(token::Return token) {return "Return";}
    std::string toString(token::Mut token) {return "Mut";}
    std::string toString(token::New token) {return "New";}
    std::string toString(token::Include token) {return "Include";}
    std::string toString(token::Extern token) {return "Extern";}
    std::string toString(token::LinkWith token) {return "LinkWith";}
    std::string toString(token::NewLine token) {return "NewLine";}
    std::string toString(token::EndOfFile token) {return "EndOfFile";}
}

void token::print(std::vector<token::Token> tokens) {
    for (auto token: tokens) {
        auto literal = getLiteral(token);
        std::visit([&token, &literal](auto& kind) {
            std::cout << "Token" << "{"
                      << "kind: " <<  toString(kind)
                      << ", literal: " << literal
                      << ", line: " << token.line
                      << ", column: " << token.column
                      << "}\n";
            return;
        }, token.kind);
    }
}