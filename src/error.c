#include "error.h"

#include <stdio.h>

#include "arena.h"
#include "ast.h"
#include "common.h"
#include "token.h"
#include "types.h"
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
    case IMPORT: return "import";
    case BREAK: return "break";
    case CONTINUE: return "continue";
    case RETURN: return "return";
    case MUT: return "and";
    case EXTERN: return "extern";
    case NEW_LINES: return "\\n";
    case END_OF_FILE: return "\\0";
    case BUILTIN: return "builtin";
    case UNKNOWN_TOKEN: todo();
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
    case IMPORT: return "import";
    case BREAK: return "break";
    case CONTINUE: return "continue";
    case RETURN: return "return";
    case MUT: return "and";
    case EXTERN: return "extern";
    case NEW_LINES: return "a new line";
    case END_OF_FILE: return "end of file";
    case BUILTIN: return "builtin";
    case UNKNOWN_TOKEN: return "an unknown character";
    }
}

void printCurrentLine(String filePath, uint32_t line, Arena scratch) {
    printf("%u│ ", line);
    String file = readFile(&scratch, string_asCString(filePath));
    for (uint32_t i = 0; i < string_size(file) && line >= 1; i++) {
        if (string_get(file, i) == '\n') {
            line -= 1;
        } else if (line == 1) {
            printf("%c", string_get(file, i));
        }
    }
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
    printf("\x1b[96m%s (%s)\x1b[0m\n\n", title, string_asCString(filePath));
}

void underlineUntilLocation(String filePath, uint32_t line, uint32_t column) {
    for (uint32_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");
    for (uint32_t i = 0; i < column - 1; i++) {
        printRed("^");
    }
}

void underlineLocation(String filePath, uint32_t line, uint32_t column) {
    for (uint32_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");
    for (uint32_t i = 0; i < column - 1; i++) {
        printf(" ");
    }
    printRed("^");
}

void underlineToken(Ast ast, Token token) {
    for (uint32_t i = 0; i < numberOfDigits(token.line); i++) {
        printf(" ");
    }
    printf("  ");
    for (uint32_t i = 0; i < token.column - 1; i++) {
        printf(" ");
    }
    char* literal = token_getLiteral(ast, token);
    while (*literal != '\0') {
        printRed("^");
        literal += 1;
    }
}

void underlineLine(String filePath, uint32_t line, Arena scratch) {
    for (uint32_t i = 0; i < numberOfDigits(line); i++) {
        printf(" ");
    }
    printf("  ");
    String file = readFile(&scratch, string_asCString(filePath));
    for (uint32_t i = 0; i < string_size(file) && line >= 1; i++) {
        if (string_get(file, i) == '\n') {
            line -= 1;
        } else if (line == 1) {
            printRed("^");
        }
    }
}

void reportError(Ast ast, String path, Error error, Arena scratch) {
    switch (error.kind) {
    case FILE_NOT_FOUND: todo(); break;
    case UNKNOWN_CHARACTER:
        printHeader("Unknown character", path);
        printCurrentLine(path, error.line, scratch);
        underlineLocation(path, error.line, error.column);
        break;
    case EXPECTING_LINE_ENDING:
        printHeader("Expecting line ending", path);
        printf("Finished parsing a statement. A new line was expected.\n\n");
        printCurrentLine(path, error.line, scratch);
        underlineToken(ast, error.expectingLineEnding.actualToken);
        break;
    case UNEXPECTED_IDENTATION:
        printHeader("Unexpected indentation", path);
        printCurrentLine(path, error.line, scratch);
        underlineUntilLocation(path, error.line, error.column);
        break;
    case EXPECTING_STATEMENT:
        printHeader("Expecting statement", path);
        printCurrentLine(path, error.line, scratch);
        underlineLine(path, error.line, scratch);
        break;
    case EXPECTING_NEW_IDENTATION_LEVEL:
        printHeader("Expecting new indentation level", path);
        if (error.line > 1) {
            printCurrentLine(path, error.line - 1, scratch);
        }
        printCurrentLine(path, error.line, scratch);
        underlineLocation(path, error.line, error.column);
        break;
    case UNEXPECTED_TOKEN:
        printHeader("Unexpected token", path);
        printf(
            "Parsing %s. Was expecting %s, but found %s.\n\n",
            error.unexpectedToken.beingParsed,
            ast_tokenAsString(error.unexpectedToken.expectedToken),
            ast_tokenAsString(error.unexpectedToken.actualToken)
        );
        printCurrentLine(path, error.line, scratch);
        underlineLocation(path, error.line, error.column);
        break;
    case EXPECTING_EXPRESSION:
        printHeader("Expected a expression", path);
        printf(
            "Was expecting a expression, but found %s.\n\n",
            ast_tokenAsString(error.expectingExpression.actualToken)
        );
        printCurrentLine(path, error.line, scratch);
        underlineLocation(path, error.line, error.column);
        break;
    case UDENFINED_VARIABLE: todo(); break;
    case REASSIGNING_IMMUTABLE_VARIABLE: todo(); break;
    case UDENFINED_FUNCTION: todo(); break;
    case UNHANDLED_RETURN_VALUE: todo(); break;
    }
    printf("\n\n");
}

void reportErrors(Ast ast, String path, Arena scratch) {
    for (uint32_t i = 0; i < list_size(ast.errors); i++) {
        reportError(ast, path, *list_get(ast.errors, i), scratch);
    }
}