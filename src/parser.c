#include "parser.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "common.h"
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

static Token peek(Parser parser) {
    assert(parser.current + 1 < parser.tokens.count);
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

#define operatorArgument(parser, name, toParse)                       \
    OptionalNodeId name = None();                                     \
    do {                                                              \
        NodeId id = createNode(parser, (AstNode){AST_CALL_ARGUMENT}); \
        AstNode *node = ast_getNode(parser->ast, id);                 \
        node->callArgument.isMutable = false;                         \
        node->callArgument.identifier = None();                       \
        bind(name##_expression, toParse(parser));                     \
        node->callArgument.expression = name##_expression;            \
        name = id;                                                    \
    } while (false);

static bool check(Parser parser, TokenKind kind) {
    return current(parser).kind == kind;
}

OptionalNodeId program(Parser *parser);
OptionalNodeId block(Parser *parser);
OptionalNodeId function(Parser *parser);
OptionalNodeId functionArgument(Parser *parser);
OptionalNodeId functionBody(Parser *parser);
OptionalNodeId interface(Parser *parser);
OptionalNodeId builtin(Parser *parser);
OptionalNodeId extern_stmt(Parser *parser);
OptionalNodeId link_with(Parser *parser);
OptionalNodeId typeDefinition(Parser *parser);
bool typeDefinitionBody(Parser *parser, AstNode *node);
OptionalNodeId statement(Parser *parser);
OptionalNodeId declaration(Parser *parser);
OptionalNodeId assignment(Parser *parser);
OptionalNodeId fieldAssignment(Parser *parser, NodeId identifier);
OptionalNodeId dereferenceAssignment(Parser *parser);
OptionalNodeId indexAssignment(Parser *parser, NodeId indexAccess);
OptionalNodeId return_stmt(Parser *parser);
OptionalNodeId break_stmt(Parser *parser);
OptionalNodeId continue_stmt(Parser *parser);
OptionalNodeId if_else(Parser *parser);
OptionalNodeId while_stmt(Parser *parser);
OptionalNodeId useStmt(Parser *parser);
OptionalNodeId call(Parser *parser);
OptionalNodeId callArgument(Parser *parser);
OptionalNodeId expression(Parser *parser);
OptionalNodeId ifElseExpr(Parser *parser);
OptionalNodeId notExpr(Parser *parser);
OptionalNodeId newExpr(Parser *parser);
OptionalNodeId binary(Parser *parser);
OptionalNodeId or (Parser * parser);
OptionalNodeId and (Parser * parser);
OptionalNodeId equality(Parser *parser);
OptionalNodeId comparison(Parser *parser);
OptionalNodeId term(Parser *parser);
OptionalNodeId factor(Parser *parser);
OptionalNodeId primary(Parser *parser);
OptionalNodeId negation(Parser *parser);
OptionalNodeId address_of(Parser *parser);
OptionalNodeId dereference(Parser *parser);
OptionalNodeId grouping(Parser *parser);
OptionalNodeId float_expr(Parser *parser);
OptionalNodeId integer(Parser *parser);
OptionalNodeId boolean(Parser *parser);
OptionalNodeId identifier(Parser *parser);
OptionalNodeId tokenAsIdentifier(Parser *parser, TokenKind token);
OptionalNodeId string(Parser *parser);
OptionalNodeId interpolatedString(Parser *parser);
OptionalNodeId array(Parser *parser);
OptionalNodeId structLiteral(Parser *parser);
OptionalNodeId structField(Parser *parser);
OptionalNodeId fieldAccess(Parser *parser, NodeId accessed);
OptionalNodeId indexAccess(Parser *parser, NodeId accessed);

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

    // Initial setps
    size_t initialErrorCount = parser->errors->count;
    advanceUntilNextStatement(parser);

    // Set new indentation level
    if (parser->indentationLevel.count == 0) {
        stack_push(parser->indentationLevel, current(*parser).column);
    } else {
        size_t previous = stack_top(parser->indentationLevel);
        stack_push(parser->indentationLevel, current(*parser).column);
        if (previous >= stack_top(parser->indentationLevel)) {
            todo();
        }
    }

    // Parse statements, defintions and imports
    while (!atEnd(*parser)) {
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

        if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
            Token curr = current(*parser);
            printf(
                "%s %zu %zu\n",
                token_getLiteral(curr),
                curr.line,
                curr.column
            );
            todo();
        } else {
            // Advance until next statement and check indentation
            size_t currentBackup = parser->current;
            advanceUntilNextStatement(parser);
            if (atEnd(*parser)) break;
            if (current(*parser).column < stack_top(parser->indentationLevel)) {
                parser->current = currentBackup;
                break;
            } else if (current(*parser).column >
                       stack_top(parser->indentationLevel)) {
                todo();
                while (!atEnd(*parser) && !check(*parser, NEW_LINE))
                    advance(parser);
            }
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
OptionalNodeId function(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_FUNCTION});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, FUNCTION);

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
OptionalNodeId interface(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_INTERFACE});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, INTERFACE);

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
OptionalNodeId builtin(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_BUILTIN});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, INTERFACE);

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
OptionalNodeId extern_stmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){.kind = AST_EXTERN});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, INTERFACE);

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
OptionalNodeId link_with(Parser *parser) {
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
OptionalNodeId typeDefinition(Parser *parser) {
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
    if (check(*parser, FUNCTION)) return function(parser);
    if (check(*parser, INTERFACE)) return interface(parser);
    if (check(*parser, BUILTIN)) return builtin(parser);
    if (check(*parser, EXTERN)) return extern_stmt(parser);
    if (check(*parser, LINK_WITH)) return link_with(parser);
    if (check(*parser, TYPE)) return typeDefinition(parser);
    if (check(*parser, RETURN)) return return_stmt(parser);
    if (check(*parser, IF)) return if_else(parser);
    if (check(*parser, WHILE)) return while_stmt(parser);
    if (check(*parser, BREAK)) return break_stmt(parser);
    if (check(*parser, CONTINUE)) return continue_stmt(parser);
    if (check(*parser, USE)) return useStmt(parser);
    if (check(*parser, INCLUDE)) return useStmt(parser);
    if (check(*parser, STAR)) return dereferenceAssignment(parser);
    if (check(*parser, INCLUDE)) return useStmt(parser);
    if (check(*parser, IDENTIFIER)) {
        if (peek(*parser).kind == EQUAL) return declaration(parser);
        if (peek(*parser).kind == BE) return declaration(parser);
        if (peek(*parser).kind == COLON_EQUAL) return assignment(parser);
        if (peek(*parser).kind == LEFT_PAREN) return call(parser);
        else todo();
    }
    todo();
}

// declaration → IDENTIFIER ("be"|"=") expression (": " type)?
OptionalNodeId declaration(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_DECLARATION});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));
    node->declaration.identifier = name;

    // Parse equal or be
    if (match(parser, EQUAL)) node->declaration.isMutable = true;
    else if (match(parser, BE)) node->declaration.isMutable = false;
    else unreachable();

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

