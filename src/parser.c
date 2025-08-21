#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "arena.h"
#include "ast.h"
#include "common.h"
#include "error.h"
#include "lexer.h"
#include "time.h"
#include "token.h"
#include "types.h"

typedef struct {
    Token previous;
    Token current;
    Token next;
    Uint32Stack indentationLevel;
    Lexer lexer;
    Ast *ast;
    Code *code;
    Arena arena;
} Parser;

static bool atEnd(Parser parser) {
    return parser.current.kind == END_OF_FILE ||
           (parser.current.kind == NEW_LINES && parser.next.kind == END_OF_FILE
           );
}

static bool check(Parser parser, TokenKind kind) {
    return parser.current.kind == kind;
}

static bool peek(Parser parser, TokenKind kind) {
    return parser.next.kind == kind;
}

static void advance(Parser *parser) {
    if (!atEnd(*parser)) {
        parser->previous = parser->current;
        parser->current = parser->next;
        parser->next = scanToken(&parser->lexer);
    }
}

static bool match(Parser *parser, TokenKind token) {
    if (parser->current.kind == token) {
        advance(parser);
        return true;
    }
    return false;
}

static void consumeIfExists(Parser *parser, TokenKind kind) {
    if (check(*parser, kind)) advance(parser);
}

static void advanceUntilNewLine(Parser *parser) {
    while (!atEnd(*parser) && parser->current.kind != NEW_LINES) {
        advance(parser);
    }
}

#define consume(parser, tokenKind, beingParsed)                  \
    if (parser->current.kind == UNKNOWN_TOKEN) {                 \
        addError(parser, UNKNOWN_CHARACTER);                     \
        return None();                                           \
    } else if (parser->current.kind != tokenKind) {              \
        addUnexpectedTokenError(parser, tokenKind, beingParsed); \
        return None();                                           \
    }                                                            \
    advance(parser);

#define bind(name, expression)  \
    uint32_t name = expression; \
    if (name == None()) return None();

#define expect(expression) \
    if (expression == None()) return None();

static bool couldBeExpression(Parser parser) {
    if (check(parser, NEW_LINES)) {
        if (parser.next.column > *stack_top(parser.indentationLevel)) {
            return true;
        } else return false;
    }
    return true;
}

static void addError(Parser *parser, ErrorKind kind) {
    Error error;
    error.kind = kind;
    error.line = parser->current.line;
    error.column = parser->current.column;
    if (error.kind == EXPECTING_LINE_ENDING) {
        error.expectingLineEnding.actualToken = parser->current;
    } else if (error.kind == EXPECTING_EXPRESSION) {
        error.expectingExpression.actualToken = parser->current.kind;
    }
    list_append(&parser->ast->arena, parser->ast->errors, error);
}

static void addUnexpectedTokenError(
    Parser *parser, TokenKind expected, char *beingParsed
) {
    Error error;
    error.kind = UNEXPECTED_TOKEN;
    error.line = parser->current.line;
    error.column = parser->current.column;
    error.unexpectedToken.expectedToken = expected;
    error.unexpectedToken.actualToken = parser->current.kind;
    error.unexpectedToken.beingParsed = beingParsed;
    list_append(&parser->ast->arena, parser->ast->errors, error);
}

