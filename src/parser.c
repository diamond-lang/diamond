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
    SizeTStack indentationLevel;
    Lexer lexer;
    Ast *ast;
} Parser;

static bool atEnd(Parser parser) {
    return parser.current.kind == END_OF_FILE ||
           (parser.current.kind == NEW_LINES && parser.next.kind == END_OF_FILE
           );
}

static bool check(Parser parser, TokenKind kind) {
    return parser.current.kind == kind;
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

#define bind(name, expression) \
    NodeId name = expression;  \
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
    list_append(parser->ast->errors, error);
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
    list_append(parser->ast->errors, error);
}

static NodeId addDefinition(Parser *parser, NodeId node, LiteralId literal) {
    for (size_t i = 0; i < list_size(parser->ast->definitions); i++) {
        Definition def = *list_get(parser->ast->definitions, i);
        LiteralId id;
        switch (def.kind) {
            case FUNCTION_DEFINITION:
                id = ast_getData(AstFunction, parser->ast, def.id)->literal;
                break;
            case TYPE_DEFINITION:
                id = ast_getData(AstTypeDefinition, parser->ast, def.id)
                         ->literal;
                break;
        }
        if (id == literal) {
            todo();
            return None();
        }
    }
    return node;
}

typedef struct {
    NodeId insertNodes;
    DataId insertData;
} InsertLocation;

static void program(Parser *parser, bool justImports);
static NodeId import(Parser *parser, Token keyword);
static NodeId type(Parser *parse);
static NodeId typeAnnotation(Parser *parser, Token colon);
static NodeId statementOrDefinition(Parser *parser);
static NodeId function(Parser *parser, Token keyword);
static NodeId interface(Parser *parser, Token keyword);
static NodeId externDefinition(Parser *parser, Token keyword);
static NodeId typeDefinition(Parser *parser, Token keyword);
static NodeId block(Parser *parser);
static NodeId statement(Parser *parser);
static NodeId declaration(
    Parser *parser, NodeId identifier, Token operator, InsertLocation location
);
static NodeId assignment(
    Parser *parser, NodeId assignable, Token operator, InsertLocation location
);
static NodeId returnStatement(Parser *parser, Token keyword);
static NodeId breakStatement(Parser *parser, Token keyword);
static NodeId continueStatement(Parser *parser, Token keyword);
static NodeId ifElse(Parser *parser, Token keyword);
static NodeId whileStatement(Parser *parser, Token keyword);
static NodeId expression(Parser *parser);
static NodeId ifElseExpression(Parser *parser, Token keyword);
static NodeId notExpression(Parser *parser, Token keyword);
static NodeId or (Parser * parser);
static NodeId and (Parser * parser);
static NodeId equality(Parser *parser);
static NodeId comparison(Parser *parser);
static NodeId term(Parser *parser);
static NodeId factor(Parser *parser);
static NodeId unary(Parser *parser);
static NodeId negation(Parser *parser, Token operator);
static NodeId addressOf(Parser *parser, Token operator);
static NodeId dereference(Parser *parser, Token operator);
static NodeId unaryPostFix(Parser *parser);
static NodeId call(Parser *parser, NodeId accessed, Token leftParen);
static NodeId fieldAccess(Parser *parser, NodeId accessed, Token dot);
static NodeId indexAccess(Parser *parser, NodeId accessed, Token leftBracket);
static NodeId primary(Parser *parser);
static NodeId grouping(Parser *parser, Token leftParen);
static NodeId floatLiteral(Parser *parser, Token token);
static NodeId integer(Parser *parser, Token token);
static NodeId boolean(Parser *parser, Token token);
static NodeId identifier(Parser *parser);
static NodeId string(Parser *parser);
static NodeId structLiteral(Parser *parser, NodeId identifier, Token leftCurly);
static NodeId array(Parser *parser, Token leftBracket);

