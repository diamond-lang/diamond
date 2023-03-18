#include <iostream>
#include <cassert>

#include "tokens.hpp"

void token::print(std::vector<Token> token) {
    for (size_t i = 0; i < token.size(); i++) {
        switch (token[i].variant) {
            case LeftParen: {
                std::cout << "token::LeftParen";
                break;
            } 
            case RightParen: {
                std::cout << "token::RightParen";
                break;
            } 
            case LeftBracket: {
                std::cout << "token::LeftBracket";
                break;
            }
            case RightBracket: {
                std::cout << "token::RightBracket";
                break;
            }
            case LeftCurly: {
                std::cout << "token::LeftCurly";
                break;
            }
            case RightCurly: {
                std::cout << "token::RightCurly";
                break;
            }
            case Comma: {
                std::cout << "token::Comma";
                break;
            } 
            case Plus: {
                std::cout << "token::Plus";
                break;
            } 
            case Slash: {
                std::cout << "token::Slash";
                break;
            }
            case Modulo: {
                std::cout << "token::Modulo";
                break;
            } 
            case Star: {
                std::cout << "token::Star";
                break;
            }
            case Minus: {
                std::cout << "token::Minus";
                break;
            } 
            case Colon: {
                std::cout << "token::Colon";
                break;
            }
            case AtSign: {
                std::cout << "token::AtSign";
                break;
            }
            case Dot: {
                std::cout << "token::Dot";
                break;
            }
            case Not: {
                std::cout << "token::Not";
                break;
            }
            case NotEqual: {
                std::cout << "token::NotEqual";
                break;
            }
            case Greater: {
                std::cout << "token::Greater";
                break;
            }
            case GreaterEqual: {
                std::cout << "token::GreaterEqual";
                break;
            }
            case Less: {
                std::cout << "token::Less";
                break;
            }
            case LessEqual: {
                std::cout << "token::LessEqual";
                break;
            }
            case Equal: {
                std::cout << "token::Equal";
                break;
            }
            case EqualEqual: {
                std::cout << "token::EqualEqual";
                break;
            }
            case Be: {
                std::cout << "token::Be";
                break;
            }
            case Integer: {
                std::cout << "token::Integer(" << token[i].get_literal() << ")";
                break;
            }
            case Float: {
                std::cout << "token::Float(" << token[i].get_literal() << ")";
                break;
            }
            case Identifier: {
                std::cout << "token::Identifier(" << token[i].get_literal() << ")";
                break;
            }
            case String: {
                std::cout << "token::String(\"" << token[i].get_literal() << "\")";
                break;
            }
            case If: {
                std::cout << "token::If";
                break;
            }
            case Else: {
                std::cout << "token::Else";
                break;
            }
            case While: {
                std::cout << "token::While";
                break;
            }
            case Function: {
                std::cout << "token::Function";
                break;
            }
            case Type: {
                std::cout << "token::Type";
                break;
            }
            case NonLocal: {
                std::cout << "token::NonLocal";
                break;
            }
            case True: {
                std::cout << "token::True";
                break;
            }
            case False: {
                std::cout << "token::False";
                break;
            }
            case Or: {
                std::cout << "token::Or";
                break;
            }
            case And: {
                std::cout << "token::And";
                break;
            }
            case Use: {
                std::cout << "token::Use";
                break;
            }
            case Break: {
                std::cout << "token::Break";
                break;
            }
            case Continue: {
                std::cout << "token::Continue";
                break;
            }
            case Return: {
                std::cout << "token::Return";
                break;
            }
            case Include: {
                std::cout << "token::Include";
                break;
            }
            case Extern: {
                std::cout << "token::Extern";
                break;
            }
            case LinkWith: {
                std::cout << "token::LinkWith";
                break;
            }
            case NewLine: {
                std::cout << "token::NewLine";
                break;
            }
            case EndOfFile: {
                std::cout << "token::EndOfFile";
                break;
            }
            default: assert(false);
        }
        std::cout << "\n";
    }
}