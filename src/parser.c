#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "error.h"
#include "token.h"
#include "types.h"

typedef struct {
    size_t current;
    SizeStack indentationLevel;
    TokenList tokens;
    Ast *ast;
} Parser;

static Token current(Parser parser) {
    assert(parser.current < parser.tokens.count);
    return parser.tokens.items[parser.current];
}

static Token previous(Parser parser) {
    assert(parser.current > 0);
    return parser.tokens.items[parser.current - 1];
}

static bool check(Parser parser, TokenKind kind) {
    return current(parser).kind == kind;
}

static Token peek(Parser parser) {
    assert(parser.current < parser.tokens.count - 1);
    return parser.tokens.items[parser.current + 1];
}

static bool atEnd(Parser parser) { return current(parser).kind == END_OF_FILE; }

static void advance(Parser *parser) {
    if (!atEnd(*parser)) {
        parser->current += 1;
    }
}

static bool match(Parser *parser, TokenKind token) {
    if (current(*parser).kind == token) {
        advance(parser);
        return true;
    }
    return false;
}

static bool couldBeExpression(Parser parser) {
    if (check(parser, NEW_LINE)) {
        if (peek(parser).column > stack_top(parser.indentationLevel)) {
            return true;
        } else return false;
    }
    return true;
}

static void advanceUntilNextStatement(Parser *parser) {
    if (!match(parser, NEW_LINE)) return;
    while (!atEnd(*parser) && current(*parser).kind == NEW_LINE) {
        advance(parser);
    }
}

static void advanceUntilNewline(Parser *parser) {
    while (!atEnd(*parser) && current(*parser).kind != NEW_LINE) {
        advance(parser);
    }
}

static NodeId createNode(Parser *parser, AstNode node) {
    return ast_createNode(parser->ast, node);
}

static void addError(Parser *parser, Error error) {
    error.line = current(*parser).line;
    error.column = current(*parser).column;
    if (error.kind == EXPECTING_LINE_ENDING) {
        error.expectingLineEnding.actualToken = current(*parser);
    }
    list_append(parser->ast->errors, error);
}

#define consume(parser, tokenKind, beingParsed)                          \
    if (current(*parser).kind != tokenKind) {                            \
        addError(                                                        \
            parser,                                                      \
            (Error){UNEXPECTED_TOKEN,                                    \
                    .unexpectedToken =                                   \
                        {tokenKind, current(*parser).kind, beingParsed}} \
        );                                                               \
        return None();                                                   \
    }                                                                    \
    advance(parser);

#define bind(name, expression)        \
    OptionalNodeId name = expression; \
    if (!hasValue(name)) return None();

OptionalNodeId program(Parser *parser);
OptionalNodeId block(Parser *parser);
OptionalNodeId function(Parser *parser, Token keyword);
OptionalNodeId typeAnnotation(Parser *parser, Token colon);
OptionalNodeId type(Parser *parser);
NodeIdList typeParameters(Parser *parser, Token leftBracket);
OptionalNodeId functionArgument(Parser *parser);
OptionalNodeId functionBody(Parser *parser);
OptionalNodeId interface(Parser *parser, Token keyword);
OptionalNodeId builtin(Parser *parser, Token keyword);
OptionalNodeId extern_stmt(Parser *parser, Token keyword);
OptionalNodeId link_with(Parser *parser, Token keyword);
OptionalNodeId typeDefinitionOrCase(Parser *parser, Token keyword);
OptionalNodeId statement(Parser *parser);
OptionalNodeId declaration(Parser *parser, NodeId identifier, Token operator);
OptionalNodeId assignment(Parser *parser, NodeId assignable, Token operator);
OptionalNodeId returnStmt(Parser *parser, Token keyword);
OptionalNodeId breakStmt(Parser *parser, Token keyword);
OptionalNodeId continueStmt(Parser *parser, Token keyword);
OptionalNodeId ifElse(Parser *parser, Token keyword);
OptionalNodeId whileStmt(Parser *parser, Token keyword);
OptionalNodeId useStmt(Parser *parser, Token keyword);
OptionalNodeId call(Parser *parser, NodeId callable, Token leftParen);
OptionalNodeId callArgument(Parser *parser);
OptionalNodeId expression(Parser *parser);
OptionalNodeId ifElseExpression(Parser *parser, Token keyword);
OptionalNodeId notExpr(Parser *parser, Token keyword);
OptionalNodeId newExpr(Parser *parser, Token keyword);
OptionalNodeId binary(Parser *parser);
OptionalNodeId or (Parser * parser);
OptionalNodeId and (Parser * parser);
OptionalNodeId equality(Parser *parser);
OptionalNodeId comparison(Parser *parser);
OptionalNodeId term(Parser *parser);
OptionalNodeId factor(Parser *parser);
OptionalNodeId unary(Parser *parser);
OptionalNodeId negation(Parser *parser, Token operator);
OptionalNodeId addressOf(Parser *parser, Token operator);
OptionalNodeId dereference(Parser *parser, Token operator);
OptionalNodeId unaryPostFix(Parser *parser);
OptionalNodeId primary(Parser *parser);
OptionalNodeId grouping(Parser *parser, Token leftParen);
OptionalNodeId float_expr(Parser *parser, Token token);
OptionalNodeId integer(Parser *parser, Token token);
OptionalNodeId boolean(Parser *parser, Token token);
OptionalNodeId identifier(Parser *parser);
OptionalNodeId string(Parser *parser);
OptionalNodeId interpolatedString(Parser *parser, Token left);
OptionalNodeId array(Parser *parser, Token leftBracket);
OptionalNodeId structLiteral(
    Parser *parser, NodeId identifier, Token leftCurly
);
OptionalNodeId structField(Parser *parser);
OptionalNodeId fieldAccess(Parser *parser, NodeId accessed, Token dot);
OptionalNodeId indexAccess(Parser *parser, NodeId accessed, Token leftBracket);
OptionalNodeId tokenAsIdentifier(Parser *parser, Token token);
OptionalNodeId asCallArgument(Parser *parser, NodeId expression);