static void program(Parser *parser, bool justImports);
static uint32_t import(Parser *parser, Token keyword);
static uint32_t type(Parser *parse);
static uint32_t typeAnnotation(Parser *parser, Token colon);
static uint32_t statementOrDefinition(Parser *parser);
static uint32_t function(Parser *parser, Token keyword);
static uint32_t interface(Parser *parser, Token keyword);
static uint32_t externDefinition(Parser *parser, Token keyword);
static uint32_t typeDefinition(Parser *parser, Token keyword);
static uint32_t block(Parser *parser);
static uint32_t statement(Parser *parser);
static uint32_t declaration(Parser *parser, uint32_t identifier, Token op);
static uint32_t assignment(Parser *parser, uint32_t assignable, Token op);
static uint32_t returnStatement(Parser *parser, Token keyword);
static uint32_t breakStatement(Parser *parser, Token keyword);
static uint32_t continueStatement(Parser *parser, Token keyword);
static uint32_t ifElse(Parser *parser, Token keyword);
static uint32_t whileStatement(Parser *parser, Token keyword);
static uint32_t expression(Parser *parser);
static uint32_t ifElseExpression(Parser *parser, Token keyword);
static uint32_t notExpression(Parser *parser, Token keyword);
static uint32_t or (Parser * parser);
static uint32_t and (Parser * parser);
static uint32_t equality(Parser *parser);
static uint32_t comparison(Parser *parser);
static uint32_t term(Parser *parser);
static uint32_t factor(Parser *parser);
static uint32_t unary(Parser *parser);
static uint32_t negation(Parser *parser, Token operator);
static uint32_t addressOf(Parser *parser, Token operator);
static uint32_t dereference(Parser *parser, Token operator);
static uint32_t unaryPostFix(Parser *parser);
static uint32_t call(Parser *parser, uint32_t accessed, Token leftParen);
static uint32_t fieldAccess(Parser *parser, uint32_t accessed, Token dot);
static uint32_t indexAccess(
    Parser *parser, uint32_t accessed, Token leftBracket
);
static uint32_t primary(Parser *parser);
static uint32_t grouping(Parser *parser, Token leftParen);
static uint32_t floatLiteral(Parser *parser, Token token);
static uint32_t integer(Parser *parser, Token token);
static uint32_t boolean(Parser *parser, Token token);
static uint32_t identifier(Parser *parser);
static uint32_t string(Parser *parser);
static uint32_t structLiteral(
    Parser *parser, uint32_t identifier, Token leftCurly
);
static uint32_t array(Parser *parser, Token leftBracket);

typedef enum { PARSING_BUILTINS, PARSING_IMPORTS, NORMAL_PARSING } ParsingMode;

static void _parse(Ast *ast, char *source, ParsingMode mode, Arena scratch) {
    // Init parser
    Parser parser = {0};
    parser.ast = ast;
    parser.code = &ast->code;
    parser.arena = scratch;

    bool parsingBuiltins = mode == PARSING_BUILTINS;
    initLexer(&parser.lexer, source, parser.ast, parsingBuiltins);
    parser.next = scanToken(&parser.lexer);
    advance(&parser);

    // Parse
    bool justImports = mode == PARSING_IMPORTS;
    program(&parser, justImports);
}

void parse(Ast *ast, char *source, Arena scratch) {
    _parse(ast, source, NORMAL_PARSING, scratch);
}

void parseImports(Ast *ast, char *source, Arena scratch) {
    _parse(ast, source, PARSING_IMPORTS, scratch);
}

void parseBuiltin(Ast *ast, char *source, Arena scratch) {
    _parse(ast, source, PARSING_BUILTINS, scratch);
}

#define checkIndentation()                                                 \
    uint32_t indentationLevel = parser->current.column;                    \
    if (check(*parser, NEW_LINES)) indentationLevel = parser->next.column; \
    if (indentationLevel < *stack_top(parser->indentationLevel)) break;    \
    else if (indentationLevel > *stack_top(parser->indentationLevel)) {    \
        addError(parser, UNEXPECTED_IDENTATION);                           \
        consumeIfExists(parser, NEW_LINES);                                \
        advanceUntilNewLine(parser);                                       \
        continue;                                                          \
    }                                                                      \
    consumeIfExists(parser, NEW_LINES);                                    \
    if (atEnd(*parser)) break;

#define checkEndOfLineAndUnknownToken()                       \
    /* Check of unknown token */                              \
    if (check(*parser, UNKNOWN_TOKEN)) {                      \
        addError(parser, UNKNOWN_CHARACTER);                  \
        advanceUntilNewLine(parser);                          \
    } /* Check were at the end of a line */                   \
    else if (!atEnd(*parser) && !check(*parser, NEW_LINES)) { \
        addError(parser, EXPECTING_LINE_ENDING);              \
        advanceUntilNewLine(parser);                          \
    }

