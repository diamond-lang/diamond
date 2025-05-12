#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "error.h"
#include "token.h"
#include "types.h"

static char nextChar(Lexer* lexer) {
    char result = *lexer->source;
    if (result != '\0') lexer->source += 1;
    return result;
}

static void advance(Lexer* lexer) {
    lexer->current = lexer->next;
    lexer->next = lexer->nextNext;
    lexer->nextNext = nextChar(lexer);
    string_append(&lexer->currentLiteral, lexer->current);
    if (lexer->current == '\n') {
        lexer->column = 1;
        lexer->line += 1;
    } else {
        lexer->column += 1;
    }
}

static void advanceUntilNewLine(Lexer* lexer) {
    while (lexer->current != '\0' && lexer->current != '\n') {
        advance(lexer);
    }
}

static bool match(Lexer* lexer, char c) {
    if (lexer->next == c) {
        advance(lexer);
        advance(lexer);
        return true;
    }
    return false;
}

static Token createToken(Lexer* lexer, TokenKind kind) {
    Token token = {
        .kind = kind,
        .line = lexer->line,
        .column = lexer->column - lexer->currentLiteral.count
    };
    return token;
}

static Token createTokenWithLiteral(Lexer* lexer, TokenKind kind) {
    // Add literal to literals
    uint32_t literalId = lexer->literals->count;
    size_t length = lexer->currentLiteral.count;
    list_appendCapacity((*lexer->literals), length + 1);
    strncpy(
        (char*)lexer->literals->items + literalId,
        lexer->currentLiteral.content,
        length
    );
    lexer->literals->count += length + 1;
    lexer->literals->items[lexer->literals->count - 1] = '\0';

    // Creat token
    Token token = {
        .kind = kind,
        .line = lexer->line,
        .column = lexer->column - lexer->currentLiteral.count,
        .literal = literalId
    };
    return token;
}

void initLexer(
    Lexer* lexer, char* source, Uint8List* literals, ErrorList* errors
) {
    lexer->line = 1;
    lexer->column = 1;
    lexer->source = source;
    lexer->literals = literals;
    lexer->errors = errors;
    lexer->currentLiteral = String();
    lexer->next = nextChar(lexer);
    lexer->nextNext = nextChar(lexer);
}

static Token scanNumber(Lexer* lexer);
static Token scanIdentifierOrKeyword(Lexer* lexer);

Token scanToken(Lexer* lexer) {
start:
    lexer->currentLiteral.count = 0;
    advance(lexer);
    switch (lexer->current) {
        case '(': return createToken(lexer, LEFT_PAREN);
        case ')': return createToken(lexer, RIGHT_PAREN);
        case '{': return createToken(lexer, LEFT_CURLY);
        case '}': return createToken(lexer, RIGHT_CURLY);
        case '[': return createToken(lexer, LEFT_BRACKET);
        case ']': return createToken(lexer, RIGHT_BRACKET);
        case '&': return createToken(lexer, AMPERSAND);
        case '.':
            if (isdigit(lexer->next)) return scanNumber(lexer);
            return createToken(lexer, DOT);
        case '+': return createToken(lexer, PLUS);
        case '-':
            if (match(lexer, '-')) {
                advanceUntilNewLine(lexer);
                goto start;
            }
            return createToken(lexer, MINUS);
        case '*': return createToken(lexer, STAR);
        case '%': return createToken(lexer, MODULO);
        case ':': {
            if (match(lexer, '=')) return createToken(lexer, COLON_EQUAL);
            return createToken(lexer, COLON);
        }
        case ',': return createToken(lexer, COMMA);
        case '!': {
            if (match(lexer, '=')) return createToken(lexer, NOT_EQUAL);
            unreachable();
        }
        case '=': {
            if (match(lexer, '=')) return createToken(lexer, EQUAL_EQUAL);
            return createToken(lexer, EQUAL);
        }
        case '>': {
            if (match(lexer, '=')) return createToken(lexer, GREATER_EQUAL);
            return createToken(lexer, GREATER);
        }
        case '<': {
            if (match(lexer, '=')) return createToken(lexer, LESS_EQUAL);
            return createToken(lexer, LESS);
        }
        case ' ':
        case '\t': goto start;
        case '\n': return createToken(lexer, NEW_LINE);
        case '\0': return createToken(lexer, END_OF_FILE);
        default: {
            if (isdigit(lexer->current)) return scanNumber(lexer);
            if (isalpha(lexer->current)) return scanIdentifierOrKeyword(lexer);
        }
    }

    advance(lexer);
    Token result = createTokenWithLiteral(lexer, UNKNOWN_TOKEN);
    return result;
}

static Token scanNumber(Lexer* lexer) {
    // consume digits
    while (isdigit(lexer->next)) advance(lexer);

    // eg: .8
    if (lexer->next == '.' && isdigit(lexer->nextNext)) {
        // consume "." and first digit
        advance(lexer);
        advance(lexer);

        // consume digits
        while (isdigit(lexer->next)) advance(lexer);

        //Add token
        return createTokenWithLiteral(lexer, FLOAT);
    }

    return createTokenWithLiteral(lexer, FLOAT);
}

static bool identifierEquals(Lexer* lexer, char* literal) {
    size_t length = lexer->currentLiteral.count;
    for (size_t i = 0; i < length; i++) {
        if (lexer->currentLiteral.content[i] != literal[i]) return false;
    }
    if (literal[length] != '\0') return false;
    return true;
}

static Token scanIdentifierOrKeyword(Lexer* lexer) {
    while (isalnum(lexer->next) || lexer->next == '_') advance(lexer);

    if (identifierEquals(lexer, "if")) return createToken(lexer, IF);
    if (identifierEquals(lexer, "else")) return createToken(lexer, ELSE);
    if (identifierEquals(lexer, "while")) return createToken(lexer, WHILE);
    if (identifierEquals(lexer, "function"))
        return createToken(lexer, FUNCTION);
    if (identifierEquals(lexer, "interface"))
        return createToken(lexer, INTERFACE);
    if (identifierEquals(lexer, "type")) return createToken(lexer, TYPE);
    if (identifierEquals(lexer, "case")) return createToken(lexer, CASE);
    if (identifierEquals(lexer, "be")) return createToken(lexer, BE);
    if (identifierEquals(lexer, "true")) return createToken(lexer, TRUE);
    if (identifierEquals(lexer, "false")) return createToken(lexer, FALSE);
    if (identifierEquals(lexer, "and")) return createToken(lexer, AND);
    if (identifierEquals(lexer, "or")) return createToken(lexer, OR);
    if (identifierEquals(lexer, "use")) return createToken(lexer, USE);
    if (identifierEquals(lexer, "include")) return createToken(lexer, INCLUDE);
    if (identifierEquals(lexer, "break")) return createToken(lexer, BREAK);
    if (identifierEquals(lexer, "continue"))
        return createToken(lexer, CONTINUE);
    if (identifierEquals(lexer, "return")) return createToken(lexer, RETURN);
    if (identifierEquals(lexer, "mut")) return createToken(lexer, MUT);
    if (identifierEquals(lexer, "not")) return createToken(lexer, NOT);
    if (identifierEquals(lexer, "extern")) return createToken(lexer, EXTERN);

    return createTokenWithLiteral(lexer, IDENTIFIER);
}