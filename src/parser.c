#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "error.h"
#include "token.h"
#include "types.h"

typedef struct {
    size_t current;
    SizeStack indentationLevel;
    TokenList tokens;
    Ast ast;
    ErrorList *errors;
    Token start;
} Parser;

static Token current(Parser parser) {
    assert(parser.current < parser.tokens.count);
    return parser.tokens.items[parser.current];
}

static Token previous(Parser parser) {
    assert(parser.current > 0);
    return parser.tokens.items[parser.current - 1];
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

static void advanceUntilNextStatement(Parser *parser) {
    if (!match(parser, NEW_LINE)) return;
    while (!atEnd(*parser) && current(*parser).kind == NEW_LINE) {
        advance(parser);
    }
}

static NodeId createNode(Parser *parser, AstNode node) {
    list_append(parser->ast.nodes, node);
    return parser->ast.nodes.count - 1;
}

#define consume(parser, tokenKind)                         \
    if (current(*parser).kind != tokenKind) return None(); \
    advance(parser);

#define bind(name, expression)        \
    OptionalNodeId name = expression; \
    if (!hasValue(name)) return None();

static bool check(Parser parser, TokenKind kind) {
    return current(parser).kind == kind;
}

static void addError(Parser *parser, Error error) {
    error.line = current(*parser).line;
    error.column = current(*parser).column;
    list_append((*parser->errors), error);
}

OptionalNodeId program(Parser *parser);
OptionalNodeId block(Parser *parser);
OptionalNodeId function(Parser *parser, Token keyword);
OptionalNodeId functionArgument(Parser *parser);
OptionalNodeId functionBody(Parser *parser);
OptionalNodeId interface(Parser *parser, Token keyword);
OptionalNodeId builtin(Parser *parser, Token keyword);
OptionalNodeId extern_stmt(Parser *parser, Token keyword);
OptionalNodeId link_with(Parser *parser, Token keyword);
OptionalNodeId typeDefinition(Parser *parser, Token keyword);
bool typeDefinitionBody(Parser *parser, AstNode *node);
OptionalNodeId statement(Parser *parser);
OptionalNodeId declaration(Parser *parser, NodeId identifier, Token operator);
OptionalNodeId assignment(Parser *parser, NodeId assignable, Token operator);
OptionalNodeId fieldAssignment(Parser *parser, NodeId identifier);
OptionalNodeId dereferenceAssignment(Parser *parser, Token operator);
OptionalNodeId indexAssignment(Parser *parser, NodeId indexAccess);
OptionalNodeId return_stmt(Parser *parser, Token keyword);
OptionalNodeId break_stmt(Parser *parser, Token keyword);
OptionalNodeId continue_stmt(Parser *parser, Token keyword);
OptionalNodeId if_else(Parser *parser, Token keyword);
OptionalNodeId while_stmt(Parser *parser, Token keyword);
OptionalNodeId useStmt(Parser *parser, Token keyword);
OptionalNodeId call(Parser *parser, NodeId callable);
OptionalNodeId callArgument(Parser *parser);
OptionalNodeId expression(Parser *parser);
OptionalNodeId ifElseExpr(Parser *parser, Token keyword);
OptionalNodeId notExpr(Parser *parser, Token keyword);
OptionalNodeId newExpr(Parser *parser, Token keyword);
OptionalNodeId binary(Parser *parser);
OptionalNodeId or (Parser * parser);
OptionalNodeId and (Parser * parser);
OptionalNodeId equality(Parser *parser);
OptionalNodeId comparison(Parser *parser);
OptionalNodeId term(Parser *parser);
OptionalNodeId factor(Parser *parser);
OptionalNodeId primary(Parser *parser);
OptionalNodeId negation(Parser *parser, Token operator);
OptionalNodeId addressOf(Parser *parser, Token operator);
OptionalNodeId dereference(Parser *parser, Token operator);
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
OptionalNodeId fieldAccess(Parser *parser, NodeId accessed);
OptionalNodeId indexAccess(Parser *parser, NodeId accessed);
OptionalNodeId tokenAsIdentifier(Parser *parser, Token token);
OptionalNodeId asCallArgument(Parser *parser, NodeId expression);

Ast parse(TokenList source, ErrorList *errors) {
    Parser parser =
        {0, Stack(), source, {.nodes = List()}, errors, source.items[0]};
    (void)program(&parser);
    return parser.ast;
}

OptionalNodeId program(Parser *parser) { return block(parser); }

OptionalNodeId block(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_BLOCK});
    AstNode *node = ast_getNode(parser->ast, id);
    size_t initialErrorCount = parser->errors->count;

    // Set new indentation level
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
            AstKind kind = ast_getNode(parser->ast, result)->kind;
            if (kind == AST_FUNCTION || kind == AST_TYPE_DEF) {
                list_append(node->block.definitions, result);
            } else if (kind == AST_IMPORT) {
                list_append(node->block.imports, result);
            } else {
                list_append(node->block.statements, result);
            }
        }

        // Check were at the end of a line
        if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
            todo();
        }
    }

    // Pop indentation level
    stack_pop(parser->indentationLevel);

    // Return
    if (initialErrorCount < parser->errors->count) return None();
    else return id;
}