static void program(Parser *parser, bool justImports) {
    // Set new indentation level
    stack_push(&parser->arena, parser->indentationLevel, 1);

    // Parse imports
    while (!atEnd(*parser)) {
        // Check indentation
        checkIndentation();

        // Check we are at the start of a use or include
        if (!match(parser, IMPORT)) break;

        // Parse import
        uint32_t result = import(parser, parser->previous);
        if (result == None()) advanceUntilNewLine(parser);

        // Check were at the end of a line and unknown token
        checkEndOfLineAndUnknownToken();
    }

    if (!justImports) {
        // Parse statements or definitions
        while (!atEnd(*parser)) {
            // Check indentation
            checkIndentation();

            // Parse statement of definition
            uint32_t result = statementOrDefinition(parser);
            if (result == None()) advanceUntilNewLine(parser);

            // Check were at the end of a line and unknown token
            checkEndOfLineAndUnknownToken();
        }
    }

    // Pop indentation level
    stack_pop(parser->indentationLevel);

    // Return
    return;
}

// import → "import" IMPORT_PATH
static uint32_t import(Parser *parser, Token keyword) {
    assert(keyword.kind == IMPORT);
    consume(parser, IMPORT_PATH, "an import");
    list_append(
        &parser->ast->arena,
        parser->ast->imports,
        (Import){parser->previous.literal}
    );
    return list_size(parser->ast->imports) - 1;
}

// type → type ("[" type ("," type)* "]")*
static uint32_t type(Parser *parser) {
    consume(parser, IDENTIFIER, "a type");
    uint32_t literal = parser->previous.literal;
    uint32_t paramsCount = 0;
    uint32_t fistParameter = list_size(parser->ast->types.types);
    if (match(parser, LEFT_BRACKET)) {
        while (!atEnd(*parser)) {
            expect(type(parser));
            paramsCount += 1;
            if (!match(parser, COMMA)) break;
        }
        consume(parser, RIGHT_BRACKET, "a type");
    }
    uint32_t id = ast_addTypeWithParams(
        &parser->ast->arena,
        &parser->ast->types,
        literal,
        paramsCount
    );
    uint32_t *parameters = ast_getParameters(parser->ast->types, id);
    for (uint32_t i = 0; i < paramsCount; i++) {
        parameters[i] = fistParameter + i;
    }
    return id;
}

// typeAnnotation → ":" type
static uint32_t typeAnnotation(Parser *parser, Token colon) {
    assert(colon.kind == COLON);
    return type(parser);
}

// statementOrDefinition → function | interface | builtin | extern |
//                       | typeDefinition | statement
static uint32_t statementOrDefinition(Parser *parser) {
    Token prev = parser->current;
    if (match(parser, FUNCTION)) return function(parser, prev);
    if (match(parser, INTERFACE)) return interface(parser, prev);
    if (match(parser, EXTERN)) return externDefinition(parser, prev);
    if (match(parser, TYPE)) return typeDefinition(parser, prev);
    return statement(parser);
}

static uint32_t functionArgument(Parser *parser, Function *function) {
    FunctionArgument argument = {false, None(), None()};
    if (match(parser, MUT)) {
        argument.mutable = true;
        advance(parser);
    } else {
        argument.mutable = false;
    }
    consume(parser, IDENTIFIER, "a function argument");
    argument.identifier = parser->previous.literal;
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, parser->previous));
        argument.type = annotation;
    }
    list_append(&parser->ast->arena, function->arguments, argument);
    return list_size(function->arguments);
}

