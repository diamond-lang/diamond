#include "token.h"

#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "types.h"

int numberOfDigits(size_t number) {
    assert(number > 0);
    int numberOfDigits = 0;
    while (number > 0) {
        number /= 10;
        numberOfDigits += 1;
    }
    return numberOfDigits;
}

void token_print(TokenList tokens) {
    size_t maxColumn = 0;
    for (size_t i = 0; i < tokens.count; i++) {
        if (tokens.items[i].column > maxColumn) {
            maxColumn = tokens.items[i].column;
        }
    }
    int maxDigitsLine = numberOfDigits(tokens.items[tokens.count - 1].line);
    int maxDigitsColumn = numberOfDigits(maxColumn);

    for (size_t i = 0; i < tokens.count; i++) {
        printf("%zu:%zu", tokens.items[i].line, tokens.items[i].column);
        for (size_t j = 0; j < (maxDigitsLine + maxDigitsColumn - 1)
                                   - numberOfDigits(tokens.items[i].line)
                                   - numberOfDigits(tokens.items[i].column);
             j++) {
            printf(" ");
        }
        printf("  ");

        switch_all_cases(tokens.items[i].kind, {
            case TokenLeftParen:
                printf("LeftParen");
                break;
            case TokenRightParen:
                printf("RightParen");
                break;
            case TokenLeftBracket:
                printf("LeftBracket");
                break;
            case TokenRightBracket:
                printf("RightBracket");
                break;
            case TokenLeftCurly:
                printf("LeftCurly");
                break;
            case TokenRightCurly:
                printf("RightCurly");
                break;
            case TokenComma:
                printf("Comma");
                break;
            case TokenPlus:
                printf("Plus");
                break;
            case TokenSlash:
                printf("Slash");
                break;
            case TokenModulo:
                printf("Modulo");
                break;
            case TokenStar:
                printf("Star");
                break;
            case TokenMinus:
                printf("Minus");
                break;
            case TokenColon:
                printf("Colon");
                break;
            case TokenAmpersand:
                printf("Ampersand");
                break;
            case TokenDot:
                printf("Dot");
                break;
            case TokenNot:
                printf("Not");
                break;
            case TokenNotEqual:
                printf("NotEqual");
                break;
            case TokenGreater:
                printf("Greater");
                break;
            case TokenGreaterEqual:
                printf("GreaterEqual");
                break;
            case TokenLess:
                printf("Less");
                break;
            case TokenLessEqual:
                printf("LessEqual");
                break;
            case TokenColonEqual:
                printf("ColonEqual");
                break;
            case TokenEqual:
                printf("Equal");
                break;
            case TokenEqualEqual:
                printf("EqualEqual");
                break;
            case TokenBe:
                printf("Be");
                break;
            case TokenInteger:
                printf("Integer(%s)", tokens.items[i].literal.content);
                break;
            case TokenFloat:
                printf("Float(%s)", tokens.items[i].literal.content);
                break;
            case TokenIdentifier:
                printf("Identifier(%s)", tokens.items[i].literal.content);
                break;
            case TokenString:
                printf("String(\"%s\")", tokens.items[i].literal.content);
                break;
            case TokenStringLeft:
                printf("StringLeft(\"%s\")", tokens.items[i].literal.content);
                break;
            case TokenStringMiddle:
                printf("StringMiddle(\"%s\")", tokens.items[i].literal.content);
                break;
            case TokenStringRight:
                printf("StringRight(\"%s\")", tokens.items[i].literal.content);
                break;
            case TokenIf:
                printf("If");
                break;
            case TokenElse:
                printf("Else");
                break;
            case TokenWhile:
                printf("While");
                break;
            case TokenFunction:
                printf("Function");
                break;
            case TokenInterface:
                printf("Interface");
                break;
            case TokenBuiltin:
                printf("Builtin");
                break;
            case TokenType:
                printf("Type");
                break;
            case TokenCase:
                printf("Case");
                break;
            case TokenTrue:
                printf("True");
                break;
            case TokenFalse:
                printf("False");
                break;
            case TokenOr:
                printf("Or");
                break;
            case TokenAnd:
                printf("And");
                break;
            case TokenUse:
                printf("Use");
                break;
            case TokenBreak:
                printf("Break");
                break;
            case TokenContinue:
                printf("Continue");
                break;
            case TokenReturn:
                printf("Return");
                break;
            case TokenMut:
                printf("Mut");
                break;
            case TokenNew:
                printf("New");
                break;
            case TokenInclude:
                printf("Include");
                break;
            case TokenExtern:
                printf("Extern");
                break;
            case TokenLinkWith:
                printf("LinkWith");
                break;
            case TokenNewLine:
                printf("NewLine");
                break;
            case TokenEndOfFile:
                printf("EndOfFile");
                break;
        });
        printf("\n");
    }
}