// function → "function" IDENTIFIER type_parameters? "(" (functionArgument (":"
// type)? ",")* ")" (":" type)? block_statement_or_expression
OptionalNodeId function(Parser *parser, Token keyword) {
    assert(keyword.kind == FUNCTION);
    NodeId id = createNode(parser, (AstNode){.kind = AST_FUNCTION});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN);

    // Parse arguments
    while (!check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    // Parse body
    bind(body, functionBody(parser));

    // Return
    return id;
}

// functionArgument → MUT? identifier
OptionalNodeId functionArgument(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_FUNCTION_ARGUMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse mutability
    if (match(parser, MUT)) {
        node->functionArgument.isMutable = true;
    }

    // Parse identifier
    bind(name, identifier(parser));
    node->functionArgument.identifier = name;

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
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN);

    // Parse arguments
    while (!check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    // Return
    return id;
}

// builtin → "builtin" IDENTIFIER type_parameters? "(" (function_argument ":"
// type) ",")* ")" ":" type
OptionalNodeId builtin(Parser *parser, Token keyword) {
    assert(keyword.kind == BUILTIN);
    NodeId id = createNode(parser, (AstNode){.kind = AST_BUILTIN});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN);

    // Parse arguments
    while (!check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    // Return
    return id;
}

// extern → "extern" IDENTIFIER "(" (IDENTIFIER ":" type "..."? ("," IDENTIFIER
// ":" type "..."?)*)? ")" ":" type
OptionalNodeId extern_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == EXTERN);
    NodeId id = createNode(parser, (AstNode){.kind = AST_EXTERN});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));

    // Parse left paren
    consume(parser, LEFT_PAREN);

    // Parse arguments
    while (!check(*parser, RIGHT_PAREN)) {
        bind(argument, functionArgument(parser));
        list_append(node->function.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    // Return
    return id;
}

// link_with → "link_with" STRING
OptionalNodeId link_with(Parser *parser, Token keyword) {
    assert(keyword.kind == LINK_WITH);
    NodeId id = createNode(parser, (AstNode){.kind = AST_LINK_WITH});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, LINK_WITH);

    // Parse string
    bind(result, string(parser));
    node->linkWith.directives = result;

    // Return
    return id;
}

// type_definition → "type" IDENTIFIER ("\n"+ IDENTIFIER ": " type)*
OptionalNodeId typeDefinition(Parser *parser, Token keyword) {
    assert(keyword.kind == TYPE);
    NodeId id = createNode(parser, (AstNode){.kind = AST_TYPE_DEF});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, TYPE);

    // Parse indentifier
    bind(name, identifier(parser));
    node->type.identifier = name;

    // Parse body
    bool result = typeDefinitionBody(parser, node);
    if (!result) return None();

    // return
    return id;
}

// type_definition_body →  (("\n"+ IDENTIFIER ": " type)|(CASE IDENTIFIER
// type_defintion_body))*
bool typeDefinitionBody(Parser *parser, AstNode *node) { todo(); }

