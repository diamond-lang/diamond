#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "common.h"
#include "token.h"

static char nextChar(Lexer* lexer) {
    char result = *lexer->source;
    if (result != '\0') lexer->source += 1;
    return result;
}

static void advance(Lexer* lexer) {
    if (lexer->current == '\n') {
        lexer->column = 1;
        lexer->line += 1;
    }
    lexer->offset += 1;
    lexer->column += 1;
    lexer->current = lexer->next;
    lexer->next = lexer->nextNext;
    lexer->nextNext = nextChar(lexer);
}

static void advanceUntilNewLine(Lexer* lexer) {
    while (lexer->current != '\0' && lexer->current != '\n') {
        advance(lexer);
    }
}

static bool match(Lexer* lexer, char c) {
    if (lexer->next == c) {
        advance(lexer);
        return true;
    }
    return false;
}

static void advanceUntilNextNoneLineToken(Lexer* lexer) {
    while (true) {
        if (lexer->next == '\n') {
            advance(lexer);
        } else if (lexer->next == '-' && lexer->nextNext == '-') {
            advance(lexer);
            advanceUntilNewLine(lexer);
        } else {
            break;
        }
    }
}

static Token createToken(Lexer* lexer, TokenKind kind) {
    Token token = {
        .kind = kind,
        .line = lexer->line,
        .column = lexer->column - (lexer->offset - lexer->start)
    };
    lexer->previousWasImport = kind == IMPORT;
    return token;
}

static Token createTokenWithLiteral(Lexer* lexer, TokenKind kind) {
    char* literal = lexer->initialSource + lexer->start;
    uint32_t length = lexer->offset - lexer->start;
    uint32_t literalId = ast_getLiteralWithLength(lexer->ast, literal, length);
    Token token = {
        .kind = kind,
        .line = lexer->line,
        .column = lexer->column - length,
        .literal = literalId
    };
    return token;
}

void initLexer(Lexer* lexer, char* source, Ast* ast, bool parsingBuiltins) {
    lexer->line = 1;
    lexer->column = 1;
    lexer->source = source;
    lexer->ast = ast;
    lexer->initialSource = source;
    lexer->start = 0;
    lexer->offset = 0;
    lexer->current = '\0';
    lexer->next = nextChar(lexer);
    lexer->nextNext = nextChar(lexer);
    lexer->previousWasImport = false;
    lexer->parsingBuiltins = parsingBuiltins;
}

static Token scanNumber(Lexer* lexer);
static Token scanIdentifierOrKeyword(Lexer* lexer);
static Token scanImport(Lexer* lexer);

Token scanToken(Lexer* lexer) {
start:
    lexer->start = lexer->offset;
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
        if (lexer->previousWasImport) return scanImport(lexer);
        return createToken(lexer, DOT);
    case '+': return createTokenWithLiteral(lexer, IDENTIFIER);
    case '-':
        if (match(lexer, '-')) {
            advanceUntilNewLine(lexer);
            goto start;
        }
        return createTokenWithLiteral(lexer, IDENTIFIER);
    case '*': return createTokenWithLiteral(lexer, IDENTIFIER);
    case '/': return createTokenWithLiteral(lexer, IDENTIFIER);
    case '%': return createTokenWithLiteral(lexer, IDENTIFIER);
    case ':': {
        if (match(lexer, '=')) return createToken(lexer, COLON_EQUAL);
        if (match(lexer, ':')) return createToken(lexer, COLON_COLON);
        return createToken(lexer, COLON);
    }
    case ',': return createToken(lexer, COMMA);
    case '!': {
        if (match(lexer, '=')) return createTokenWithLiteral(lexer, IDENTIFIER);
        unreachable();
    }
    case '=': {
        if (match(lexer, '=')) return createTokenWithLiteral(lexer, IDENTIFIER);
        return createToken(lexer, EQUAL);
    }
    case '>': {
        if (match(lexer, '=')) return createTokenWithLiteral(lexer, IDENTIFIER);
        createTokenWithLiteral(lexer, IDENTIFIER);
    }
    case '<': {
        if (match(lexer, '=')) return createTokenWithLiteral(lexer, IDENTIFIER);
        return createTokenWithLiteral(lexer, IDENTIFIER);
    }
    case ' ':
    case '\t': goto start;
    case '\n': {
        Token token = createToken(lexer, NEW_LINES);
        advanceUntilNextNoneLineToken(lexer);
        return token;
    }
    case '\0': return createToken(lexer, END_OF_FILE);
    default: {
        if (isdigit(lexer->current)) return scanNumber(lexer);
        if (isalpha(lexer->current)) return scanIdentifierOrKeyword(lexer);
    }
    }

    Token result = createTokenWithLiteral(lexer, UNKNOWN_TOKEN);
    advance(lexer);
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
    uint32_t length = lexer->offset - lexer->start;
    for (uint32_t i = 0; i < length; i++) {
        if ((lexer->initialSource + lexer->start)[i] != literal[i])
            return false;
    }
    if (literal[length] != '\0') return false;
    return true;
}

static Token scanIdentifierOrKeyword(Lexer* lexer) {
    if (lexer->previousWasImport) return scanImport(lexer);

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
    if (identifierEquals(lexer, "break")) return createToken(lexer, BREAK);
    if (identifierEquals(lexer, "continue"))
        return createToken(lexer, CONTINUE);
    if (identifierEquals(lexer, "return")) return createToken(lexer, RETURN);
    if (identifierEquals(lexer, "mut")) return createToken(lexer, MUT);
    if (identifierEquals(lexer, "extern")) return createToken(lexer, EXTERN);
    if (identifierEquals(lexer, "import")) return createToken(lexer, IMPORT);
    if (identifierEquals(lexer, "interface"))
        return createToken(lexer, INTERFACE);
    if (lexer->parsingBuiltins && identifierEquals(lexer, "builtin"))
        return createToken(lexer, BUILTIN);

    return createTokenWithLiteral(lexer, IDENTIFIER);
}

static Token scanImport(Lexer* lexer) {
    while (true) {
        if (lexer->next == '/') advance(lexer);
        if (lexer->next == '.' && lexer->nextNext == '.') {
            advance(lexer);
            advance(lexer);
        } else if (lexer->next == '.') {
            advance(lexer);
        } else {
            while (isalnum(lexer->next) || lexer->next == '_') advance(lexer);
        }
        if (lexer->next != '/') break;
    }
    return createTokenWithLiteral(lexer, IMPORT_PATH);
}