// function → "function" IDENTIFIER type_parameters? "(" (functionArgument (":" type)? ",")* ")" (":" type)? block
static uint32_t function(Parser *parser, Token keyword) {
    Function function = {0};
    assert(keyword.kind == FUNCTION);
    consume(parser, IDENTIFIER, "a function");
    function.identifier = parser->previous.literal;
    if (match(parser, LEFT_BRACKET)) {
        todo();
    }
    consume(parser, LEFT_PAREN, "a function");
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        expect(functionArgument(parser, &function));
        if (!match(parser, COMMA)) break;
    }
    consume(parser, RIGHT_PAREN, "a function");
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, parser->previous));
        function.returnType = annotation;

        bool allTypesSet = true;
        for (uint32_t i = 0; i < list_size(function.arguments); i++) {
            if (list_get(function.arguments, i)->type == None()) {
                allTypesSet = false;
                break;
            }
        }

        if (allTypesSet) {
            function.type = ast_addFunctionType(
                &parser->ast->arena,
                &parser->ast->types,
                function.returnType,
                list_size(function.arguments)
            );
            uint32_t *parameters =
                ast_getParameters(parser->ast->types, function.type);
            for (uint32_t i = 0; i < list_size(function.arguments); i++) {
                parameters[i] = list_get(function.arguments, i)->type;
            }
        }
    }
    if (!parser->lexer.parsingBuiltins || !match(parser, BUILTIN)) {
        Code *backup = parser->code;
        parser->code = &function.code;
        bind(body, block(parser));
        parser->code = backup;
    }
    list_append(&parser->ast->arena, parser->ast->functions, function);
    return list_size(parser->ast->functions) - 1;
}

// interface → "interface" IDENTIFIER type_parameters "(" (function_argument ":" type) ",")* ")" ":" type
static uint32_t interface(Parser *parser, Token keyword) { todo(); }

// extern → "extern" IDENTIFIER "(" (IDENTIFIER ":" type "..."? ("," IDENTIFIER ":" type "..."?)*)? ")" ":" type
static uint32_t externDefinition(Parser *parser, Token keyword) { todo(); }

// typeDefinition → "type" IDENTIFIER ("\n"+ IDENTIFIER typeAnnoation)*
static uint32_t typeDefinition(Parser *parser, Token keyword) {
    TypeDefinition typeDefinition = {0};
    assert(keyword.kind == TYPE);
    consume(parser, IDENTIFIER, "a type definition");
    typeDefinition.identifier = parser->previous.literal;

    // Set new indentation level
    if (check(*parser, NEW_LINES)) {
        if (*stack_top(parser->indentationLevel) >= parser->next.column) {
            goto end;
        }
        consumeIfExists(parser, NEW_LINES);
        stack_push(
            &parser->arena,
            parser->indentationLevel,
            parser->current.column
        );

        // Parse fields
        while (!atEnd(*parser)) {
            checkIndentation();
            consume(parser, IDENTIFIER, "a type definition");
            list_append(
                &parser->arena,
                typeDefinition.fields,
                parser->previous.literal
            );
            consume(parser, COLON, "a type definition");
            bind(fieldType, typeAnnotation(parser, parser->previous));
            list_append(
                &parser->ast->arena,
                typeDefinition.fieldTypes,
                fieldType
            );
        }

        // Remove indentation level
        stack_pop(parser->indentationLevel);
    }
end:
    list_append(
        &parser->ast->arena,
        parser->ast->typeDefinitions,
        typeDefinition
    );
    return list_size(parser->ast->typeDefinitions) - 1;
}

static uint32_t block(Parser *parser) {
    uint32_t initialErrorCount = list_size(parser->ast->errors);

    // Set new indentation level
    consumeIfExists(parser, NEW_LINES);
    if (*stack_top(parser->indentationLevel) >= parser->current.column) {
        addError(parser, EXPECTING_NEW_IDENTATION_LEVEL);
    }
    stack_push(
        &parser->arena,
        parser->indentationLevel,
        parser->current.column
    );

    while (!atEnd(*parser)) {
        // Check indentation
        checkIndentation();

        // Parse statement of definition
        uint32_t result = statement(parser);
        if (result == None()) advanceUntilNewLine(parser);

        // Check were at the end of a line and unknown token
        checkEndOfLineAndUnknownToken();
    }

    // Pop indentation level
    stack_pop(parser->indentationLevel);

    // Return
    if (initialErrorCount < list_size(parser->ast->errors)) return None();
    else return 0;
}