// statement → function
//           | interface
//           | builtin
//           | extern
//           | link_with
//           | type_definition
//           | return
//           | if_else
//           | while
//           | break
//           | continue
//           | use
//           | dereferenceAssignment
//           | declaration
//           | assignment
//           | call
//           | fieldAssignment
//           | indexAssignment
OptionalNodeId statement(Parser *parser) {
    Token curr = current(*parser);
    if (match(parser, FUNCTION)) return function(parser, curr);
    if (match(parser, INTERFACE)) return interface(parser, curr);
    if (match(parser, BUILTIN)) return builtin(parser, curr);
    if (match(parser, EXTERN)) return extern_stmt(parser, curr);
    if (match(parser, LINK_WITH)) return link_with(parser, curr);
    if (match(parser, TYPE)) return typeDefinition(parser, curr);
    if (match(parser, RETURN)) return return_stmt(parser, curr);
    if (match(parser, IF)) return if_else(parser, curr);
    if (match(parser, WHILE)) return while_stmt(parser, curr);
    if (match(parser, BREAK)) return break_stmt(parser, curr);
    if (match(parser, CONTINUE)) return continue_stmt(parser, curr);
    if (match(parser, USE)) return useStmt(parser, curr);
    if (match(parser, INCLUDE)) return useStmt(parser, curr);
    if (match(parser, STAR)) return dereferenceAssignment(parser, curr);
    if (check(*parser, IDENTIFIER)) {
        bind(name, identifier(parser));
        curr = current(*parser);
        if (match(parser, EQUAL)) return declaration(parser, name, curr);
        if (match(parser, BE)) return declaration(parser, name, curr);
        if (match(parser, COLON_EQUAL)) return assignment(parser, name, curr);
        if (match(parser, LEFT_PAREN)) return call(parser, name);
        else todo();
    }
    todo();
}

// declaration → identifier ("be"|"=") expression (": " type)?
OptionalNodeId declaration(Parser *parser, NodeId identifier, Token operator) {
    assert(ast_getNode(parser->ast, identifier)->kind == AST_IDENTIFIER);
    assert(operator.kind == EQUAL || operator.kind == BE);
    NodeId id = createNode(parser, (AstNode){AST_DECLARATION});
    AstNode *node = ast_getNode(parser->ast, id);
    node->declaration.identifier = identifier;

    // Parse expression
    bind(result, expression(parser));
    node->declaration.expression = result;

    // Parse type annotation
    if (check(*parser, COLON)) {
        todo();
    }

    // Return
    return id;
}

OptionalNodeId assignment(Parser *parser, NodeId assignable, Token operator) {
    assert(operator.kind == COLON_EQUAL);
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);
    node->assignment.assignable = assignable;

    // Parse expression
    bind(result, expression(parser));
    node->assignment.expression = result;

    // Parse type annotation
    if (check(*parser, COLON)) {
        todo();
    }

    // Return
    return id;
}

// fieldAssignment → fieldAssignment "=" expression (":" type)?
OptionalNodeId fieldAssignment(Parser *parser, NodeId identifier) {
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    node->declaration.identifier = identifier;

    // Parse equal
    consume(parser, EQUAL);

    // Parse expression
    bind(result, expression(parser));
    node->declaration.expression = result;

    // Parse type annotation
    if (check(*parser, COLON)) {
        todo();
    }

    // Return
    return id;
}

// dereferenceAssignment → dereference "=" expression (":" type)?
OptionalNodeId dereferenceAssignment(Parser *parser, Token operator) {
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(identifier, dereference(parser, operator));
    node->assignment.assignable = identifier;

    // Parse equal
    consume(parser, EQUAL);

    // Parse expression
    bind(result, expression(parser));
    node->declaration.expression = result;

    // Parse type annotation
    if (check(*parser, COLON)) {
        todo();
    }

    // Return
    return id;
}

// indexAssignment → index_access "=" expression (":" type)?
OptionalNodeId indexAssignment(Parser *parser, NodeId indexAccess) {
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    node->assignment.assignable = indexAccess;

    // Parse equal
    consume(parser, EQUAL);

    // Parse expression
    bind(result, expression(parser));
    node->declaration.expression = result;

    // Parse type annotation
    if (check(*parser, COLON)) {
        todo();
    }

    // Return
    return id;
}

// return → "return" expression?
OptionalNodeId return_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == RETURN);
    NodeId id = createNode(parser, (AstNode){AST_RETURN});
    AstNode *node = ast_getNode(parser->ast, id);
    node->returnNode.expression = None();

    // Parse expression
    if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
        bind(result, expression(parser));
        node->returnNode.expression = result;
    }

    // Return
    return id;
}

