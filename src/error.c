#include "error.h"

#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "token.h"
#include "utilities.h"

static char* token_getLiteral(Ast ast, Token token) {
    switch (token.kind) {
        case LEFT_PAREN: return "(";
        case RIGHT_PAREN: return ")";
        case LEFT_BRACKET: return "[";
        case RIGHT_BRACKET: return "]";
        case LEFT_CURLY: return "{";
        case RIGHT_CURLY: return "}";
        case COMMA: return ",";
        case PLUS: return "+";
        case SLASH: return "/";
        case MODULO: return "%";
        case STAR: return "*";
        case MINUS: return "-";
        case COLON: return ":";
        case AMPERSAND: return "&";
        case DOT: return ".";
        case NOT: return "not";
        case NOT_EQUAL: return "!=";
        case GREATER: return ">";
        case GREATER_EQUAL: return ">=";
        case LESS: return "<";
        case LESS_EQUAL: return "<=";
        case COLON_EQUAL: return ":=";
        case EQUAL: return "=";
        case EQUAL_EQUAL: return "==";
        case BE: return "be";
        case INTEGER: todo();
        case FLOAT: todo();
        case IDENTIFIER: todo();
        case STRING: todo();
        case STRING_LEFT: todo();
        case STRING_MIDDLE: todo();
        case STRING_RIGHT: todo();
        case IMPORT_PATH: todo();
        case IF: return "if";
        case ELSE: return "else";
        case WHILE: return "while";
        case FUNCTION: return "function";
        case INTERFACE: return "interface";
        case TYPE: return "type";
        case CASE: return "case";
        case TRUE: return "true";
        case FALSE: return "false";
        case OR: return "or";
        case AND: return "and";
        case USE: return "use";
        case BREAK: return "break";
        case CONTINUE: return "continue";
        case RETURN: return "return";
        case MUT: return "and";
        case INCLUDE: return "include";
        case EXTERN: return "extern";
        case NEW_LINES: return "\\n";
        case END_OF_FILE: return "\\0";
        case UNKNOWN_TOKEN: return (char*)ast.literals.items + token.literal;
    }
}

static char* ast_tokenAsString(TokenKind kind) {
    switch (kind) {
        case LEFT_PAREN: return "'('";
        case RIGHT_PAREN: return "')'";
        case LEFT_BRACKET: return "'['";
        case RIGHT_BRACKET: return "']'";
        case LEFT_CURLY: return "'{'";
        case RIGHT_CURLY: return "'}'";
        case COMMA: return "','";
        case PLUS: return "'+'";
        case SLASH: return "'/'";
        case MODULO: return "'%%'";
        case STAR: return "'*'";
        case MINUS: return "'-'";
        case COLON: return "':'";
        case AMPERSAND: return "'&'";
        case DOT: return "'.'";
        case NOT: return "'not'";
        case NOT_EQUAL: return "'!='";
        case GREATER: return "'>'";
        case GREATER_EQUAL: return "'>='";
        case LESS: return "'<'";
        case LESS_EQUAL: return "'<='";
        case COLON_EQUAL: return "':='";
        case EQUAL: return "'='";
        case EQUAL_EQUAL: return "'=='";
        case BE: return "be";
        case INTEGER: return "an integer";
        case FLOAT: return "a float";
        case IDENTIFIER: return "an identifier";
        case STRING: return "a string";
        case STRING_LEFT: return "the start of an interpolated string";
        case STRING_MIDDLE: return "the middle of an interpolated string";
        case STRING_RIGHT: return "the end of an interpolated string";
        case IMPORT_PATH: return "an import path";
        case IF: return "if";
        case ELSE: return "else";
        case WHILE: return "while";
        case FUNCTION: return "function";
        case INTERFACE: return "interface";
        case TYPE: return "type";
        case CASE: return "case";
        case TRUE: return "true";
        case FALSE: return "false";
        case OR: return "or";
        case AND: return "and";
        case USE: return "use";
        case BREAK: return "break";
        case CONTINUE: return "continue";
        case RETURN: return "return";
        case MUT: return "and";
        case INCLUDE: return "include";
        case EXTERN: return "extern";
        case NEW_LINES: return "a new line";
        case END_OF_FILE: return "end of file";
        case UNKNOWN_TOKEN: return "an unknown character";
    }
}

void printCurrentLine(String filePath, size_t line) {
    printf("%zu│ ", line);
    String file = readFile(filePath.content);
    for (size_t i = 0; i < file.count && line >= 1; i++) {
        if (file.content[i] == '\n') {
            line -= 1;
        } else if (line == 1) {
            printf("%c", file.content[i]);
        }
    }
    free(file.content);
    printf("\n");
}

void printBold(char* str) { printf("\x1b[1m%s\x1b[0m", str); }

