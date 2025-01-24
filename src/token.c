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
        int numberOfDigitsLine = numberOfDigits(tokens.items[i].line);
        int numberOfDigitsColumn = numberOfDigits(tokens.items[i].column);
        for (size_t j = 0; j < (maxDigitsLine + maxDigitsColumn) -
                                   numberOfDigitsLine - numberOfDigitsColumn;
             j++) {
            printf(" ");
        }
        printf("  ");

        switch (tokens.items[i].kind) {
            case LEFT_PAREN:
                printf("LEFT_PAREN");
                break;
            case RIGHT_PAREN:
                printf("RIGHT_PAREN");
                break;
            case LEFT_BRACKET:
                printf("LEFT_BRACKET");
                break;
            case RIGHT_BRACKET:
                printf("RIGHT_BRACKET");
                break;
            case LEFT_CURLY:
                printf("LEFT_CURLY");
                break;
            case RIGHT_CURLY:
                printf("RIGHT_CURLY");
                break;
            case COMMA:
                printf("COMMA");
                break;
            case PLUS:
                printf("PLUS");
                break;
            case SLASH:
                printf("SLASH");
                break;
            case MODULO:
                printf("MODULO");
                break;
            case STAR:
                printf("STAR");
                break;
            case MINUS:
                printf("MINUS");
                break;
            case COLON:
                printf("COLON");
                break;
            case AMPERSAND:
                printf("AMPERSAND");
                break;
            case DOT:
                printf("DOT");
                break;
            case NOT:
                printf("NOT");
                break;
            case NOT_EQUAL:
                printf("NOT_EQUAL");
                break;
            case GREATER:
                printf("GREATER");
                break;
            case GREATER_EQUAL:
                printf("GREATER_EQUAL");
                break;
            case LESS:
                printf("LESS");
                break;
            case LESS_EQUAL:
                printf("LESS_EQUAL");
                break;
            case COLON_EQUAL:
                printf("COLON_EQUAL");
                break;
            case EQUAL:
                printf("EQUAL");
                break;
            case EQUAL_EQUAL:
                printf("EQUAL_EQUAL");
                break;
            case BE:
                printf("BE");
                break;
            case INTEGER:
                printf("INTEGER(%s)", tokens.items[i].literal.content);
                break;
            case FLOAT:
                printf("FLOAT(%s)", tokens.items[i].literal.content);
                break;
            case IDENTIFIER:
                printf("IDENTIFIER(%s)", tokens.items[i].literal.content);
                break;
            case STRING:
                printf("STRING(\"%s\")", tokens.items[i].literal.content);
                break;
            case STRING_LEFT:
                printf("STRING_LEFT(\"%s\")", tokens.items[i].literal.content);
                break;
            case STRING_MIDDLE:
                printf(
                    "STRING_MIDDLE(\"%s\")",
                    tokens.items[i].literal.content
                );
                break;
            case STRING_RIGHT:
                printf("STRING_RIGHT(\"%s\")", tokens.items[i].literal.content);
                break;
            case IF:
                printf("IF");
                break;
            case ELSE:
                printf("ELSE");
                break;
            case WHILE:
                printf("WHILE");
                break;
            case FUNCTION:
                printf("FUNCTION");
                break;
            case INTERFACE:
                printf("INTERFACE");
                break;
            case BUILTIN:
                printf("BUILTIN");
                break;
            case TYPE:
                printf("TYPE");
                break;
            case CASE:
                printf("CASE");
                break;
            case TRUE:
                printf("TRUE");
                break;
            case FALSE:
                printf("FALSE");
                break;
            case OR:
                printf("OR");
                break;
            case AND:
                printf("AND");
                break;
            case USE:
                printf("USE");
                break;
            case BREAK:
                printf("BREAK");
                break;
            case CONTINUE:
                printf("CONTINUE");
                break;
            case RETURN:
                printf("RETURN");
                break;
            case MUT:
                printf("MUT");
                break;
            case NEW:
                printf("NEW");
                break;
            case INCLUDE:
                printf("INCLUDE");
                break;
            case EXTERN:
                printf("EXTERN");
                break;
            case LINK_WITH:
                printf("LINK_WITH");
                break;
            case NEW_LINE:
                printf("NEW_LINE");
                break;
            case END_OF_FILE:
                printf("END_OF_FILE");
                break;
        }
        printf("\n");
    }
}

char* token_getLiteral(Token token) {
    switch (token.kind) {
        case LEFT_PAREN:
            return "(";
        case RIGHT_PAREN:
            return ")";
        case LEFT_BRACKET:
            return "[";
        case RIGHT_BRACKET:
            return "]";
        case LEFT_CURLY:
            return "{";
        case RIGHT_CURLY:
            return "}";
        case COMMA:
            return ",";
        case PLUS:
            return "+";
        case SLASH:
            return "/";
        case MODULO:
            return "%%";
        case STAR:
            return "*";
        case MINUS:
            return "-";
        case COLON:
            return ":";
        case AMPERSAND:
            return "&";
        case DOT:
            return ".";
        case NOT:
            return "not";
        case NOT_EQUAL:
            return "!=";
        case GREATER:
            return ">";
        case GREATER_EQUAL:
            return ">=";
        case LESS:
            return "<";
        case LESS_EQUAL:
            return "<=";
        case COLON_EQUAL:
            return ":=";
        case EQUAL:
            return "=";
        case EQUAL_EQUAL:
            return "==";
        case BE:
            return "be";
        case INTEGER:
            return token.literal.content;
        case FLOAT:
            return token.literal.content;
        case IDENTIFIER:
            return token.literal.content;
        case STRING:
            return token.literal.content;
        case STRING_LEFT:
            return token.literal.content;
        case STRING_MIDDLE:
            return token.literal.content;
        case STRING_RIGHT:
            return token.literal.content;
        case IF:
            return "if";
        case ELSE:
            return "else";
        case WHILE:
            return "while";
        case FUNCTION:
            return "function";
        case INTERFACE:
            return "interface";
        case BUILTIN:
            return "builtin";
        case TYPE:
            return "type";
        case CASE:
            return "case";
        case TRUE:
            return "true";
        case FALSE:
            return "false";
        case OR:
            return "or";
        case AND:
            return "and";
        case USE:
            return "use";
        case BREAK:
            return "break";
        case CONTINUE:
            return "continue";
        case RETURN:
            return "return";
        case MUT:
            return "and";
        case NEW:
            return "new";
        case INCLUDE:
            return "include";
        case EXTERN:
            return "extern";
        case LINK_WITH:
            return "link_with";
        case NEW_LINE:
            return "\\n";
        case END_OF_FILE:
            return "\\0";
    }
    unreachable();
}