void parse(Ast *ast) {
    Parser parser = {0, Stack(), ast->tokens, ast};
    (void)program(&parser);
}

OptionalNodeId program(Parser *parser) { return block(parser); }

OptionalNodeId block(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_BLOCK});
    AstNode *node = ast_getNode(*parser->ast, id);
    size_t initialErrorCount = parser->ast->errors.count;

    // Set new indentation level
    advanceUntilNextStatement(parser);
    if (parser->indentationLevel.count == 0) {
        stack_push(parser->indentationLevel, 1);
    } else {
        size_t previous = stack_top(parser->indentationLevel);
        stack_push(parser->indentationLevel, current(*parser).column);
        if (previous >= stack_top(parser->indentationLevel)) {
            addError(parser, (Error){EXPECTING_NEW_IDENTATION_LEVEL});
        }
    }

    while (true) {
        // Advance until next statement
        size_t currentBackup = parser->current;
        advanceUntilNextStatement(parser);
        if (atEnd(*parser)) break;

        // Check indentation
        if (current(*parser).column < stack_top(parser->indentationLevel)) {
            parser->current = currentBackup;
            break;
        } else if (current(*parser).column >
                   stack_top(parser->indentationLevel)) {
            addError(parser, (Error){UNEXPECTED_IDENTATION});
            while (!atEnd(*parser) && !check(*parser, NEW_LINE))
                advance(parser);
            continue;
        }

        // Parse statement
        OptionalNodeId result = statement(parser);
        if (hasValue(result)) {
            AstKind kind = ast_getNode(*parser->ast, result)->kind;
            if (kind == AST_FUNCTION || kind == AST_TYPE_DEFINITION) {
                list_append(node->block.definitions, result);
            } else if (kind == AST_IMPORT) {
                list_append(node->block.imports, result);
            } else {
                list_append(node->block.statements, result);
            }
        } else advanceUntilNewline(parser);

        // Check were at the end of a line
        if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
            addError(parser, (Error){EXPECTING_LINE_ENDING});
            advanceUntilNewline(parser);
        }
    }

    // Pop indentation level
    stack_pop(parser->indentationLevel);

    // Return
    if (initialErrorCount < parser->ast->errors.count) return None();
    else return id;
}

// typeAnnotation → ":" type
OptionalNodeId typeAnnotation(Parser *parser, Token colon) {
    assert(colon.kind == COLON);
    return type(parser);
}

// type → type ("[" type ("," type)* "]")*
OptionalNodeId type(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_TYPE});
    AstNode *node = ast_getNode(*parser->ast, id);
    consume(parser, IDENTIFIER, "a type");
    node->type.token = previous(*parser);
    if (match(parser, LEFT_BRACKET)) {
        while (!atEnd(*parser)) {
            bind(parameter, type(parser));
            list_append(node->type.parameters, parameter);
            if (!match(parser, COMMA)) break;
        }
        consume(parser, RIGHT_BRACKET, "a type");
    }
    return id;
}