void printUnderline(char* str) { printf("\x1b[4m%s\x1b[0m", str); }

void printCyan(char* str) { printf("\x1b[36m%s\x1b[4m", str); }

void printBrightCyan(char* str) { printf("\x1b[96m%s\x1b[0m", str); }

void printRed(char* str) { printf("\x1b[31m%s\x1b[0m", str); }

void printMagenta(char* str) { printf("\x1b[35m%s\x1b[0m", str); }

void printBrightMagenta(char* str) { printf("\x1b[95m%s\x1b[0m", str); }

void printHeader(char* title, String filePath) {
    printf("\x1b[96m%s (%s)\x1b[0m\n\n", title, filePath.content);
}

void underlineUntilLocation(String filePath, size_t line, size_t column) {
    for (size_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");
    for (size_t i = 0; i < column - 1; i++) {
        printRed("^");
    }
}

void underlineLocation(String filePath, size_t line, size_t column) {
    for (size_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");
    for (size_t i = 0; i < column - 1; i++) {
        printf(" ");
    }
    printRed("^");
}

void underlineToken(Ast ast, Token token) {
    for (size_t i = 0; i < numberOfDigits(token.line); i++) {
        printf(" ");
    }
    printf("  ");
    for (size_t i = 0; i < token.column - 1; i++) {
        printf(" ");
    }
    char* literal = token_getLiteral(ast, token);
    while (*literal != '\0') {
        printRed("^");
        literal += 1;
    }
}

void underlineLine(String filePath, size_t line) {
    for (size_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");

    String file = readFile(filePath.content);
    for (size_t i = 0; i < file.count && line >= 1; i++) {
        if (file.content[i] == '\n') {
            line -= 1;
        } else if (line == 1) {
            printRed("^");
        }
    }
    free(file.content);
}

void reportError(Ast ast, Error error) {
    switch (error.kind) {
        case FILE_NOT_FOUND: todo(); break;
        case UNKNOWN_CHARACTER:
            printHeader("Unknown character", ast.canonicalPath);
            printCurrentLine(ast.canonicalPath, error.line);
            underlineLocation(ast.canonicalPath, error.line, error.column);
            break;
        case EXPECTING_LINE_ENDING:
            printHeader("Expecting line ending", ast.canonicalPath);
            printf("Finished parsing a statement. A new line was expected.\n\n"
            );
            printCurrentLine(ast.canonicalPath, error.line);
            underlineToken(ast, error.expectingLineEnding.actualToken);
            break;
        case UNEXPECTED_IDENTATION:
            printHeader("Unexpected indentation", ast.canonicalPath);
            printCurrentLine(ast.canonicalPath, error.line);
            underlineUntilLocation(ast.canonicalPath, error.line, error.column);
            break;
        case EXPECTING_STATEMENT:
            printHeader("Expecting statement", ast.canonicalPath);
            printCurrentLine(ast.canonicalPath, error.line);
            underlineLine(ast.canonicalPath, error.line);
            break;
        case EXPECTING_NEW_IDENTATION_LEVEL:
            printHeader("Expecting new indentation level", ast.canonicalPath);
            if (error.line > 1) {
                printCurrentLine(ast.canonicalPath, error.line - 1);
            }
            printCurrentLine(ast.canonicalPath, error.line);
            underlineLocation(ast.canonicalPath, error.line, error.column);
            break;
        case UNEXPECTED_TOKEN:
            printHeader("Unexpected token", ast.canonicalPath);
            printf(
                "Parsing %s. Was expecting %s, but found %s.\n\n",
                error.unexpectedToken.beingParsed,
                ast_tokenAsString(error.unexpectedToken.expectedToken),
                ast_tokenAsString(error.unexpectedToken.actualToken)
            );
            printCurrentLine(ast.canonicalPath, error.line);
            underlineLocation(ast.canonicalPath, error.line, error.column);
            break;
        case EXPECTING_EXPRESSION:
            printHeader("Expected a expression", ast.canonicalPath);
            printf(
                "Was expecting a expression, but found %s.\n\n",
                ast_tokenAsString(error.expectingExpression.actualToken)
            );
            printCurrentLine(ast.canonicalPath, error.line);
            underlineLocation(ast.canonicalPath, error.line, error.column);
            break;
        case UDENFINED_VARIABLE: todo(); break;
        case REASSIGNING_IMMUTABLE_VARIABLE: todo(); break;
        case UDENFINED_FUNCTION: todo(); break;
        case UNHANDLED_RETURN_VALUE: todo(); break;
    }
    printf("\n\n");
}

void reportErrors(Ast ast) {
    for (size_t i = 0; i < ast.errors.count; i++) {
        reportError(ast, ast.errors.items[i]);
    }
}