void parse(Ast *ast) {
    arena_newLifetime();

    // Init parser
    Parser parser = {.indentationLevel = Stack()};
    parser.ast = ast;

    initLexer(
        &parser.lexer,
        string_pointer(ast->canonicalPath),
        &parser.ast->literals
    );
    parser.next = scanToken(&parser.lexer);
    advance(&parser);

    // Parse
    program(&parser, false);

    arena_destroyCurrentLifetime();
}

void parseImports(Ast *ast) {
    arena_newLifetime();

    // Init parser
    Parser parser = {.indentationLevel = Stack()};
    parser.ast = ast;
    initLexer(
        &parser.lexer,
        string_pointer(ast->canonicalPath),
        &parser.ast->literals
    );
    parser.next = scanToken(&parser.lexer);
    advance(&parser);

    // Parse
    program(&parser, true);

    arena_destroyCurrentLifetime();
}

#define checkIndentation()                                                 \
    size_t indentationLevel = parser->current.column;                      \
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
    stack_push(parser->indentationLevel, 1);

    // Parse imports
    while (!atEnd(*parser)) {
        // Check indentation
        checkIndentation();

        // Check we are at the start of a use or include
        if (!match(parser, IMPORT)) break;

        // Parse import
        NodeId result = import(parser, parser->previous);
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
            NodeId result = statementOrDefinition(parser);
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
static NodeId import(Parser *parser, Token keyword) {
    assert(keyword.kind == IMPORT);
    AstKind node = AST_IMPORT;
    NodeId id = ast_createNode(parser->ast, node);
    consume(parser, IMPORT_PATH, "an import");
    ast_getData(AstImport, parser->ast, id)->path = parser->previous.literal;
    return id;
}

// type → type ("[" type ("," type)* "]")*
static NodeId type(Parser *parser) {
    NodeId id = ast_createNode(parser->ast, AST_TYPE);
    consume(parser, IDENTIFIER, "a type");
    ast_getData(AstType, parser->ast, id)->literal = parser->previous.literal;

    if (match(parser, LEFT_BRACKET)) {
        while (!atEnd(*parser)) {
            expect(type(parser));
            if (!match(parser, COMMA)) break;
        }
        consume(parser, RIGHT_BRACKET, "a type");
    }
    ast_getData(AstType, parser->ast, id)->lastNode =
        list_size(parser->ast->nodes) - 1;
    return id;
}

// typeAnnotation → ":" type
static NodeId typeAnnotation(Parser *parser, Token colon) {
    assert(colon.kind == COLON);
    return type(parser);
}

// statementOrDefinition → function | interface | builtin | extern |
//                       | typeDefinition | statement
static NodeId statementOrDefinition(Parser *parser) {
    Token prev = parser->current;
    if (match(parser, FUNCTION)) return function(parser, prev);
    if (match(parser, INTERFACE)) return interface(parser, prev);
    if (match(parser, EXTERN)) return externDefinition(parser, prev);
    if (match(parser, TYPE)) return typeDefinition(parser, prev);
    return statement(parser);
}

static NodeId functionArgument(Parser *parser, AstFunction *functionData) {
    if (match(parser, MUT)) {
        ast_setBit(
            &functionData->argumentsMutability,
            functionData->numberOfArguments
        );
        advance(parser);
    }
    functionData->numberOfArguments += 1;

    consume(parser, IDENTIFIER, "a function argument");
    NodeId id = ast_createNode(parser->ast, AST_FUNCTION_ARGUMENT);
    ast_getData(AstFunctionArgument, parser->ast, id)->literal =
        parser->previous.literal;

    if (match(parser, COLON)) {
        expect(typeAnnotation(parser, parser->previous));
    }
    return id;
}

// function → "function" IDENTIFIER type_parameters? "(" (functionArgument (":" type)? ",")* ")" (":" type)? block
static NodeId function(Parser *parser, Token keyword) {
    assert(keyword.kind == FUNCTION);
    NodeId id = ast_createNode(parser->ast, AST_FUNCTION);
    AstFunction data;
    consume(parser, IDENTIFIER, "a function");
    data.literal = parser->previous.literal;
    if (match(parser, LEFT_BRACKET)) {
        todo();
    }
    consume(parser, LEFT_PAREN, "a function");
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        expect(functionArgument(parser, &data));
        if (!match(parser, COMMA)) break;
    }
    consume(parser, RIGHT_PAREN, "a function");
    if (match(parser, COLON)) {
        expect(typeAnnotation(parser, parser->previous));
    }
    bind(body, block(parser));
    data.lastNode = list_size(parser->ast->nodes) - 1;
    *ast_getData(AstFunction, parser->ast, id) = data;
    return addDefinition(parser, id, data.literal);
}