// typeParameters → "[" identifier (, identifier)* "]"
NodeIdList typeParameters(Parser *parser, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    NodeIdList list = (NodeIdList)List();

    // Parse type parameters
    while (!atEnd(*parser) && !check(*parser, RIGHT_BRACKET)) {
        OptionalNodeId result = identifier(parser);
        if (!hasValue(result)) todo();
        list_append(list, result);

        if (!match(parser, COMMA)) break;
    }

    // Parse right bracket
    if (!match(parser, RIGHT_BRACKET)) todo();

    // Return
    return list;
}

// function → "function" IDENTIFIER type_parameters? "(" (functionArgument (":" type)? ",")* ")" (":" type)? block_statement_or_expression
OptionalNodeId function(Parser *parser, Token keyword) {
    assert(keyword.kind == FUNCTION);
    NodeId id = createNode(parser, (AstNode){.kind = AST_FUNCTION});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));
    node->function.identifier = name;

    // Parse type parameters
    if (match(parser, LEFT_BRACKET)) {
        node->function.typeParameters =
            typeParameters(parser, previous(*parser));
        if (node->function.typeParameters.count == 0) todo();
    }

    // Parse left paren
    consume(parser, LEFT_PAREN, "a function");

    // Parse arguments
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN, "a function");

    // Parse type
    if (match(parser, COLON)) {
        bind(type, typeAnnotation(parser, previous(*parser)));
        node->function.type = type;
    }

    // Parse body
    bind(body, functionBody(parser));
    node->function.body = body;

    // Return
    return id;
}

// functionArgument → MUT? identifier
OptionalNodeId functionArgument(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_FUNCTION_ARGUMENT});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse mutability
    if (match(parser, MUT)) {
        node->functionArgument.isMutable = true;
    }

    // Parse identifier
    bind(name, identifier(parser));
    node->functionArgument.identifier = name;

    // Parse type
    if (match(parser, COLON)) {
        bind(result, typeAnnotation(parser, previous(*parser)));
        node->functionArgument.type = result;
    }

    // Return
    return id;
}

// functionBody → block
OptionalNodeId functionBody(Parser *parser) { return block(parser); }

// interface → "interface" IDENTIFIER type_parameters "(" (function_argument ":"
// type) ",")* ")" ":" type
OptionalNodeId interface(Parser *parser, Token keyword) {
    assert(keyword.kind == INTERFACE);
    NodeId id = createNode(parser, (AstNode){.kind = AST_INTERFACE});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse type parameters
    consume(parser, LEFT_BRACKET, "an interface");
    todo();

    // Parse left paren
    consume(parser, LEFT_PAREN, "an interface");

    // Parse arguments
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN, "an interface");

    // Return
    return id;
}

// builtin → "builtin" IDENTIFIER type_parameters? "(" (function_argument ":"
// type) ",")* ")" ":" type
OptionalNodeId builtin(Parser *parser, Token keyword) {
    assert(keyword.kind == BUILTIN);
    NodeId id = createNode(parser, (AstNode){.kind = AST_BUILTIN});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN, "a builtin");

    // Parse arguments
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN, "a builtin");

    // Return
    return id;
}

// extern → "extern" IDENTIFIER "(" (IDENTIFIER ":" type "..."? ("," IDENTIFIER
// ":" type "..."?)*)? ")" ":" type
OptionalNodeId extern_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == EXTERN);
    NodeId id = createNode(parser, (AstNode){.kind = AST_EXTERN});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN, "a extern");

    // Parse arguments
    while (!atEnd(*parser) && !check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN, "a extern");

    // Return
    return id;
}

// link_with → "link_with" STRING
OptionalNodeId link_with(Parser *parser, Token keyword) {
    assert(keyword.kind == LINK_WITH);
    NodeId id = createNode(parser, (AstNode){.kind = AST_LINK_WITH});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse keyword
    consume(parser, LINK_WITH, "a link with");

    // Parse string
    bind(result, string(parser));
    node->linkWith.directives = result;

    // Return
    return id;
}

