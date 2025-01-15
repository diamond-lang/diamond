#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "token.h"

typedef ListType(bool) ListBool;

typedef struct {
    size_t line;
    size_t column;
    size_t start;
    size_t current;
    String source;
    BoolStack context;
    TokenList tokens;
    ErrorList* errors;
} Lexer;

void scanToken(Lexer* lexer);
void scanString(Lexer* lexer);
void scanNumber(Lexer* lexer);
void scanIdentifierOrKeyword(Lexer* lexer);
bool atEnd(Lexer lexer);
void advance(Lexer* lexer);
bool match(Lexer* lexer, char* string);
char peek(Lexer lexer);
char peekNext(Lexer lexer);
void advanceUntilNewLine(Lexer* lexer);
void addToken(Lexer* lexer, TokenKind kind);
void addTokenWithLiteral(Lexer* lexer, TokenKind kind, String literal);

TokenList lex(String source, ErrorList* errors) {
    Lexer lexer = {1, 1, 0, 0, source, Stack(), List(), errors};
    while (!atEnd(lexer)) {
        scanToken(&lexer);
    }
    lexer.start = lexer.current;
    addToken(&lexer, TokenEndOfFile);
    return lexer.tokens;
}

void scanToken(Lexer* lexer) {
    lexer->start = lexer->current;

    if (match(lexer, "(")) return addToken(lexer, TokenLeftParen);
    if (match(lexer, ")")) return addToken(lexer, TokenRightParen);
    if (match(lexer, "[")) return addToken(lexer, TokenLeftBracket);
    if (match(lexer, "]")) return addToken(lexer, TokenRightBracket);
    if (peek(*lexer) == '{') {
        if (lexer->context.count != 0) {
            stack_push(lexer->context, false);
        }
        advance(lexer);
        return addToken(lexer, TokenLeftCurly);
    }
    if (peek(*lexer) == '}') {
        if (lexer->context.count != 0) {
            if (stack_top(lexer->context)) {
                stack_pop(lexer->context);
                return scanString(lexer);
            } else {
                stack_pop(lexer->context);
                advance(lexer);
                return addToken(lexer, TokenLeftCurly);
            }
        } else {
            advance(lexer);
            return addToken(lexer, TokenRightCurly);
        }
    }
    if (match(lexer, "+")) return addToken(lexer, TokenPlus);
    if (match(lexer, "*")) return addToken(lexer, TokenStar);
    if (match(lexer, "/")) return addToken(lexer, TokenSlash);
    if (match(lexer, "%")) return addToken(lexer, TokenModulo);
    if (match(lexer, ":=")) return addToken(lexer, TokenColonEqual);
    if (match(lexer, ":")) return addToken(lexer, TokenColon);
    if (match(lexer, ",")) return addToken(lexer, TokenComma);
    if (match(lexer, "!=")) return addToken(lexer, TokenNotEqual);
    if (match(lexer, "==")) return addToken(lexer, TokenEqualEqual);
    if (match(lexer, "=")) return addToken(lexer, TokenEqual);
    if (match(lexer, ">=")) return addToken(lexer, TokenGreaterEqual);
    if (match(lexer, ">")) return addToken(lexer, TokenGreater);
    if (match(lexer, "<=")) return addToken(lexer, TokenLessEqual);
    if (match(lexer, "<")) return addToken(lexer, TokenLess);
    if (match(lexer, "&")) return addToken(lexer, TokenAmpersand);
    if (peek(*lexer) == '.' && isdigit(peekNext(*lexer))) {
        return scanNumber(lexer);
    }
    if (match(lexer, ".")) {
        return addToken(lexer, TokenDot);
    }
    if (match(lexer, "---")) {
        while (!(atEnd(*lexer) || match(lexer, "---"))) {
            advance(lexer);
        }
        if (atEnd(*lexer)) {
            list_append(
                (*lexer->errors),
                (Error){"Error: Unclosed block comment\n"}
            );
        }
        return scanToken(lexer);
    }
    if (match(lexer, "--")) {
        advanceUntilNewLine(lexer);
        advance(lexer);
        return scanToken(lexer);
    }
    if (match(lexer, "-")) {
        return addToken(lexer, TokenMinus);
    }
    if (match(lexer, "_")) {
        return scanIdentifierOrKeyword(lexer);
    }
    if (match(lexer, " ") || match(lexer, "\t")) {
        return;
    }
    if (peek(*lexer) == '\n'
        || (peek(*lexer) == '\r' && peekNext(*lexer) == '\n')) {
        addToken(lexer, TokenNewLine);
        advance(lexer);
        return;
    }
    if (peek(*lexer) == '\"') {
        return scanString(lexer);
    }
    if (isdigit(peek(*lexer))) return scanNumber(lexer);
    if (isalpha(peek(*lexer))) return scanIdentifierOrKeyword(lexer);

    list_append((*lexer->errors), (Error){"Error: Unrecognized character"});
}

void scanString(Lexer* lexer) {
    String literal = {NULL, 0, 0};
    bool isRight = false;

    if (peek(*lexer) == '}') {
        isRight = true;
    }

    advance(lexer);
    while (!(atEnd(*lexer) || match(lexer, "\n"))) {
        if (match(lexer, "\\n")) {
            string_append(&literal, '\n');
        } else if (match(lexer, "\\\"")) {
            string_append(&literal, '\"');
        } else if (match(lexer, "\"")) {
            if (isRight) {
                return addTokenWithLiteral(lexer, TokenStringRight, literal);
            } else {
                return addTokenWithLiteral(lexer, TokenString, literal);
            }
        } else if (match(lexer, "\\{")) {
            string_append(&literal, '{');
        } else if (match(lexer, "{")) {
            stack_push(lexer->context, true);
            if (isRight) {
                return addTokenWithLiteral(lexer, TokenStringMiddle, literal);
            } else {
                return addTokenWithLiteral(lexer, TokenStringLeft, literal);
            }
        } else {
            string_append(&literal, peek(*lexer));
            advance(lexer);
        }
    }

    todo();
}