OptionalNodeId break_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == BREAK);
    NodeId id = createNode(parser, (AstNode){AST_BREAK});
    return id;
}

OptionalNodeId continue_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == CONTINUE);
    NodeId id = createNode(parser, (AstNode){AST_CONTINUE});
    return id;
}

// if_else → "if" expression block ("\n"* "else" block)
OptionalNodeId if_else(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    NodeId id = createNode(parser, (AstNode){AST_IF_ELSE});
    AstNode *node = ast_getNode(parser->ast, id);
    node->ifElse.elseNode = None();

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
OptionalNodeId while_stmt(Parser *parser, Token keyword) {
    assert(keyword.kind == WHILE);
    NodeId id = createNode(parser, (AstNode){AST_WHILE});
    AstNode *node = ast_getNode(parser->ast, id);

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
    AstNode *node = ast_getNode(parser->ast, id);

    // Check keyword
    if (keyword.kind == INCLUDE) node->importNode.includes = true;
    else if (keyword.kind == USE) node->importNode.includes = false;

    // Parse path
    bind(path, string(parser));
    node->importNode.path = path;

    // Return
    return id;
}

OptionalNodeId call(Parser *parser, NodeId callable) {
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);
    node->call.callable = callable;

    // Parse arguments
    while (!check(*parser, RIGHT_PAREN)) {
        bind(argument, callArgument(parser));
        list_append(node->call.arguments, argument);
        if (!match(parser, COMMA)) break;
    }

    // Consume right paren
    consume(parser, RIGHT_PAREN);

    // Return
    return id;
}

OptionalNodeId callArgument(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CALL_ARGUMENT});
    AstNode *node = ast_getNode(parser->ast, id);
    node->callArgument.isMutable = false;
    node->callArgument.identifier = None();

    // Parse expression
    bind(result, expression(parser));
    node->callArgument.expression = result;

    // Return
    return id;
}

// expression → if_else_expression
//            | new
//            | not
//            | binary
OptionalNodeId expression(Parser *parser) {
    advanceUntilNextStatement(parser);
    Token curr = current(*parser);
    if (match(parser, IF)) return ifElseExpr(parser, curr);
    else if (match(parser, NEW)) return newExpr(parser, curr);
    else if (match(parser, NOT)) return notExpr(parser, curr);
    else return or (parser);
}

OptionalNodeId ifElseExpr(Parser *parser, Token keyword) {
    assert(keyword.kind == IF);
    NodeId id = createNode(parser, (AstNode){AST_IF_ELSE});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, IF);

    // Parse condition
    advanceUntilNextStatement(parser);
    bind(condition, expression(parser));
    node->ifElse.condition = condition;

    // Parse if branch
    advanceUntilNextStatement(parser);
    bind(result, expression(parser));
    node->ifElse.ifNode = result;

    // Parse keyword
    advanceUntilNextStatement(parser);
    consume(parser, ELSE);

    // Parse else branch
    advanceUntilNextStatement(parser);
    bind(result2, expression(parser));
    node->ifElse.elseNode = result;

    // Return
    return id;
}

