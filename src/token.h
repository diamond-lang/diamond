#ifndef token_h
#define token_h

#include <stdlib.h>

#include "types.h"

// Token
typedef enum {
    TokenLeftParen,
    TokenRightParen,
    TokenLeftBracket,
    TokenRightBracket,
    TokenLeftCurly,
    TokenRightCurly,
    TokenComma,
    TokenPlus,
    TokenSlash,
    TokenModulo,
    TokenStar,
    TokenMinus,
    TokenColon,
    TokenAmpersand,
    TokenDot,
    TokenNot,
    TokenNotEqual,
    TokenGreater,
    TokenGreaterEqual,
    TokenLess,
    TokenLessEqual,
    TokenColonEqual,
    TokenEqual,
    TokenEqualEqual,
    TokenBe,
    TokenInteger,
    TokenFloat,
    TokenIdentifier,
    TokenString,
    TokenStringLeft,
    TokenStringMiddle,
    TokenStringRight,
    TokenIf,
    TokenElse,
    TokenWhile,
    TokenFunction,
    TokenInterface,
    TokenBuiltin,
    TokenType,
    TokenCase,
    TokenTrue,
    TokenFalse,
    TokenOr,
    TokenAnd,
    TokenUse,
    TokenBreak,
    TokenContinue,
    TokenReturn,
    TokenMut,
    TokenNew,
    TokenInclude,
    TokenExtern,
    TokenLinkWith,
    TokenNewLine,
    TokenEndOfFile
} TokenKind;

typedef struct {
    TokenKind kind;
    String literal;
    size_t line;
    size_t column;
} Token;

typedef ListType(Token) TokenList;

void token_print(TokenList tokens);

#endif