// typeDefinitionOrCase → ("type"|"case") IDENTIFIER (("\n"+ IDENTIFIER typeAnnoation)|(CASE IDENTIFIER typeDefinitionOrCase))*
OptionalNodeId typeDefinitionOrCase(Parser *parser, Token keyword) {
    assert(keyword.kind == TYPE || keyword.kind == CASE);
    AstKind kind = AST_TYPE_DEFINITION;
    if (keyword.kind == CASE) kind = AST_CASE_DEFINITION;
    NodeId id = createNode(parser, (AstNode){.kind = kind});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse indentifier
    bind(name, identifier(parser));
    node->typeDefinition.identifier = name;

    // Set new indentation level
    advanceUntilNextStatement(parser);
    if (parser->indentationLevel.count == 0) {
        stack_push(parser->indentationLevel, 1);
    } else {
        size_t previous = stack_top(parser->indentationLevel);
        stack_push(parser->indentationLevel, current(*parser).column);
        if (previous >= stack_top(parser->indentationLevel)) {
            addError(parser, (Error){EXPECTING_NEW_IDENTATION_LEVEL});
            return false;
        }
    }

    while (true) {
        // Advance until next statement
        size_t currentBackup = parser->current;
        advanceUntilNextStatement(parser);
        if (atEnd(*parser)) break;

        // Check indentation
        if (current(*parser).column < stack_top(parser->indentationLevel)) {
            parser->current = currentBackup;
            break;
        } else if (current(*parser).column >
                   stack_top(parser->indentationLevel)) {
            addError(parser, (Error){UNEXPECTED_IDENTATION});
            while (!atEnd(*parser) && !check(*parser, NEW_LINE))
                advance(parser);
            continue;
        }

        // Parse field or case
        if (check(*parser, IDENTIFIER)) {
            bind(field, identifier(parser));
            consume(parser, COLON, "a type definition");
            bind(type, typeAnnotation(parser, previous(*parser)));
            ast_setType(*parser->ast, field, type);
            list_append(node->typeDefinition.fields, field);

        } else if (match(parser, CASE)) {
            Token keyword = previous(*parser);
            bind(typeCase, typeDefinitionOrCase(parser, keyword));
            list_append(node->typeDefinition.cases, typeCase);
        }

        // Check were at the end of a line
        if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
            addError(parser, (Error){EXPECTING_LINE_ENDING});
            advanceUntilNewline(parser);
        }
    }

    // Pop indentation level
    stack_pop(parser->indentationLevel);

    // return
    return id;
}

// statement → function | interface | builtin | extern | link_with | type_definition
//           | return | ifElse | while | break | continue | use | declaration |
//           | assignment | call
OptionalNodeId statement(Parser *parser) {
    Token prev = current(*parser);
    if (match(parser, FUNCTION)) return function(parser, prev);
    if (match(parser, INTERFACE)) return interface(parser, prev);
    if (match(parser, BUILTIN)) return builtin(parser, prev);
    if (match(parser, EXTERN)) return extern_stmt(parser, prev);
    if (match(parser, LINK_WITH)) return link_with(parser, prev);
    if (match(parser, TYPE)) return typeDefinitionOrCase(parser, prev);
    if (match(parser, RETURN)) return returnStmt(parser, prev);
    if (match(parser, IF)) return ifElse(parser, prev);
    if (match(parser, WHILE)) return whileStmt(parser, prev);
    if (match(parser, BREAK)) return breakStmt(parser, prev);
    if (match(parser, CONTINUE)) return continueStmt(parser, prev);
    if (match(parser, USE)) return useStmt(parser, prev);
    if (match(parser, INCLUDE)) return useStmt(parser, prev);

    bind(result, unary(parser));
    AstNode *node = ast_getNode(*parser->ast, result);
    prev = current(*parser);
    if (node->kind == AST_IDENTIFIER) {
        if (match(parser, EQUAL)) return declaration(parser, result, prev);
        if (match(parser, BE)) return declaration(parser, result, prev);
        if (match(parser, COLON_EQUAL)) return assignment(parser, result, prev);
    } else if (match(parser, EQUAL)) {
        if (node->kind == AST_UNARY && node->unary.operator.kind == STAR)
            return assignment(parser, result, prev);
        if (node->kind == AST_FIELD_ACCESS)
            return assignment(parser, result, prev);
        if (node->kind == AST_INDEX_ACCESS)
            return assignment(parser, result, prev);
        if (node->kind == AST_CALL) return assignment(parser, result, prev);
    } else if (node->kind == AST_CALL) return result;

    addError(parser, (Error){EXPECTING_STATEMENT});
    return None();
}

// declaration → identifier ("be"|"=") expression (": " type)?
OptionalNodeId declaration(Parser *parser, NodeId identifier, Token operator) {
    assert(ast_getNode(*parser->ast, identifier)->kind == AST_IDENTIFIER);
    assert(operator.kind == EQUAL || operator.kind == BE);
    NodeId id = createNode(parser, (AstNode){AST_DECLARATION});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->declaration.identifier = identifier;
    node->declaration.isMutable = operator.kind == EQUAL;

    // Parse expression
    bind(result, expression(parser));
    node->declaration.expression = result;

    // Parse type annotation
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, previous(*parser)));
        ast_setType(*parser->ast, node->declaration.expression, annotation);
    }

    // Return
    return id;
}