// interface → "interface" IDENTIFIER type_parameters "(" (function_argument ":" type) ",")* ")" ":" type
static NodeId interface(Parser *parser, Token keyword) { todo(); }

// extern → "extern" IDENTIFIER "(" (IDENTIFIER ":" type "..."? ("," IDENTIFIER ":" type "..."?)*)? ")" ":" type
static NodeId externDefinition(Parser *parser, Token keyword) { todo(); }

// typeDefinition → "type" IDENTIFIER ("\n"+ IDENTIFIER typeAnnoation)*
static NodeId typeDefinition(Parser *parser, Token keyword) {
    assert(keyword.kind == TYPE);
    NodeId id = ast_createNode(parser->ast, AST_TYPE_DEFINITION);
    AstTypeDefinition data;
    consume(parser, IDENTIFIER, "a type definition");
    data.literal = parser->previous.literal;

    // Set new indentation level
    if (check(*parser, NEW_LINES)) {
        if (*stack_top(parser->indentationLevel) >= parser->next.column) {
            return id;
        }
        consumeIfExists(parser, NEW_LINES);
        stack_push(parser->indentationLevel, parser->current.column);

        // Parse fields
        while (!atEnd(*parser)) {
            checkIndentation();
            expect(identifier(parser));
            consume(parser, COLON, "a type definition");
            expect(typeAnnotation(parser, parser->previous));
        }
    }
    data.lastNode = list_size(parser->ast->nodes) - 1;
    *ast_getData(AstTypeDefinition, parser->ast, id) = data;
    return addDefinition(parser, id, data.literal);
}