// statement → return | ifElse | while | break | continue | declaration |
//           | assignment | call
static uint32_t statement(Parser *parser) {
    Token prev = parser->current;
    if (match(parser, RETURN)) return returnStatement(parser, prev);
    if (match(parser, IF)) return ifElse(parser, prev);
    if (match(parser, WHILE)) return whileStatement(parser, prev);
    if (match(parser, BREAK)) return breakStatement(parser, prev);
    if (match(parser, CONTINUE)) return continueStatement(parser, prev);
    if ((check(*parser, IDENTIFIER) && peek(*parser, EQUAL)) ||
        (check(*parser, IDENTIFIER) && peek(*parser, BE))) {
        advance(parser);
        advance(parser);
        return declaration(parser, prev.literal, parser->previous);
    }

    // Parser tentatively
    bind(result, unary(parser));
    prev = parser->current;
    AstInstructionKind inst = ast_getInstruction(*parser->code, result);
    if (inst == AST_IDENTIFIER && match(parser, COLON_EQUAL)) {
        return assignment(parser, result, prev);
    } else if (match(parser, EQUAL)) {
        if (inst == AST_DEREFERENCE) return assignment(parser, result, prev);
        if (inst == AST_FIELD_ACCESS) return assignment(parser, result, prev);
        if (inst == AST_INDEX_ACCESS) return assignment(parser, result, prev);
        if (inst == AST_CALL) return assignment(parser, result, prev);
    } else if (inst == AST_CALL) return result;

    addError(parser, EXPECTING_STATEMENT);
    return None();
}

// declaration → identifier ("be"|"=") expression (": " type)?
static uint32_t declaration(Parser *parser, uint32_t identifier, Token op) {
    assert(op.kind == EQUAL || op.kind == BE);
    uint32_t type = None();
    expect(expression(parser));
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, parser->previous));
        type = annotation;
    }
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_DECLARATION);
    AstDeclaration *data = (void *)ast_getData(parser->code, id);
    data->mutable = op.kind == EQUAL;
    data->identifier = identifier;
    data->type = type;
    return id;
}

// assignment → assignable ("="|":=") expression (": " type)?
static uint32_t assignment(Parser *parser, uint32_t assignable, Token op) {
    AstInstructionKind kind = *list_get(parser->code->instructions, assignable);
    assert(
        kind == AST_IDENTIFIER || kind == AST_DEREFERENCE ||
        kind == AST_FIELD_ACCESS || kind == AST_INDEX_ACCESS || kind == AST_CALL
    );
    assert(op.kind == COLON_EQUAL || op.kind == EQUAL);
    bool nonlocal = op.kind == COLON_EQUAL;
    uint32_t type = None();
    expect(expression(parser));
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, parser->previous));
        type = annotation;
    }
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_ASSIGNMENT);
    AstAssignment *data = (void *)ast_getData(parser->code, id);
    data->nonlocal = nonlocal;
    data->type = type;
    return id;
}

// return → "return" expression?
static uint32_t returnStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == RETURN);
    if (!atEnd(*parser) && couldBeExpression(*parser)) {
        expect(expression(parser));
        return ast_addInstruction(
            &parser->ast->arena,
            parser->code,
            AST_RETURN_EXPRESSION
        );
    } else {
        return ast_addInstruction(
            &parser->ast->arena,
            parser->code,
            AST_RETURN
        );
    }
}

// break → "break"
static uint32_t breakStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == BREAK);
    return ast_addInstruction(&parser->ast->arena, parser->code, AST_BREAK);
}

// continue → "continue"
static uint32_t continueStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == CONTINUE);
    return ast_addInstruction(&parser->ast->arena, parser->code, AST_CONTINUE);
}