OptionalNodeId assignment(Parser *parser, NodeId assignable, Token operator) {
    assert(operator.kind == COLON_EQUAL || operator.kind == EQUAL);
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->assignment.assignable = assignable;

    // Parse expression
    bind(result, expression(parser));
    node->assignment.expression = result;

    // Parse type annotation
    if (match(parser, COLON)) {
        bind(annotation, typeAnnotation(parser, previous(*parser)));
        ast_setType(*parser->ast, node->declaration.expression, annotation);
    }

    // Return
    return id;
}

// return → "return" expression?
OptionalNodeId returnStmt(Parser *parser, Token keyword) {
    assert(keyword.kind == RETURN);
    NodeId id = createNode(parser, (AstNode){AST_RETURN});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse expression
    if (!atEnd(*parser) && couldBeExpression(*parser)) {
        bind(result, expression(parser));
        node->returnNode.expression = result;
    }

    // Return
    return id;
}

OptionalNodeId breakStmt(Parser *parser, Token keyword) {
    assert(keyword.kind == BREAK);
    NodeId id = createNode(parser, (AstNode){AST_BREAK});
    return id;
}

OptionalNodeId continueStmt(Parser *parser, Token keyword) {
    assert(keyword.kind == CONTINUE);
    NodeId id = createNode(parser, (AstNode){AST_CONTINUE});
    return id;
}

// ifElse → "if" expression block ("\n"* "else" block)
OptionalNodeId ifElse(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    NodeId id = createNode(parser, (AstNode){AST_IF_ELSE});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse condition
    bind(condition, expression(parser));
    node->ifElse.condition = condition;

    // Parse if block
    bind(ifNode, block(parser));
    node->ifElse.ifNode = ifNode;

    // Parse else block
    size_t currentBackup = parser->current;
    advanceUntilNextStatement(parser);

    size_t indentationLevel = current(*parser).column;
    if (match(parser, ELSE)) {
        if (stack_top(parser->indentationLevel) == indentationLevel) {
            // Parse else block
            bind(elseNode, block(parser));
            node->ifElse.elseNode = elseNode;
        } else if (stack_top(parser->indentationLevel) < indentationLevel) {
            todo();
        } else if (stack_top(parser->indentationLevel) > indentationLevel) {
            todo();
        }
    } else {
        parser->current = currentBackup;
    }

    // Return
    return id;
}

// while → "while" expression block
OptionalNodeId whileStmt(Parser *parser, Token keyword) {
    assert(keyword.kind == WHILE);
    NodeId id = createNode(parser, (AstNode){AST_WHILE});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse condition
    bind(condition, expression(parser));
    node->whileNode.condition = condition;

    // Parse block
    bind(result, block(parser));
    node->whileNode.body = result;

    // Return
    return id;
}

// use → ("use"|"include") string
OptionalNodeId useStmt(Parser *parser, Token keyword) {
    assert(keyword.kind == INCLUDE || keyword.kind == USE);
    NodeId id = createNode(parser, (AstNode){AST_IMPORT});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Check keyword
    if (keyword.kind == INCLUDE) node->importNode.includes = true;
    else if (keyword.kind == USE) node->importNode.includes = false;

    // Parse path
    bind(path, string(parser));
    node->importNode.path = path;

    // Return
    return id;
}

// call → unaryPostFix "(" callArgument (", " callArgument)*  ")"
OptionalNodeId call(Parser *parser, NodeId callable, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->call.called = callable;

    // Parse arguments
    if (check(*parser, NEW_LINE) && couldBeExpression(*parser)) advance(parser);
    while (!check(*parser, RIGHT_PAREN) && !check(*parser, NEW_LINE)) {
        bind(argument, callArgument(parser));
        list_append(node->call.arguments, argument);
        if (check(*parser, COMMA)) advance(parser);
        else if (check(*parser, NEW_LINE) && couldBeExpression(*parser))
            advance(parser);
        else if (check(*parser, NEW_LINE) && peek(*parser).kind == RIGHT_PAREN)
            advance(parser);
    }

    // Consume right paren
    consume(parser, RIGHT_PAREN, "a call");

    // Return
    return id;
}

// callArgument → "mut"? expression
OptionalNodeId callArgument(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CALL_ARGUMENT});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse mut
    if (match(parser, MUT)) node->callArgument.isMutable = true;

    // Parse expression
    bind(result, expression(parser));
    node->callArgument.expression = result;

    // Return
    return id;
}