static NodeId block(Parser *parser) {
    size_t initialErrorCount = list_size(parser->ast->errors);

    // Set new indentation level
    consumeIfExists(parser, NEW_LINES);
    if (*stack_top(parser->indentationLevel) >= parser->current.column) {
        addError(parser, EXPECTING_NEW_IDENTATION_LEVEL);
    }
    stack_push(parser->indentationLevel, parser->current.column);

    while (!atEnd(*parser)) {
        // Check indentation
        checkIndentation();

        // Parse statement of definition
        NodeId result = statement(parser);
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
static NodeId statement(Parser *parser) {
    Token prev = parser->current;
    if (match(parser, RETURN)) return returnStatement(parser, prev);
    if (match(parser, IF)) return ifElse(parser, prev);
    if (match(parser, WHILE)) return whileStatement(parser, prev);
    if (match(parser, BREAK)) return breakStatement(parser, prev);
    if (match(parser, CONTINUE)) return continueStatement(parser, prev);

    // Rememember position to insert
    NodeId insertLocationNodes = list_size(parser->ast->nodes);
    NodeId insertLocationData = list_size(parser->ast->data);
    InsertLocation loc = {insertLocationNodes, insertLocationData};

    // Parser tentatively
    bind(result, unary(parser));
    prev = parser->current;
    AstKind node = *list_get(parser->ast->nodes, result);
    if (node == AST_IDENTIFIER) {
        if (match(parser, EQUAL)) return declaration(parser, result, prev, loc);
        if (match(parser, BE)) return declaration(parser, result, prev, loc);
        if (match(parser, COLON_EQUAL))
            return assignment(parser, result, prev, loc);
    } else if (match(parser, EQUAL)) {
        if (node == AST_DEREFERENCE)
            return assignment(parser, result, prev, loc);
        if (node == AST_FIELD_ACCESS)
            return assignment(parser, result, prev, loc);
        if (node == AST_INDEX_ACCESS)
            return assignment(parser, result, prev, loc);
        if (node == AST_CALL) return assignment(parser, result, prev, loc);
    } else if (node == AST_CALL) return result;

    addError(parser, EXPECTING_STATEMENT);
    return None();
}

// declaration → identifier ("be"|"=") expression (": " type)?
static NodeId declaration(
    Parser *parser, NodeId identifier, Token operator, InsertLocation location
) {
    assert(*list_get(parser->ast->nodes, identifier) == AST_IDENTIFIER);
    assert(operator.kind == EQUAL || operator.kind == BE);
    NodeId id = ast_insertNode(
        parser->ast,
        AST_DECLARATION,
        location.insertNodes,
        location.insertData
    );
    ast_getData(AstDeclaration, parser->ast, id)->mutable = operator.kind ==
                                                            EQUAL;
    expect(expression(parser));
    ast_getData(AstDeclaration, parser->ast, id)->lastExpressionNode =
        list_size(parser->ast->nodes) - 1;
    if (match(parser, COLON)) {
        expect(typeAnnotation(parser, parser->previous));
        ast_getData(AstDeclaration, parser->ast, id)->lastTypeNode =
            list_size(parser->ast->nodes) - 1;
    }
    return id;
}

// assignment → assignable ("="|":=") expression (": " type)?
static NodeId assignment(
    Parser *parser, NodeId assignable, Token operator, InsertLocation location
) {
    AstKind kind = *list_get(parser->ast->nodes, assignable);
    assert(
        kind == AST_IDENTIFIER || kind == AST_DEREFERENCE ||
        kind == AST_FIELD_ACCESS || kind == AST_INDEX_ACCESS || kind == AST_CALL
    );
    assert(operator.kind == COLON_EQUAL || operator.kind == EQUAL);
    NodeId id = ast_insertNode(
        parser->ast,
        AST_ASSIGNMENT,
        location.insertNodes,
        location.insertData
    );
    ast_getData(AstAssignment, parser->ast, id)->nonlocal = operator.kind ==
                                                            COLON_EQUAL;
    ast_getData(AstAssignment, parser->ast, id)->lastAssignableNode =
        list_size(parser->ast->nodes) - 1;
    bind(result, expression(parser));
    ast_getData(AstAssignment, parser->ast, id)->lastExpressionNode =
        list_size(parser->ast->nodes) - 1;
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, parser->previous));
        ast_getData(AstAssignment, parser->ast, id)->lastTypeNode =
            list_size(parser->ast->nodes) - 1;
    }
    return id;
}

// return → "return" expression?
static NodeId returnStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == RETURN);
    if (!atEnd(*parser) && couldBeExpression(*parser)) {
        NodeId id = ast_createNode(parser->ast, AST_RETURN_WITH_EXPRESSION);
        expect(expression(parser));
        return id;
    } else {
        return ast_createNode(parser->ast, AST_RETURN);
    }
}

// break → "break"
static NodeId breakStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == BREAK);
    return ast_createNode(parser->ast, AST_BREAK);
}

// continue → "continue"
static NodeId continueStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == CONTINUE);
    return ast_createNode(parser->ast, AST_CONTINUE);
}

// ifElse → "if" expression block ("\n"* "else" block)
static NodeId ifElse(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    NodeId id = ast_createNode(parser->ast, AST_IF_ELSE);
    expect(expression(parser));
    ast_getData(AstIfElse, parser->ast, id)->lastConditionNode =
        list_size(parser->ast->nodes) - 1;
    expect(block(parser));
    ast_getData(AstIfElse, parser->ast, id)->lastIfNode =
        list_size(parser->ast->nodes) - 1;

    // Parse else block
    if (parser->next.kind == ELSE) {
        advance(parser);
        size_t indentationLevel = parser->current.column;
        advance(parser);

        if (*stack_top(parser->indentationLevel) == indentationLevel) {
            expect(block(parser));
            ast_getData(AstIfElse, parser->ast, id)->lastElseNode =
                list_size(parser->ast->nodes) - 1;
        } else if (*stack_top(parser->indentationLevel) < indentationLevel) {
            todo();
        } else if (*stack_top(parser->indentationLevel) > indentationLevel) {
            todo();
        }
    }

    // Return
    return id;
}