void scanNumber(Lexer* lexer) {
    // consume digits
    while (isdigit(peek(*lexer))) advance(lexer);

    // eg: .8
    if (peek(*lexer) == '.' && isdigit(peekNext(*lexer))) {
        // consume "."
        advance(lexer);

        // consume digits
        while (isdigit(peek(*lexer))) advance(lexer);

        // Add token
        String literal = string_substring(
            lexer->source,
            lexer->start,
            lexer->current - lexer->start
        );
        return addTokenWithLiteral(lexer, TokenFloat, literal);
    }

    // Add token
    String literal = string_substring(
        lexer->source,
        lexer->start,
        lexer->current - lexer->start
    );
    return addTokenWithLiteral(lexer, TokenInteger, literal);
}

void scanIdentifierOrKeyword(Lexer* lexer) {
    while (isalnum(peek(*lexer)) || peek(*lexer) == '_') advance(lexer);

    String literal = string_substring(
        lexer->source,
        lexer->start,
        lexer->current - lexer->start
    );

    if (string_equals(literal.content, "if")) return addToken(lexer, TokenIf);
    if (string_equals(literal.content, "else"))
        return addToken(lexer, TokenElse);
    if (string_equals(literal.content, "while"))
        return addToken(lexer, TokenWhile);
    if (string_equals(literal.content, "function"))
        return addToken(lexer, TokenFunction);
    if (string_equals(literal.content, "interface"))
        return addToken(lexer, TokenInterface);
    if (string_equals(literal.content, "builtin"))
        return addToken(lexer, TokenBuiltin);
    if (string_equals(literal.content, "type"))
        return addToken(lexer, TokenType);
    if (string_equals(literal.content, "case"))
        return addToken(lexer, TokenCase);
    if (string_equals(literal.content, "be")) return addToken(lexer, TokenBe);
    if (string_equals(literal.content, "true"))
        return addToken(lexer, TokenTrue);
    if (string_equals(literal.content, "false"))
        return addToken(lexer, TokenFalse);
    if (string_equals(literal.content, "and")) return addToken(lexer, TokenAnd);
    if (string_equals(literal.content, "or")) return addToken(lexer, TokenOr);
    if (string_equals(literal.content, "use")) return addToken(lexer, TokenUse);
    if (string_equals(literal.content, "include"))
        return addToken(lexer, TokenInclude);
    if (string_equals(literal.content, "break"))
        return addToken(lexer, TokenBreak);
    if (string_equals(literal.content, "continue"))
        return addToken(lexer, TokenContinue);
    if (string_equals(literal.content, "return"))
        return addToken(lexer, TokenReturn);
    if (string_equals(literal.content, "mut")) return addToken(lexer, TokenMut);
    if (string_equals(literal.content, "new")) return addToken(lexer, TokenNew);
    if (string_equals(literal.content, "not")) return addToken(lexer, TokenNot);
    if (string_equals(literal.content, "extern"))
        return addToken(lexer, TokenExtern);
    if (string_equals(literal.content, "link_with"))
        return addToken(lexer, TokenLinkWith);

    return addTokenWithLiteral(lexer, TokenIdentifier, literal);
}

bool atEnd(Lexer lexer) { return lexer.current >= lexer.source.count; }

void advance(Lexer* lexer) {
    if (peek(*lexer) == '\n') {
        lexer->column = 1;
        lexer->line += 1;
        lexer->current += 1;
    } else if (peek(*lexer) == '\r' && peekNext(*lexer) == '\n') {
        lexer->column = 1;
        lexer->line += 1;
        lexer->current += 2;
    } else {
        lexer->column += 1;
        lexer->current += 1;
    }
}

bool match(Lexer* lexer, char* string) {
    bool match = true;
    int i = 0;
    size_t line = lexer->line;
    size_t column = lexer->column;
    while (!atEnd(*lexer) && string[i] != '\0') {
        if (peek(*lexer) != string[i]) {
            match = false;
            lexer->current -= i;
            lexer->line = line;
            lexer->column = column;
            break;
        }
        advance(lexer);
        i++;
    }
    if (atEnd(*lexer) && string[i] != '\0') {
        match = false;
        lexer->current -= i;
        lexer->line = line;
        lexer->column = column;
    }
    return match;
}

char peek(Lexer lexer) {
    if (atEnd(lexer)) return '\0';
    return lexer.source.content[lexer.current];
}

char peekNext(Lexer lexer) {
    if (lexer.current + 1 >= lexer.source.count) return '\0';
    return lexer.source.content[lexer.current + 1];
}

void advanceUntilNewLine(Lexer* lexer) {
    while (peek(*lexer) != '\n') {
        advance(lexer);
    }
}

void addToken(Lexer* lexer, TokenKind kind) {
    Token token
        = {kind,
           String(),
           lexer->line,
           lexer->column - (lexer->current - lexer->start)};
    list_append(lexer->tokens, token);
}

void addTokenWithLiteral(Lexer* lexer, TokenKind kind, String literal) {
    Token token
        = {kind,
           literal,
           lexer->line,
           lexer->column - (lexer->current - lexer->start)};
    list_append(lexer->tokens, token);
}