// expression → ifElseExpression
//            | new
//            | not
//            | binary
OptionalNodeId expression(Parser *parser) {
    if (check(*parser, NEW_LINE)) {
        if (!couldBeExpression(*parser)) {
            addError(parser, (Error){EXPECTING_NEW_IDENTATION_LEVEL});
            return None();
        }
        advance(parser);
    }
    Token curr = current(*parser);
    if (match(parser, IF)) return ifElseExpression(parser, curr);
    else if (match(parser, NEW)) return newExpr(parser, curr);
    else if (match(parser, NOT)) return notExpr(parser, curr);
    else return or (parser);
}

OptionalNodeId ifElseExpression(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    NodeId id = createNode(parser, (AstNode){AST_IF_ELSE_EXPR});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse condition
    bind(condition, expression(parser));
    node->ifElse.condition = condition;

    // Parse if branch
    bind(result, expression(parser));
    node->ifElse.ifNode = result;

    // Parse keyword
    if (check(*parser, NEW_LINE) && peek(*parser).kind == ELSE) advance(parser);
    consume(parser, ELSE, "an if else");

    // Parse else branch
    bind(result2, expression(parser));
    node->ifElse.elseNode = result2;

    // Return
    return id;
}

// not → "not" expression
OptionalNodeId notExpr(Parser *parser, Token keyword) {
    assert(keyword.kind == NOT);
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->call.called = tokenAsIdentifier(parser, keyword);

    // Parse expression
    bind(result, expression(parser));
    list_append(node->call.arguments, asCallArgument(parser, result));

    // Push node to ast
    return id;
}

// new → "new" expression
OptionalNodeId newExpr(Parser *parser, Token keyword) {
    assert(keyword.kind == NEW);
    NodeId id = createNode(parser, (AstNode){AST_NEW});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse expression
    bind(result, expression(parser));
    node->newNode.expression = result;

    // Return
    return id;
}

// binary → or
OptionalNodeId binary(Parser *parser) { return or (parser); }