// ifElse → "if" expression block ("\n"* "else" block)
static uint32_t ifElse(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    expect(expression(parser));
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_IF_ELSE);
    expect(block(parser));
    uint32_t ifBlockEnd = list_size(parser->code->instructions) - 1;
    uint32_t elseBlockEnd = None();
    if (parser->next.kind == ELSE) {
        advance(parser);
        uint32_t indentationLevel = parser->current.column;
        advance(parser);

        if (*stack_top(parser->indentationLevel) == indentationLevel) {
            expect(block(parser));
            elseBlockEnd = list_size(parser->code->instructions) - 1;
        } else if (*stack_top(parser->indentationLevel) < indentationLevel) {
            todo();
        } else if (*stack_top(parser->indentationLevel) > indentationLevel) {
            todo();
        }
    }
    AstIfElse *data = (void *)ast_getData(parser->code, id);
    data->ifBlockEnd = ifBlockEnd;
    data->elseBlockEnd = elseBlockEnd;
    return id;
}

// while → "while" expression block
static uint32_t whileStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == WHILE);
    expect(expression(parser));
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_WHILE);
    expect(block(parser));
    AstWhile *data = (void *)ast_getData(parser->code, id);
    data->whileEnd = list_size(parser->code->instructions) - 1;
    return id;
}

// expression → ifElseExpression
//            | not
//            | or
static uint32_t expression(Parser *parser) {
    if (check(*parser, NEW_LINES)) {
        if (!couldBeExpression(*parser)) {
            addError(parser, EXPECTING_NEW_IDENTATION_LEVEL);
            return None();
        }
        advance(parser);
    }
    Token current = parser->current;
    if (match(parser, IF)) return ifElseExpression(parser, current);
    else if (match(parser, NOT)) return notExpression(parser, current);
    else return or (parser);
}

// ifElseExpression → "if" expression expression "else" expression
static uint32_t ifElseExpression(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    expect(expression(parser));
    expect(expression(parser));
    if (check(*parser, NEW_LINES) && parser->next.kind == ELSE) advance(parser);
    consume(parser, ELSE, "an if else");
    expect(expression(parser));
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_IF_ELSE_EXPRESSION
    );
}

// not → "not" expression
static uint32_t notExpression(Parser *parser, Token keyword) {
    assert(keyword.kind == NOT);
    expect(expression(parser));
    return ast_addInstruction(&parser->ast->arena, parser->code, AST_NOT);
}

static AstInstructionKind getBinaryOperator(Token token) {
    switch ((uint8_t)token.kind) {
    case OR: return AST_OR;
    case AND: return AST_AND;
    case EQUAL_EQUAL: return AST_EQUAL_EQUAL;
    case NOT_EQUAL: return AST_NOT_EQUAL;
    case LESS: return AST_LESS;
    case LESS_EQUAL: return AST_LESS_EQUAL;
    case GREATER: return AST_GREATER;
    case GREATER_EQUAL: return AST_GREATER_EQUAL;
    case PLUS: return AST_ADD;
    case MINUS: return AST_SUBTRACT;
    case STAR: return AST_MUL;
    case SLASH: return AST_DIV;
    case MODULO: return AST_MOD;
    }
    unreachable();
}