OptionalNodeId assignment(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));
    node->assignment.assignable = name;

    // Parse equal
    consume(parser, COLON_EQUAL);

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
OptionalNodeId dereferenceAssignment(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_ASSIGNMENT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(identifier, dereference(parser));
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
OptionalNodeId return_stmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_RETURN});
    AstNode *node = ast_getNode(parser->ast, id);
    node->returnNode.expression = None();

    // Parse keyword
    consume(parser, RETURN);

    // Parse expression
    if (!atEnd(*parser) && !check(*parser, NEW_LINE)) {
        bind(result, expression(parser));
        node->returnNode.expression = result;
    }

    // Return
    return id;
}

OptionalNodeId break_stmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_BREAK});
    consume(parser, BREAK);
    return id;
}

OptionalNodeId continue_stmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CONTINUE});
    consume(parser, CONTINUE);
    return id;
}

// if_else → "if" expression block ("\n"* "else" block)
OptionalNodeId if_else(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_IF_ELSE});
    AstNode *node = ast_getNode(parser->ast, id);
    node->ifElse.elseNode = None();

    // Parse keyword
    consume(parser, IF);

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
OptionalNodeId while_stmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_WHILE});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    consume(parser, WHILE);

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
OptionalNodeId useStmt(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_IMPORT});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse keyword
    if (match(parser, INCLUDE)) node->importNode.includes = true;
    else if (match(parser, USE)) node->importNode.includes = false;
    else todo();

    // Parse path
    bind(path, string(parser));
    node->importNode.path = path;

    // Return
    return id;
}

OptionalNodeId call(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, identifier(parser));
    node->call.identifier = name;

    // Consume left paren
    consume(parser, LEFT_PAREN);

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
    if (check(*parser, IF)) return ifElseExpr(parser);
    else if (check(*parser, NEW)) return newExpr(parser);
    else if (check(*parser, NOT)) return notExpr(parser);
    else return or (parser);
}