// not → "not" expression
OptionalNodeId notExpr(Parser *parser, Token keyword) {
    assert(keyword.kind == NOT);
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);
    node->call.callable = tokenAsIdentifier(parser, keyword);

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
    AstNode *node = ast_getNode(parser->ast, id);

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

    while (match(parser, OR)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, and(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// and → equality ("and" equality)*
OptionalNodeId and (Parser * parser) {
    bind(left, equality(parser));

    while (match(parser, AND)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, equality(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// equality → comparison (("=="|"!=") comparison)*
OptionalNodeId equality(Parser *parser) {
    bind(left, comparison(parser));

    while (match(parser, EQUAL_EQUAL) || match(parser, NOT_EQUAL)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, comparison(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// comparison → term ((">"|">="|"<"|"<=") term)*
OptionalNodeId comparison(Parser *parser) {
    bind(left, term(parser));

    while (match(parser, LESS) || match(parser, LESS_EQUAL) ||
           match(parser, GREATER) || match(parser, GREATER_EQUAL)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, term(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// term → factor (("+"|"-") factor)*
OptionalNodeId term(Parser *parser) {
    bind(left, factor(parser));

    while (match(parser, PLUS) || match(parser, MINUS)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, factor(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// factor → primary (("*"|"/"|"%") primary)*
OptionalNodeId factor(Parser *parser) {
    bind(left, primary(parser));

    while (match(parser, STAR) || match(parser, SLASH)) {
        NodeId operator= tokenAsIdentifier(parser, previous(*parser));
        bind(right, primary(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.callable = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// primary → negation
//         | dereference
//         | address_of
//         | grouping
//         | array
//         | float
//         | integer
//         | boolean
//         | string
//         | interpolatedString
//         | call
//         | struct
//         | grouping
OptionalNodeId primary(Parser *parser) {
    Token curr = current(*parser);
    if (match(parser, MINUS)) return negation(parser, curr);
    else if (match(parser, STAR)) return dereference(parser, curr);
    else if (match(parser, AMPERSAND)) return addressOf(parser, curr);
    else if (match(parser, LEFT_PAREN)) return grouping(parser, curr);
    else if (match(parser, LEFT_BRACKET)) return array(parser, curr);
    else if (match(parser, FLOAT)) return float_expr(parser, curr);
    else if (match(parser, INTEGER)) return integer(parser, curr);
    else if (match(parser, TRUE)) return boolean(parser, curr);
    else if (match(parser, FALSE)) return boolean(parser, curr);
    else if (match(parser, STRING)) return string(parser);
    else if (match(parser, STRING_LEFT))
        return interpolatedString(parser, curr);
    else if (check(*parser, IDENTIFIER)) {
        bind(name, identifier(parser));
        curr = current(*parser);
        if (match(parser, LEFT_CURLY)) return structLiteral(parser, name, curr);
        if (match(parser, LEFT_PAREN)) return call(parser, name);
        else return identifier(parser);
    }
    todo();
}

// negation → "-" primary
OptionalNodeId negation(Parser *parser, Token operator) {
    assert(operator.kind == MINUS);
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);
    node->call.callable = tokenAsIdentifier(parser, operator);

    // Parse expression
    bind(result, primary(parser));
    list_append(node->call.arguments, asCallArgument(parser, result));

    // Return
    return id;
}

// address_of → "&" (field_access|identifier|indexAccess)
OptionalNodeId addressOf(Parser *parser, Token operator) {
    assert(operator.kind == AMPERSAND);
    NodeId id = createNode(parser, (AstNode){AST_ADDRESS_OF});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse expression
    bind(result, primary(parser));
    list_append(node->call.arguments, result);

    // Return
    return id;
}

// dereference → "*" (dereference|assignable)
OptionalNodeId dereference(Parser *parser, Token operator) {
    assert(operator.kind == STAR);
    NodeId id = createNode(parser, (AstNode){AST_DEREFERENCE});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse expression
    bind(result, primary(parser));
    list_append(node->call.arguments, result);

    // Return
    return id;
}

// grouping → "(" expression ")"
OptionalNodeId grouping(Parser *parser, Token leftParen) {
    assert(leftParen.kind == LEFT_PAREN);

    // Parse expression
    bind(result, expression(parser));

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    return result;
}

// float → FLOAT
OptionalNodeId float_expr(Parser *parser, Token token) {
    assert(token.kind == FLOAT);
    NodeId id = createNode(parser, (AstNode){AST_FLOAT});
    ast_getNode(parser->ast, id)->floatNode.value = token;
    return id;
}

// integer → INTEGER
OptionalNodeId integer(Parser *parser, Token token) {
    assert(token.kind == INTEGER);
    NodeId id = createNode(parser, (AstNode){AST_INTEGER});
    ast_getNode(parser->ast, id)->integer.value = token;
    return id;
}

// boolean → TRUE|FALSE
OptionalNodeId boolean(Parser *parser, Token token) {
    assert(token.kind == TRUE || token.kind == FALSE);
    NodeId id = createNode(parser, (AstNode){AST_BOOLEAN});
    ast_getNode(parser->ast, id)->boolean.value = token;
    return id;
}

// identifier → IDENTIFIER
OptionalNodeId identifier(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(parser->ast, id)->identifier.value = current(*parser);
    consume(parser, IDENTIFIER);
    return id;
}

// string → STRING
OptionalNodeId string(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_STRING});
    ast_getNode(parser->ast, id)->string.value = current(*parser);
    consume(parser, STRING);
    return id;
}

// interpolatedString → STRING_LEFT expression (STRING_MIDDLE expression)*
// STRING_RIGHT
OptionalNodeId interpolatedString(Parser *parser, Token left) {
    assert(left.kind == STRING_LEFT);
    NodeId id = createNode(parser, (AstNode){AST_INTERPOLATED_STRING});
    AstNode *node = ast_getNode(parser->ast, id);
    list_append(node->interpolatedString.strings, left);

    while (true) {
        // Parse expression
        bind(result, expression(parser));
        list_append(node->interpolatedString.expressions, result);

        if (check(*parser, STRING_MIDDLE)) {
            list_append(node->interpolatedString.strings, current(*parser));
            consume(parser, STRING_MIDDLE);
        } else {
            break;
        }
    }

    // Parse string right
    list_append(node->interpolatedString.strings, current(*parser));
    consume(parser, STRING_RIGHT);

    // Return
    return id;
}

// array → "[" (expression ("," expression)*)* "]"
OptionalNodeId array(Parser *parser, Token leftBracket) {
    assert(leftBracket.kind == LEFT_BRACKET);
    NodeId id = createNode(parser, (AstNode){AST_ARRAY});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse elements
    while (!check(*parser, RIGHT_BRACKET) && !atEnd(*parser)) {
        bind(result, expression(parser));
        list_append(node->arrayNode.elements, result);

        if (check(*parser, COMMA)) advance(parser);
        else if (check(*parser, NEW_LINE)) advanceUntilNextStatement(parser);
        else break;
    }

    // Parse right bracket
    consume(parser, RIGHT_BRACKET);

    // Return
    return id;
}

// struct → IDENTIFIER "{" structField (","|("\n"+)))*  "}"
OptionalNodeId structLiteral(
    Parser *parser, NodeId identifier, Token leftCurly
) {
    assert(ast_getNode(parser->ast, identifier)->kind == AST_IDENTIFIER);
    assert(leftCurly.kind == LEFT_CURLY);
    NodeId id = createNode(parser, (AstNode){AST_STRUCT_LITERAL});
    AstNode *node = ast_getNode(parser->ast, id);
    node->structLiteral.identifier = identifier;

    // Parse fields
    while (!check(*parser, RIGHT_CURLY) && !atEnd(*parser)) {
        advanceUntilNextStatement(parser);

        bind(field, structField(parser));

        if (!match(parser, COMMA) || !match(parser, NEW_LINE)) break;
    }

    // Parse right curly
    consume(parser, RIGHT_CURLY);

    // Return
    return id;
}

OptionalNodeId structField(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_STRUCT_FIELD});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parser identifier
    bind(name, identifier(parser));
    node->structField.identifier = name;

    // Parse colon
    consume(parser, COLON);

    // Parser expression
    bind(result, expression(parser));
    node->structField.expression = name;

    // Return
    return id;
}

// field_access → expression "." IDENTFIER
OptionalNodeId fieldAccess(Parser *parser, NodeId accessed) {
    NodeId id = createNode(parser, (AstNode){AST_FIELD_ACCESS});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse dot
    consume(parser, DOT);

    // Parse identifier
    bind(name, identifier(parser));
    node->fieldAccess.identifier = name;

    // Return
    return id;
}

// indexAccess → assignable "[" expression "]"
OptionalNodeId indexAccess(Parser *parser, NodeId accessed) {
    NodeId id = createNode(parser, (AstNode){AST_INDEX_ACCESS});
    AstNode *node = ast_getNode(parser->ast, id);
    node->indexAccess.accessed = accessed;

    // Parse left bracket
    consume(parser, LEFT_BRACKET);

    // Parse index expression
    bind(result, expression(parser));
    node->indexAccess.index = result;

    // Parse right bracket
    consume(parser, RIGHT_BRACKET);

    // Return
    return id;
}

OptionalNodeId tokenAsIdentifier(Parser *parser, Token token) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(parser->ast, id)->identifier.value = token;
    return id;
}

OptionalNodeId asCallArgument(Parser *parser, NodeId expression) {
    NodeId id = createNode(parser, (AstNode){AST_CALL_ARGUMENT});
    AstNode *node = ast_getNode(parser->ast, id);
    node->callArgument.isMutable = false;
    node->callArgument.identifier = None();
    node->callArgument.expression = expression;
    return id;
}