// or → and ("or" and)*
uint32_t or (Parser * parser) {
    bind(left, and(parser));
    while (match(parser, OR)) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(and(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// and → equality ("and" equality)*
uint32_t and (Parser * parser) {
    bind(left, equality(parser));
    while (match(parser, AND)) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(equality(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// equality → comparison (("=="|"!=") comparison)*
uint32_t equality(Parser *parser) {
    bind(left, comparison(parser));
    while (match(parser, EQUAL_EQUAL) || match(parser, NOT_EQUAL)) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(comparison(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// comparison → term ((">"|">="|"<"|"<=") term)*
uint32_t comparison(Parser *parser) {
    bind(left, term(parser));
    while (match(parser, LESS) || match(parser, LESS_EQUAL) ||
           match(parser, GREATER) || match(parser, GREATER_EQUAL)) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(term(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// term → factor (("+"|"-") factor)*
uint32_t term(Parser *parser) {
    bind(left, factor(parser));
    while (match(parser, PLUS) || match(parser, MINUS)) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(factor(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// factor → unary (("*"|"/"|"%") unary)*
uint32_t factor(Parser *parser) {
    bind(left, unary(parser));
    while (match(parser, STAR) || match(parser, SLASH) || match(parser, MODULO)
    ) {
        AstInstructionKind op = getBinaryOperator(parser->previous);
        expect(unary(parser));
        left = ast_addInstruction(&parser->ast->arena, parser->code, op);
    }
    return left;
}

// unary → negation | addressOf | dererefence | unaryPostFix
uint32_t unary(Parser *parser) {
    Token previous = parser->current;
    if (match(parser, MINUS)) return negation(parser, previous);
    if (match(parser, STAR)) return dereference(parser, previous);
    if (match(parser, AMPERSAND)) return addressOf(parser, previous);
    return unaryPostFix(parser);
}

// negation → "-" unary
uint32_t negation(Parser *parser, Token operator) {
    assert(operator.kind == MINUS);
    expect(unary(parser));
    return ast_addInstruction(&parser->ast->arena, parser->code, AST_NEGATION);
}

// address_of → "&" unary
static uint32_t addressOf(Parser *parser, Token operator) {
    assert(operator.kind == AMPERSAND);
    expect(unary(parser));
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_ADDRESS_OF
    );
}

// dereference → "*" unary
static uint32_t dereference(Parser *parser, Token operator) {
    assert(operator.kind == STAR);
    expect(unary(parser));
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_DEREFERENCE
    );
}

// unaryPostFix → call | fieldAccess | IndexAccess | primary
static uint32_t unaryPostFix(Parser *parser) {
    bind(operand, primary(parser));

    while (true) {
        Token previous = parser->current;
        if (match(parser, LEFT_PAREN))
            operand = call(parser, operand, previous);
        else if (match(parser, DOT))
            operand = fieldAccess(parser, operand, previous);
        else if (match(parser, LEFT_BRACKET))
            operand = indexAccess(parser, operand, previous);
        else break;
    }

    return operand;
}

static uint32_t callArgument(Parser *parser, AstCall *callData) {
    if (match(parser, MUT)) {
        todo();
    }
    callData->argumentsCount += 1;
    return expression(parser);
}

// call → unaryPostFix "(" callArgument (", " callArgument)*  ")"
static uint32_t call(Parser *parser, uint32_t accessed, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);
    AstCall callData = {.argumentsCount = 0, .argumentsMutability = 0};

    if (check(*parser, NEW_LINES) && couldBeExpression(*parser))
        advance(parser);
    while (!check(*parser, RIGHT_PAREN) && !check(*parser, NEW_LINES)) {
        expect(callArgument(parser, &callData));
        if (check(*parser, COMMA)) advance(parser);
        else if (check(*parser, NEW_LINES) && couldBeExpression(*parser))
            advance(parser);
        else if (check(*parser, NEW_LINES) && parser->next.kind == RIGHT_PAREN)
            advance(parser);
    }
    consume(parser, RIGHT_PAREN, "a call");

    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_CALL);
    *(AstCall *)ast_getData(parser->code, id) = callData;
    return id;
}

// field_access → unaryPostFix "." IDENTFIER
static uint32_t fieldAccess(Parser *parser, uint32_t accessed, Token dot) {
    assert(dot.kind == DOT);
    expect(identifier(parser));
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_FIELD_ACCESS
    );
}

// indexAccess → unaryPostFix "[" expression "]"
static uint32_t indexAccess(
    Parser *parser, uint32_t accessed, Token leftBracket
) {
    assert(leftBracket.kind == LEFT_BRACKET);
    expect(expression(parser));
    consume(parser, RIGHT_BRACKET, " a index access");
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_INDEX_ACCESS
    );
}

// primary → grouping
//         | array
//         | float
//         | integer
//         | boolean
//         | string
//         | structLiteral
//         | grouping
uint32_t primary(Parser *parser) {
    Token prev = parser->current;
    if (match(parser, LEFT_PAREN)) return grouping(parser, prev);
    if (match(parser, LEFT_BRACKET)) return array(parser, prev);
    if (match(parser, FLOAT)) return floatLiteral(parser, prev);
    if (match(parser, INTEGER)) return integer(parser, prev);
    if (match(parser, TRUE)) return boolean(parser, prev);
    if (match(parser, FALSE)) return boolean(parser, prev);
    if (check(*parser, STRING)) return string(parser);
    if (check(*parser, IDENTIFIER)) {
        bind(id, identifier(parser));
        prev = parser->current;
        if (match(parser, LEFT_CURLY)) return structLiteral(parser, id, prev);
        return id;
    }

    addError(parser, EXPECTING_EXPRESSION);
    return None();
}

// grouping → "(" expression ")"
uint32_t grouping(Parser *parser, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);
    bind(result, expression(parser));
    consume(parser, RIGHT_PAREN, "a grouping");
    return result;
}

// float → FLOAT
uint32_t floatLiteral(Parser *parser, Token token) {
    assert(token.kind == FLOAT);
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_FLOAT);
    AstFloat *data = (void *)ast_getData(parser->code, id);
    data->literal = token.literal;
    return id;
}

// integer → INTEGER
static uint32_t integer(Parser *parser, Token token) {
    assert(token.kind == INTEGER);
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_INTEGER);
    AstInteger *data = (void *)ast_getData(parser->code, id);
    data->literal = token.literal;
    return id;
}

// boolean → TRUE|FALSE
static uint32_t boolean(Parser *parser, Token token) {
    assert(token.kind == TRUE || token.kind == FALSE);
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_BOOLEAN);
    AstBoolean *data = (void *)ast_getData(parser->code, id);
    data->value = token.kind == TRUE;
    return id;
}

// identifier → IDENTIFIER
static uint32_t identifier(Parser *parser) {
    consume(parser, IDENTIFIER, "an identifier");
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_IDENTIFIER);
    AstIdentifier *data = (void *)ast_getData(parser->code, id);
    data->literal = parser->previous.literal;
    return id;
}

// string → STRING
static uint32_t string(Parser *parser) {
    consume(parser, STRING, "a string");
    uint32_t id =
        ast_addInstruction(&parser->ast->arena, parser->code, AST_STRING);
    AstString *data = (void *)ast_getData(parser->code, id);
    data->literal = parser->previous.literal;
    return id;
}

// struct → IDENTIFIER "{" structField (","|("\n"+)))*  "}"
static uint32_t structLiteral(Parser *parser, uint32_t name, Token leftCurly) {
    assert(*list_get(parser->code->instructions, name) == AST_IDENTIFIER);
    assert(leftCurly.kind == LEFT_CURLY);
    while (!atEnd(*parser) && !check(*parser, RIGHT_CURLY)) {
        consumeIfExists(parser, NEW_LINES);
        expect(identifier(parser));
        consume(parser, COLON, "a struct literal");
        expect(expression(parser));
        if (!match(parser, COMMA) && !match(parser, NEW_LINES)) break;
    }
    consume(parser, RIGHT_CURLY, "a struct literal");
    return ast_addInstruction(
        &parser->ast->arena,
        parser->code,
        AST_STRUCT_LITERAL
    );
}

// array → "[" (expression ("," expression)*)* "]"
static uint32_t array(Parser *parser, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    while (!check(*parser, RIGHT_BRACKET) && !atEnd(*parser)) {
        consumeIfExists(parser, NEW_LINES);
        expect(expression(parser));
        if (!match(parser, COMMA) && !match(parser, NEW_LINES)) break;
    }
    consume(parser, RIGHT_BRACKET, "an array");
    return ast_addInstruction(&parser->ast->arena, parser->code, AST_ARRAY);
}