// while → "while" expression block
static NodeId whileStatement(Parser *parser, Token keyword) {
    assert(keyword.kind == WHILE);
    NodeId id = ast_createNode(parser->ast, AST_WHILE);
    expect(expression(parser));
    expect(block(parser));
    return id;
}

// expression → ifElseExpression
//            | not
//            | or
static NodeId expression(Parser *parser) {
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
static NodeId ifElseExpression(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    expect(expression(parser));
    expect(expression(parser));
    if (check(*parser, NEW_LINES) && parser->next.kind == ELSE) advance(parser);
    consume(parser, ELSE, "an if else");
    expect(expression(parser));
    return ast_createNode(parser->ast, AST_IF_ELSE_EXPRESSION);
}

// not → "not" expression
static NodeId notExpression(Parser *parser, Token keyword) {
    assert(keyword.kind == NOT);
    expect(expression(parser));
    return ast_createNode(parser->ast, AST_NOT);
}

static AstKind getBinaryOperator(Token token) {
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
NodeId or (Parser * parser) {
    bind(left, and(parser));
    while (match(parser, OR)) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(and(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// and → equality ("and" equality)*
NodeId and (Parser * parser) {
    bind(left, equality(parser));
    while (match(parser, AND)) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(equality(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// equality → comparison (("=="|"!=") comparison)*
NodeId equality(Parser *parser) {
    bind(left, comparison(parser));
    while (match(parser, EQUAL_EQUAL) || match(parser, NOT_EQUAL)) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(comparison(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// comparison → term ((">"|">="|"<"|"<=") term)*
NodeId comparison(Parser *parser) {
    bind(left, term(parser));
    while (match(parser, LESS) || match(parser, LESS_EQUAL) ||
           match(parser, GREATER) || match(parser, GREATER_EQUAL)) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(term(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// term → factor (("+"|"-") factor)*
NodeId term(Parser *parser) {
    bind(left, factor(parser));
    while (match(parser, PLUS) || match(parser, MINUS)) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(factor(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// factor → unary (("*"|"/"|"%") unary)*
NodeId factor(Parser *parser) {
    bind(left, unary(parser));
    while (match(parser, STAR) || match(parser, SLASH) || match(parser, MODULO)
    ) {
        AstKind op = getBinaryOperator(parser->previous);
        expect(unary(parser));
        left = ast_createNode(parser->ast, op);
    }
    return left;
}

// unary → negation | addressOf | dererefence | unaryPostFix
NodeId unary(Parser *parser) {
    Token previous = parser->current;
    if (match(parser, MINUS)) return negation(parser, previous);
    if (match(parser, STAR)) return dereference(parser, previous);
    if (match(parser, AMPERSAND)) return addressOf(parser, previous);
    return unaryPostFix(parser);
}

// negation → "-" unary
NodeId negation(Parser *parser, Token operator) {
    assert(operator.kind == MINUS);
    expect(unary(parser));
    return ast_createNode(parser->ast, AST_NEGATION);
}

// address_of → "&" unary
static NodeId addressOf(Parser *parser, Token operator) {
    assert(operator.kind == AMPERSAND);
    expect(unary(parser));
    return ast_createNode(parser->ast, AST_ADDRESS_OF);
}

// dereference → "*" unary
static NodeId dereference(Parser *parser, Token operator) {
    assert(operator.kind == STAR);
    expect(unary(parser));
    return ast_createNode(parser->ast, AST_DEREFERENCE);
}

// unaryPostFix → call | fieldAccess | IndexAccess | primary
static NodeId unaryPostFix(Parser *parser) {
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

static NodeId callArgument(Parser *parser, AstCall *callData) {
    if (match(parser, MUT)) {
        ast_setBit(&callData->argumentsMutability, callData->numberOfArguments);
        advance(parser);
    }
    callData->numberOfArguments += 1;
    return expression(parser);
}

// call → unaryPostFix "(" callArgument (", " callArgument)*  ")"
static NodeId call(Parser *parser, NodeId accessed, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);
    AstCall callData = {.numberOfArguments = 0, .argumentsMutability = 0};

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

    NodeId id = ast_createNode(parser->ast, AST_CALL);
    *ast_getData(AstCall, parser->ast, id) = callData;
    return id;
}

// field_access → unaryPostFix "." IDENTFIER
static NodeId fieldAccess(Parser *parser, NodeId accessed, Token dot) {
    assert(dot.kind == DOT);
    expect(identifier(parser));
    return ast_createNode(parser->ast, AST_FIELD_ACCESS);
}

// indexAccess → unaryPostFix "[" expression "]"
static NodeId indexAccess(Parser *parser, NodeId accessed, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    expect(expression(parser));
    consume(parser, RIGHT_BRACKET, " a index access");
    return ast_createNode(parser->ast, AST_INDEX_ACCESS);
}

// primary → grouping
//         | array
//         | float
//         | integer
//         | boolean
//         | string
//         | structLiteral
//         | grouping
NodeId primary(Parser *parser) {
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
NodeId grouping(Parser *parser, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);
    bind(result, expression(parser));
    consume(parser, RIGHT_PAREN, "a grouping");
    return result;
}

// float → FLOAT
NodeId floatLiteral(Parser *parser, Token token) {
    assert(token.kind == FLOAT);
    NodeId id = ast_createNode(parser->ast, AST_FLOAT);
    ast_getData(AstFloat, parser->ast, id)->literal = token.literal;
    return id;
}

// integer → INTEGER
static NodeId integer(Parser *parser, Token token) {
    assert(token.kind == INTEGER);
    NodeId id = ast_createNode(parser->ast, AST_INTEGER);
    ast_getData(AstInteger, parser->ast, id)->literal = token.literal;
    return id;
}

// boolean → TRUE|FALSE
static NodeId boolean(Parser *parser, Token token) {
    assert(token.kind == TRUE || token.kind == FALSE);
    NodeId id = ast_createNode(parser->ast, AST_BOOLEAN);
    ast_getData(AstBoolean, parser->ast, id)->value = token.kind == TRUE;
    return id;
}

// identifier → IDENTIFIER
static NodeId identifier(Parser *parser) {
    consume(parser, IDENTIFIER, "an identifier");
    NodeId id = ast_createNode(parser->ast, AST_IDENTIFIER);
    ast_getData(AstIdentifier, parser->ast, id)->literal =
        parser->previous.literal;
    return id;
}

// string → STRING
static NodeId string(Parser *parser) {
    consume(parser, STRING, "a string");
    NodeId id = ast_createNode(parser->ast, AST_STRING);
    ast_getData(AstString, parser->ast, id)->literal = parser->previous.literal;
    return id;
}

// struct → IDENTIFIER "{" structField (","|("\n"+)))*  "}"
static NodeId structLiteral(Parser *parser, NodeId name, Token leftCurly) {
    assert(*list_get(parser->ast->nodes, name) == AST_IDENTIFIER);
    assert(leftCurly.kind == LEFT_CURLY);
    while (!atEnd(*parser) && !check(*parser, RIGHT_CURLY)) {
        consumeIfExists(parser, NEW_LINES);
        expect(identifier(parser));
        consume(parser, COLON, "a struct literal");
        expect(expression(parser));
        if (!match(parser, COMMA) && !match(parser, NEW_LINES)) break;
    }
    consume(parser, RIGHT_CURLY, "a struct literal");
    return ast_createNode(parser->ast, AST_STRUCT_LITERAL);
}

// array → "[" (expression ("," expression)*)* "]"
static NodeId array(Parser *parser, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    while (!check(*parser, RIGHT_BRACKET) && !atEnd(*parser)) {
        consumeIfExists(parser, NEW_LINES);
        expect(expression(parser));
        if (!match(parser, COMMA) && !match(parser, NEW_LINES)) break;
    }
    consume(parser, RIGHT_BRACKET, "an array");
    return ast_createNode(parser->ast, AST_ARRAY);
}