// or → and ("or" and)*
OptionalNodeId or (Parser * parser) {
    bind(left, and(parser));

    if (check(*parser, NEW_LINE) && peek(*parser).kind == OR) advance(parser);
    while (match(parser, OR)) {
        Token operator= previous(*parser);
        bind(right, and(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;

        if (check(*parser, NEW_LINE) && peek(*parser).kind == OR)
            advance(parser);
    }

    return left;
}

// and → equality ("and" equality)*
OptionalNodeId and (Parser * parser) {
    bind(left, equality(parser));

    if (check(*parser, NEW_LINE) && peek(*parser).kind == AND) advance(parser);
    while (match(parser, AND)) {
        Token operator= previous(*parser);
        bind(right, equality(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;

        if (check(*parser, NEW_LINE) && peek(*parser).kind == AND)
            advance(parser);
    }

    return left;
}

// equality → comparison (("=="|"!=") comparison)*
OptionalNodeId equality(Parser *parser) {
    bind(left, comparison(parser));

    while (match(parser, EQUAL_EQUAL) || match(parser, NOT_EQUAL)) {
        Token operator= previous(*parser);
        bind(right, comparison(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;
    }

    return left;
}

// comparison → term ((">"|">="|"<"|"<=") term)*
OptionalNodeId comparison(Parser *parser) {
    bind(left, term(parser));

    while (match(parser, LESS) || match(parser, LESS_EQUAL) ||
           match(parser, GREATER) || match(parser, GREATER_EQUAL)) {
        Token operator= previous(*parser);
        bind(right, term(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;
    }

    return left;
}

// term → factor (("+"|"-") factor)*
OptionalNodeId term(Parser *parser) {
    bind(left, factor(parser));

    while (match(parser, PLUS) || match(parser, MINUS)) {
        Token operator= previous(*parser);
        bind(right, factor(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;
    }

    return left;
}

// factor → unary (("*"|"/"|"%") unary)*
OptionalNodeId factor(Parser *parser) {
    bind(left, unary(parser));

    while (match(parser, STAR) || match(parser, SLASH) || match(parser, MODULO)
    ) {
        Token operator= previous(*parser);
        bind(right, unary(parser));

        NodeId id = createNode(parser, (AstNode){AST_BINARY});
        AstNode *node = ast_getNode(*parser->ast, id);
        node->binary.operator= operator;
        node->binary.left = left;
        node->binary.right = right;
        left = id;
    }

    return left;
}

// unary → negation | addressOf | dererefence | call
OptionalNodeId unary(Parser *parser) {
    Token curr = current(*parser);
    if (match(parser, MINUS)) return negation(parser, curr);
    if (match(parser, STAR)) return dereference(parser, curr);
    if (match(parser, AMPERSAND)) return addressOf(parser, curr);
    return unaryPostFix(parser);
}

// negation → "-" unary
OptionalNodeId negation(Parser *parser, Token operator) {
    assert(operator.kind == MINUS);
    NodeId id = createNode(parser, (AstNode){AST_UNARY});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->unary.operator= operator;

    // Parse expression
    bind(result, unary(parser));
    node->unary.expression = result;

    // Return
    return id;
}

// address_of → "&" unary
OptionalNodeId addressOf(Parser *parser, Token operator) {
    assert(operator.kind == AMPERSAND);
    NodeId id = createNode(parser, (AstNode){AST_UNARY});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->unary.operator= operator;

    // Parse expression
    bind(result, unary(parser));
    node->unary.expression = result;

    // Return
    return id;
}

// dereference → "*" unary
OptionalNodeId dereference(Parser *parser, Token operator) {
    assert(operator.kind == STAR);
    NodeId id = createNode(parser, (AstNode){AST_UNARY});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->unary.operator= operator;

    // Parse expression
    bind(result, unary(parser));
    node->unary.expression = result;

    // Return
    return id;
}

// unaryPostFix → call | fieldAccess | IndexAccess | primary
OptionalNodeId unaryPostFix(Parser *parser) {
    bind(operand, primary(parser));

    while (true) {
        Token prev = current(*parser);
        if (match(parser, LEFT_PAREN)) operand = call(parser, operand, prev);
        else if (match(parser, DOT))
            operand = fieldAccess(parser, operand, prev);
        else if (match(parser, LEFT_BRACKET))
            operand = indexAccess(parser, operand, prev);
        else break;
    }

    return operand;
}

// primary → grouping
//         | array
//         | float
//         | integer
//         | boolean
//         | string
//         | interpolatedString
//         | structLiteral
//         | grouping
OptionalNodeId primary(Parser *parser) {
    Token prev = current(*parser);
    if (match(parser, MINUS)) return negation(parser, prev);
    if (match(parser, STAR)) return dereference(parser, prev);
    if (match(parser, AMPERSAND)) return addressOf(parser, prev);
    if (match(parser, LEFT_PAREN)) return grouping(parser, prev);
    if (match(parser, LEFT_BRACKET)) return array(parser, prev);
    if (match(parser, FLOAT)) return float_expr(parser, prev);
    if (match(parser, INTEGER)) return integer(parser, prev);
    if (match(parser, TRUE)) return boolean(parser, prev);
    if (match(parser, FALSE)) return boolean(parser, prev);
    if (check(*parser, STRING)) return string(parser);
    if (match(parser, STRING_LEFT)) return interpolatedString(parser, prev);
    if (check(*parser, IDENTIFIER)) {
        bind(id, identifier(parser));
        prev = current(*parser);
        if (match(parser, LEFT_CURLY)) return structLiteral(parser, id, prev);
        return id;
    }

    addError(
        parser,
        (Error){EXPECTING_EXPRESSION,
                .expectingExpression = {current(*parser).kind}}
    );
    return None();
}

// grouping → "(" expression ")"
OptionalNodeId grouping(Parser *parser, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);

    // Parse expression
    bind(result, expression(parser));

    // Parse right paren
    consume(parser, RIGHT_PAREN, "a grouping");

    return result;
}

// float → FLOAT
OptionalNodeId float_expr(Parser *parser, Token token) {
    assert(token.kind == FLOAT);
    NodeId id = createNode(parser, (AstNode){AST_FLOAT});
    ast_getNode(*parser->ast, id)->floatNode.value = token;
    return id;
}

// integer → INTEGER
OptionalNodeId integer(Parser *parser, Token token) {
    assert(token.kind == INTEGER);
    NodeId id = createNode(parser, (AstNode){AST_INTEGER});
    ast_getNode(*parser->ast, id)->integer.value = token;
    return id;
}

// boolean → TRUE|FALSE
OptionalNodeId boolean(Parser *parser, Token token) {
    assert(token.kind == TRUE || token.kind == FALSE);
    NodeId id = createNode(parser, (AstNode){AST_BOOLEAN});
    ast_getNode(*parser->ast, id)->boolean.value = token;
    return id;
}

// identifier → IDENTIFIER
OptionalNodeId identifier(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(*parser->ast, id)->identifier.value = current(*parser);
    consume(parser, IDENTIFIER, "an identifier");
    return id;
}

// string → STRING
OptionalNodeId string(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_STRING});
    ast_getNode(*parser->ast, id)->string.value = current(*parser);
    consume(parser, STRING, "a string");
    return id;
}

// interpolatedString → STRING_LEFT expression (STRING_MIDDLE expression)*
// STRING_RIGHT
OptionalNodeId interpolatedString(Parser *parser, Token left) {
    assert(left.kind == STRING_LEFT);
    NodeId id = createNode(parser, (AstNode){AST_INTERPOLATED_STRING});
    AstNode *node = ast_getNode(*parser->ast, id);
    list_append(node->interpolatedString.strings, left);

    while (true) {
        // Parse expression
        bind(result, expression(parser));
        list_append(node->interpolatedString.expressions, result);

        if (check(*parser, STRING_MIDDLE)) {
            list_append(node->interpolatedString.strings, current(*parser));
            consume(parser, STRING_MIDDLE, "an interpolated string");
        } else {
            break;
        }
    }

    // Parse string right
    list_append(node->interpolatedString.strings, current(*parser));
    consume(parser, STRING_RIGHT, "an interpolated string");

    // Return
    return id;
}

// array → "[" (expression ("," expression)*)* "]"
OptionalNodeId array(Parser *parser, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    NodeId id = createNode(parser, (AstNode){AST_ARRAY});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parse elements
    while (!check(*parser, RIGHT_BRACKET) && !atEnd(*parser)) {
        bind(result, expression(parser));
        list_append(node->arrayNode.elements, result);

        if (check(*parser, COMMA)) advance(parser);
        else if (check(*parser, NEW_LINE)) advanceUntilNextStatement(parser);
        else break;
    }

    // Parse right bracket
    consume(parser, RIGHT_BRACKET, "an array");

    // Return
    return id;
}

// struct → IDENTIFIER "{" structField (","|("\n"+)))*  "}"
OptionalNodeId structLiteral(
    Parser *parser, NodeId identifier, Token leftCurly
) {
    assert(ast_getNode(*parser->ast, identifier)->kind == AST_IDENTIFIER);
    assert(leftCurly.kind == LEFT_CURLY);
    NodeId id = createNode(parser, (AstNode){AST_STRUCT_LITERAL});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->structLiteral.identifier = identifier;

    // Parse fields
    while (!atEnd(*parser) && !check(*parser, RIGHT_CURLY)) {
        advanceUntilNextStatement(parser);

        bind(field, structField(parser));
        list_append(node->structLiteral.fields, field);

        if (!match(parser, COMMA) && !match(parser, NEW_LINE)) break;
    }

    // Parse right curly
    consume(parser, RIGHT_CURLY, "a struct literal");

    // Return
    return id;
}

OptionalNodeId structField(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_STRUCT_FIELD});
    AstNode *node = ast_getNode(*parser->ast, id);

    // Parser identifier
    bind(name, identifier(parser));
    node->structField.identifier = name;

    // Parse colon
    consume(parser, COLON, "a struct literal");

    // Parser expression
    bind(result, expression(parser));
    node->structField.expression = result;

    // Return
    return id;
}

// field_access → unaryPostFix "." IDENTFIER
OptionalNodeId fieldAccess(Parser *parser, NodeId accessed, Token dot) {
    assert(dot.kind == DOT);
    NodeId id = createNode(parser, (AstNode){AST_FIELD_ACCESS});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->fieldAccess.accessed = accessed;

    // Parse identifier
    bind(name, identifier(parser));
    node->fieldAccess.identifier = name;

    // Return
    return id;
}

// indexAccess → unaryPostFix "[" expression "]"
OptionalNodeId indexAccess(Parser *parser, NodeId accessed, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    NodeId id = createNode(parser, (AstNode){AST_INDEX_ACCESS});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->indexAccess.accessed = accessed;

    // Parse index expression
    bind(result, expression(parser));
    node->indexAccess.index = result;

    // Parse right bracket
    consume(parser, RIGHT_BRACKET, " a index access");

    // Return
    return id;
}

OptionalNodeId tokenAsIdentifier(Parser *parser, Token token) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(*parser->ast, id)->identifier.value = token;
    return id;
}

OptionalNodeId asCallArgument(Parser *parser, NodeId expression) {
    NodeId id = createNode(parser, (AstNode){AST_CALL_ARGUMENT});
    AstNode *node = ast_getNode(*parser->ast, id);
    node->callArgument.isMutable = false;
    node->callArgument.expression = expression;
    return id;
}