OptionalNodeId ifElseExpr(Parser *parser) {
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

OptionalNodeId notExpr(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse indentifier
    bind(result, tokenAsIdentifier(parser, NOT));
    node->call.identifier = result;

    // Parse expression
    operatorArgument(parser, result2, expression);
    list_append(node->call.arguments, result2);

    // Push node to ast
    return id;
}

// new → "new" expression
OptionalNodeId newExpr(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_NEW});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse token
    consume(parser, NEW);

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

    while (check(*parser, OR)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, and(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// and → equality ("and" equality)*
OptionalNodeId and (Parser * parser) {
    bind(left, equality(parser));

    while (check(*parser, AND)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, equality(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// equality → comparison (("=="|"!=") comparison)*
OptionalNodeId equality(Parser *parser) {
    bind(left, comparison(parser));

    while (check(*parser, EQUAL_EQUAL) || check(*parser, NOT_EQUAL)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, comparison(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// comparison → term ((">"|">="|"<"|"<=") term)*
OptionalNodeId comparison(Parser *parser) {
    bind(left, term(parser));

    while (check(*parser, LESS) || check(*parser, LESS_EQUAL) ||
           check(*parser, GREATER) || check(*parser, GREATER_EQUAL)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, term(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// term → factor (("+"|"-") factor)*
OptionalNodeId term(Parser *parser) {
    bind(left, factor(parser));

    while (check(*parser, PLUS) || check(*parser, MINUS)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, factor(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
        list_append(node->call.arguments, left);
        list_append(node->call.arguments, right);
        left = id;
    }

    return left;
}

// factor → primary (("*"|"/"|"%") primary)*
OptionalNodeId factor(Parser *parser) {
    bind(left, primary(parser));

    while (check(*parser, STAR) || check(*parser, SLASH)) {
        bind(operator, tokenAsIdentifier(parser, current(*parser).kind));
        bind(right, primary(parser));

        NodeId id = createNode(parser, (AstNode){AST_CALL});
        AstNode *node = ast_getNode(parser->ast, id);
        node->call.identifier = operator;
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
    if (check(*parser, MINUS)) return negation(parser);
    else if (check(*parser, STAR)) return dereference(parser);
    else if (check(*parser, AMPERSAND)) return address_of(parser);
    else if (check(*parser, LEFT_PAREN)) return grouping(parser);
    else if (check(*parser, LEFT_BRACKET)) return array(parser);
    else if (check(*parser, FLOAT)) return float_expr(parser);
    else if (check(*parser, INTEGER)) return integer(parser);
    else if (check(*parser, TRUE)) return boolean(parser);
    else if (check(*parser, FALSE)) return boolean(parser);
    else if (check(*parser, STRING)) return string(parser);
    else if (check(*parser, STRING_LEFT)) return interpolatedString(parser);
    else if (check(*parser, IDENTIFIER)) {
        if (peek(*parser).kind == LEFT_CURLY) return structLiteral(parser);
        if (peek(*parser).kind == LEFT_PAREN) return call(parser);
        else return identifier(parser);
    }
    todo();
}

// negation → "-" primary
OptionalNodeId negation(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_CALL});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, tokenAsIdentifier(parser, MINUS));
    node->call.identifier = name;

    // Parse expression
    operatorArgument(parser, result, primary);
    list_append(node->call.arguments, result);

    // Return
    return id;
}

// address_of → "&" (field_access|identifier|indexAccess)
OptionalNodeId address_of(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_ADDRESS_OF});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, tokenAsIdentifier(parser, AMPERSAND));
    node->call.identifier = name;

    // Parse expression
    bind(result, primary(parser));
    list_append(node->call.arguments, result);

    // Return
    return id;
}

// dereference → "*" (dereference|assignable)
OptionalNodeId dereference(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_DEREFERENCE});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse identifier
    bind(name, tokenAsIdentifier(parser, STAR));
    node->call.identifier = name;

    // Parse expression
    bind(result, primary(parser));
    list_append(node->call.arguments, result);

    // Return
    return id;
}

// grouping → "(" expression ")"
OptionalNodeId grouping(Parser *parser) {
    // Parse left paren
    consume(parser, LEFT_PAREN);

    // Parse expression
    bind(result, expression(parser));

    // Parse right paren
    consume(parser, RIGHT_PAREN);

    return result;
}

// float → FLOAT
OptionalNodeId float_expr(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_FLOAT});
    ast_getNode(parser->ast, id)->floatNode.value = current(*parser);
    consume(parser, FLOAT);
    return id;
}

// integer → INTEGER
OptionalNodeId integer(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_INTEGER});
    ast_getNode(parser->ast, id)->integer.value = current(*parser);
    consume(parser, INTEGER);
    return id;
}

// boolean → TRUE|FALSE
OptionalNodeId boolean(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_BOOLEAN});
    ast_getNode(parser->ast, id)->boolean.value = current(*parser);
    if (check(*parser, TRUE)) {
        consume(parser, TRUE);
    } else if (check(*parser, FALSE)) {
        consume(parser, FALSE);
    }
    return id;
}

// identifier → IDENTIFIER
OptionalNodeId identifier(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(parser->ast, id)->identifier.value = current(*parser);
    consume(parser, IDENTIFIER);
    return id;
}

OptionalNodeId tokenAsIdentifier(Parser *parser, TokenKind token) {
    NodeId id = createNode(parser, (AstNode){AST_IDENTIFIER});
    ast_getNode(parser->ast, id)->identifier.value = current(*parser);
    consume(parser, token);
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
OptionalNodeId interpolatedString(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_INTERPOLATED_STRING});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse string left
    list_append(node->interpolatedString.strings, current(*parser));
    consume(parser, STRING_LEFT);

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
OptionalNodeId array(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_ARRAY});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse left bracket
    consume(parser, LEFT_BRACKET);

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
OptionalNodeId structLiteral(Parser *parser) {
    NodeId id = createNode(parser, (AstNode){AST_STRUCT_LITERAL});
    AstNode *node = ast_getNode(parser->ast, id);

    // Parse indentifier
    bind(name, identifier(parser));
    node->structLiteral.identifier = name;

    // Parse left curly
    consume(parser, LEFT_CURLY);

    // Parse fields
    while (!check(*parser, RIGHT_CURLY) && !atEnd(*parser)) {
        advanceUntilNextStatement(parser);

        bind(field, structLiteral(parser));

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
