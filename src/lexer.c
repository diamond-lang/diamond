#include "lexer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "common.h"
#include "error.h"
#include "token.h"
#include "types.h"
#include "utilities.h"

static char nextChar(Lexer* lexer) {
    char result = *lexer->sourcePointer;
    if (result != '\0') lexer->sourcePointer += 1;
    return result;
}

static void advance(Lexer* lexer) {
    if (lexer->current == '\n') {
        lexer->column = 1;
        lexer->line += 1;
    }
    lexer->column += 1;
    lexer->current = lexer->next;
    lexer->next = lexer->nextNext;
    lexer->nextNext = nextChar(lexer);
    string_append(&lexer->currentLiteral, lexer->current);
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
        .column = lexer->column - string_size(lexer->currentLiteral)
    };
    lexer->previousWasUseOrInclude = kind == USE || kind == INCLUDE;
    return token;
}

static inline Data numberOfSlotsNeeded(Data length) {
    return 1 + length / sizeof(Data) + (length % sizeof(Data) != 0);
}

static Token createTokenWithLiteral(Lexer* lexer, TokenKind kind) {
    const size_t literalsCount = list_size(*lexer->literals);
    const Data length = string_size(lexer->currentLiteral);
    const char* currentLiteral = string_pointer(lexer->currentLiteral);

    // Check if literal already exist
    Data literalId = None();
    for (Data i = 0; i < list_size(*lexer->literals);
         i += numberOfSlotsNeeded(*list_get(*lexer->literals, i))) {
        if (*list_get(*lexer->literals, i) == length &&
            memcmp(list_get(*lexer->literals, i + 1), currentLiteral, length) ==
                0) {
            literalId = i;
            break;
        }
    }

    // Add literal to literals
    if (literalId == None()) {
        literalId = literalsCount;

        Data extraCapacityNeeded = numberOfSlotsNeeded(length);
        list_ensureExtraCapacity((*lexer->literals), extraCapacityNeeded);

        list_size(*lexer->literals) += extraCapacityNeeded;
        *list_get(*lexer->literals, literalId) = length;
        strncpy(
            (char*)list_get(*lexer->literals, literalId + 1),
            currentLiteral,
            length
        );
    }

    // Create token
    Token token = {
        .kind = kind,
        .line = lexer->line,
        .column = lexer->column - length,
        .literal = literalId
    };
    return token;
}

void initLexer(
    Lexer* lexer, char* filePath, DataList* literals, ErrorList* errors
) {
    lexer->line = 1;
    lexer->column = 1;
    lexer->source = readFile(filePath);
    lexer->sourcePointer = string_pointer(lexer->source);
    lexer->literals = literals;
    lexer->errors = errors;
    lexer->currentLiteral = String();
    lexer->current = '\0';
    lexer->next = nextChar(lexer);
    lexer->nextNext = nextChar(lexer);
    lexer->previousWasUseOrInclude = false;
}

static Token scanNumber(Lexer* lexer);
static Token scanIdentifierOrKeyword(Lexer* lexer);
static Token scanImport(Lexer* lexer);

Token scanToken(Lexer* lexer) {
start:
    string_clear(&lexer->currentLiteral);
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
            if (lexer->previousWasUseOrInclude) return scanImport(lexer);
            return createToken(lexer, DOT);
        case '+': return createToken(lexer, PLUS);
        case '-':
            if (match(lexer, '-')) {
                advanceUntilNewLine(lexer);
                goto start;
            }
            return createToken(lexer, MINUS);
        case '*': return createToken(lexer, STAR);
        case '/': return createToken(lexer, SLASH);
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
    size_t length = string_size(lexer->currentLiteral);
    for (size_t i = 0; i < length; i++) {
        if (string_get(lexer->currentLiteral, i) != literal[i]) return false;
    }
    if (literal[length] != '\0') return false;
    return true;
}

static Token scanIdentifierOrKeyword(Lexer* lexer) {
    if (lexer->previousWasUseOrInclude) return scanImport(lexer);

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
    if (identifierEquals(lexer, "not")) return createToken(lexer, NOT);
    if (identifierEquals(lexer, "extern")) return createToken(lexer, EXTERN);
    if (identifierEquals(lexer, "use")) return createToken(lexer, USE);
    if (identifierEquals(lexer, "include")) return createToken(lexer, INCLUDE);

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