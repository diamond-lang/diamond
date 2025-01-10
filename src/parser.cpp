#include "parser.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#include "ast.hpp"
#include "errors.hpp"
#include "shared.hpp"
#include "tokens.hpp"

namespace parse {
    struct Parser {
        size_t current = 0;
        std::vector<size_t> indentation_level;
        std::filesystem::path &file;
        std::vector<token::Token> tokens;
        ast::Ast &ast;
        std::vector<Error> errors;
    };

    std::optional<ast::Node *> parse_program(Parser &parser);
    std::optional<ast::Node *> parse_block(Parser &parser);
    std::optional<std::vector<ast::TypeParameter>> parse_type_parameters(
        Parser &parser
    );
    std::optional<ast::Node *> parse_function_argument(Parser &parser);
    std::optional<ast::Node *> parse_function(Parser &parser);
    std::optional<ast::Node *> parse_interface(Parser &parser);
    std::optional<ast::Node *> parse_builtin(Parser &parser);
    std::optional<ast::Node *> parse_extern(Parser &parser);
    std::optional<ast::Node *> parse_link_with(Parser &parser);
    std::optional<ast::Node *> parse_type_definition(Parser &parser);
    std::optional<Ok> parse_type_definition_body(
        Parser &parser, ast::TypeNode *node
    );
    std::optional<ast::Type> parse_type(Parser &parser);
    std::optional<ast::InterfaceType> parse_interface_type(Parser &parser);
    std::optional<ast::Node *> parse_statement(Parser &parser);
    std::optional<ast::Node *> parse_block_or_statement(Parser &parser);
    std::optional<ast::Node *> parse_block_statement_or_expression(
        Parser &parser
    );
    std::optional<ast::Node *> parse_declaration(Parser &parser);
    std::optional<ast::Node *> parse_assignment(Parser &parser);
    std::optional<ast::Node *> parse_field_assignment(
        Parser &parser, ast::Node *identifier
    );
    std::optional<ast::Node *> parse_dereference_assignment(Parser &parser);
    std::optional<ast::Node *> parse_index_assignment(
        Parser &parser, ast::Node *index_access
    );
    std::optional<ast::Node *> parse_return_stmt(Parser &parser);
    std::optional<ast::Node *> parse_break_stmt(Parser &parser);
    std::optional<ast::Node *> parse_continue_stmt(Parser &parser);
    std::optional<ast::Node *> parse_if_else(Parser &parser);
    std::optional<ast::Node *> parse_while_stmt(Parser &parser);
    std::optional<ast::Node *> parse_use_stmt(Parser &parser);
    std::optional<ast::Node *> parse_call_argument(Parser &parser);
    std::optional<ast::Node *> parse_call(
        Parser &parser, ast::Node *identifier
    );
    std::optional<ast::Node *> parse_struct(Parser &parser);
    std::optional<ast::Node *> parse_expression(Parser &parser);
    std::optional<ast::Node *> parse_if_else_expr(Parser &parser);
    std::optional<ast::Node *> parse_not_expr(Parser &parser);
    std::optional<ast::Node *> parse_new_expr(Parser &parser);
    std::optional<ast::Node *> parse_binary(Parser &parser, int precedence = 1);
    std::optional<ast::Node *> parse_primary(Parser &parser);
    std::optional<ast::Node *> parse_grouping_or_assignable(Parser &parser);
    std::optional<ast::Node *> parse_negation(Parser &parser);
    std::optional<ast::Node *> parse_address_of(Parser &parser);
    std::optional<ast::Node *> parse_dereference(Parser &parser);
    std::optional<ast::Node *> parse_grouping(Parser &parser);
    std::optional<ast::Node *> parse_float(Parser &parser);
    std::optional<ast::Node *> parse_integer(Parser &parser);
    std::optional<ast::Node *> parse_boolean(Parser &parser);
    std::optional<ast::Node *> parse_identifier(Parser &parser);
    std::optional<ast::Node *> parse_function_identifier(Parser &parser);
    std::optional<ast::Node *> parse_identifier(Parser &parser, size_t token);
    std::optional<ast::Node *> parse_string(Parser &parser);
    std::optional<ast::Node *> parse_interpolated_string(Parser &parser);
    std::optional<ast::Node *> parse_array(Parser &parser);
    std::optional<ast::Node *> parse_field_access(
        Parser &parser, ast::Node *accessed
    );
    std::optional<ast::Node *> parse_index_access(
        Parser &parser, ast::Node *expression
    );
    std::optional<token::Token> parse_token(Parser &parser, size_t token);

    token::Token current(Parser &parser);
    size_t current_indentation(Parser &parser);
    void advance(Parser &parser);
    void advance_until_next_statement(Parser &parser);
    bool at_end(Parser &parser);
    bool sequenceMatch(Parser &parser, std::vector<size_t> tokens);
    Location location(Parser &parser);
};

token::Token parse::current(Parser &parser) {
    assert(parser.current < parser.tokens.size());
    return parser.tokens[parser.current];
}

size_t parse::current_indentation(Parser &parser) {
    assert(parser.indentation_level.size() > 0);
    return parser.indentation_level[parser.indentation_level.size() - 1];
}

void parse::advance(Parser &parser) {
    if (!at_end(parser)) {
        parser.current += 1;
    }
}

void parse::advance_until_next_statement(Parser &parser) {
    while (!at_end(parser)
           && std::holds_alternative<token::NewLine>(current(parser).kind)) {
        advance(parser);
    }
}

bool parse::at_end(Parser &parser) {
    return std::holds_alternative<token::EndOfFile>(current(parser).kind);
}

bool parse::sequenceMatch(Parser &parser, std::vector<size_t> tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (parser.current + i >= parser.tokens.size()
            || parser.tokens[parser.current + i].kind.index() != tokens[i]) {
            return false;
        }
    }
    return true;
}

#define match(expression)                             \
    do {                                              \
        auto result = expression;                     \
        if (!result.has_value()) return std::nullopt; \
    } while (false);

#define bind(name, expression)                                 \
    decltype(expression)::value_type name;                     \
    do {                                                       \
        auto name##_optional = expression;                     \
        if (!name##_optional.has_value()) return std::nullopt; \
        name = name##_optional.value();                        \
    } while (false);

Location parse::location(Parser &parser) {
    return Location(current(parser).line, current(parser).column, parser.file);
}

std::variant<ast::Ast, Errors> parse::program(
    const std::vector<token::Token> &tokens, const std::filesystem::path &file
) {
    ast::Ast ast;
    std::filesystem::path module_path
        = std::filesystem::canonical(std::filesystem::current_path() / file);

    auto parser = Parser{
        .file = module_path,
        .tokens = tokens,
        .ast = ast,
    };
    auto parsing_result = parse_program(parser);
    if (!parsing_result.has_value()) return parser.errors;

    ast.module_path = module_path;
    ast.program = (ast::BlockNode *)parsing_result.value();
    ast.modules[module_path.string()]
        = (ast::BlockNode *)parsing_result.value();

    return ast;
}

std::variant<Ok, Errors> parse::module(
    ast::Ast &ast,
    const std::vector<token::Token> &tokens,
    std::filesystem::path &file
) {
    auto parser = Parser{
        .file = file,
        .tokens = tokens,
        .ast = ast,
    };
    auto parsing_result = parse_block(parser);
    if (!parsing_result.has_value()) return parser.errors;

    ast.modules[file.string()] = (ast::BlockNode *)parsing_result.value();
    return Ok{};
}

// program → block
std::optional<ast::Node *> parse::parse_program(Parser &parser) {
    return parse_block(parser);
}

// block → statement*
std::optional<ast::Node *> parse::parse_block(Parser &parser) {
    bool there_was_errors = false;

    // Create node
    ast::BlockNode block
        = {.line = current(parser).line, .column = current(parser).column};

    // Advance until next statement
    advance_until_next_statement(parser);

    // Set new indentation level
    if (parser.indentation_level.size() == 0) {
        parser.indentation_level.push_back(1);
    } else {
        size_t previous = current_indentation(parser);
        parser.indentation_level.push_back(current(parser).column);
        if (previous >= current_indentation(parser)) {
            parser.errors.push_back(
                errors::expecting_new_indentation_level(location(parser))
            );
            return std::nullopt;
        }
    }

    // Main loop, parses line by line
    while (!at_end(parser)) {
        // Advance until next statement
        size_t backup = parser.current;
        advance_until_next_statement(parser);
        if (at_end(parser)) break;

        // Check indentation
        if (current(parser).column < current_indentation(parser)) {
            parser.current = backup;
            break;
        } else if (current(parser).column > current_indentation(parser)) {
            parser.errors.push_back(errors::unexpected_indent(location(parser))
            );
            there_was_errors = true;
            while (
                !at_end(parser)
                && !std::holds_alternative<token::NewLine>(current(parser).kind)
            )
                advance(parser);  // advances until new line
            continue;
        }

        // Parse statement
        auto result = parse_statement(parser);
        if (result.has_value()) {
            if (std::holds_alternative<ast::FunctionNode>(*result.value())) {
                block.functions.push_back((ast::FunctionNode *)result.value());
            } else if (std::holds_alternative<ast::InterfaceNode>(*result.value(
                       ))) {
                block.interfaces.push_back((ast::InterfaceNode *)result.value()
                );
            } else if (std::holds_alternative<ast::TypeNode>(*result.value())) {
                block.types.push_back((ast::TypeNode *)result.value());
            } else if (std::holds_alternative<ast::UseNode>(*result.value())) {
                block.use_statements.push_back((ast::UseNode *)result.value());
            } else if (std::holds_alternative<ast::LinkWithNode>(*result.value()
                       )) {
                parser.ast.link_with.push_back(
                    std::get<ast::LinkWithNode>(*result.value())
                        .directives->value
                );
            } else {
                // If we are printing an interpolated string desugares it
                if (std::holds_alternative<ast::CallNode>(*result.value())
                    && std::get<ast::CallNode>(*result.value())
                               .identifier->value
                           == "print"
                    && std::get<ast::CallNode>(*result.value()).args.size() == 1
                    && std::holds_alternative<ast::InterpolatedStringNode>(
                        *std::get<ast::CallNode>(*result.value())
                             .args[0]
                             ->expression
                    )) {
                    ast::CallNode &print_call
                        = std::get<ast::CallNode>(*result.value());
                    ast::InterpolatedStringNode &interpolated_string
                        = std::get<ast::InterpolatedStringNode>(
                            *std::get<ast::CallNode>(*result.value())
                                 .args[0]
                                 ->expression
                        );
                    for (size_t i = 0; i < interpolated_string.strings.size();
                         i++) {
                        // print string
                        auto call
                            = ast::CallNode{print_call.line, print_call.column};

                        auto identifier = ast::IdentifierNode{
                            print_call.line,
                            print_call.column
                        };
                        identifier.value = "printWithoutLineEnding";
                        if (i + 1 == interpolated_string.strings.size()) {
                            identifier.value = "print";
                        }
                        parser.ast.push_back(identifier);
                        call.identifier
                            = (ast::IdentifierNode *)parser.ast.last_element();

                        auto string_node = ast::StringNode{
                            interpolated_string.line,
                            interpolated_string.column
                        };
                        string_node.value = interpolated_string.strings[i];
                        parser.ast.push_back(string_node);

                        auto call_argument_node = ast::CallArgumentNode{
                            interpolated_string.line,
                            interpolated_string.column
                        };
                        call_argument_node.expression
                            = parser.ast.last_element();
                        parser.ast.push_back(call_argument_node);
                        call.args.push_back(
                            (ast::CallArgumentNode *)parser.ast.last_element()
                        );

                        parser.ast.push_back(call);
                        block.statements.push_back(parser.ast.last_element());

                        // print expression
                        if (i + 1 != interpolated_string.strings.size()) {
                            call = ast::CallNode{
                                print_call.line,
                                print_call.column
                            };

                            identifier = ast::IdentifierNode{
                                print_call.line,
                                print_call.column
                            };
                            identifier.value = "printWithoutLineEnding";
                            parser.ast.push_back(identifier);
                            call.identifier = (ast::IdentifierNode *)
                                                  parser.ast.last_element();

                            call_argument_node = ast::CallArgumentNode{
                                interpolated_string.line,
                                interpolated_string.column
                            };
                            call_argument_node.expression
                                = interpolated_string.expressions[i];
                            parser.ast.push_back(call_argument_node);
                            call.args.push_back((ast::CallArgumentNode *)
                                                    parser.ast.last_element());

                            parser.ast.push_back(call);
                            block.statements.push_back(parser.ast.last_element()
                            );
                        }
                    }
                }
                // just push the statement
                else {
                    block.statements.push_back(result.value());
                }

                if (!at_end(parser)
                    && !std::holds_alternative<token::NewLine>(
                        current(parser).kind
                    )) {
                    parser.errors.push_back(
                        errors::unexpected_character(location(parser))
                    );
                    there_was_errors = true;
                }
            }
        } else {
            there_was_errors = true;
        }

        // Advance until new line
        while (!at_end(parser)
               && !std::holds_alternative<token::NewLine>(current(parser).kind))
            advance(parser);
    }

    // Pop indentation level
    parser.indentation_level.pop_back();

    // Return
    if (there_was_errors)
        return std::nullopt;
    else {
        parser.ast.push_back(block);
        return parser.ast.last_element();
    }
}

// type_parameters → "[" identifier (":" interface_type)? ("," identifier (":"
// interface_type)?)? "]"
std::optional<std::vector<ast::TypeParameter>> parse::parse_type_parameters(
    Parser &parser
) {
    std::vector<ast::TypeParameter> type_parameters;

    // Parse left bracket
    match(parse_token(parser, getIndex<token::Kind, token::LeftBracket>()));

    // Parse type parameters
    while (!std::holds_alternative<token::RightBracket>(current(parser).kind)
           && !at_end(parser)) {
        bind(parameter, parse_type(parser));
        assert(parameter.is_final_type_variable());

        ast::TypeParameter type_parameter;
        type_parameter.type = parameter;

        if (std::holds_alternative<token::Colon>(current(parser).kind)) {
            advance(parser);
            bind(interface_type, parse_interface_type(parser));
            type_parameter.interface.insert(interface_type);
        }

        type_parameters.push_back(type_parameter);

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    // Parse right bracket
    match(parse_token(parser, getIndex<token::Kind, token::RightBracket>()));

    // Return
    return type_parameters;
}

std::optional<ast::Node *> parse::parse_function_argument(Parser &parser) {
    // Create node
    auto function_argument = ast::FunctionArgumentNode{
        current(parser).line,
        current(parser).column
    };

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        function_argument.is_mutable = true;
        advance(parser);
    }

    // Parse indentifier
    bind(identifier, parse_identifier(parser));
    function_argument.identifier = (ast::IdentifierNode *)identifier;

    parser.ast.push_back(function_argument);
    return parser.ast.last_element();
}

// function → "function" IDENTIFIER type_parameters? "(" (function_argument (":"
// type)? ",")* ")" (":" type)? block_statement_or_expression
std::optional<ast::Node *> parse::parse_function(Parser &parser) {
    // Create node
    auto function
        = ast::FunctionNode{current(parser).line, current(parser).column};
    function.module_path = parser.file;

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Function>()));

    // Parse indentifier
    bind(identifier, parse_function_identifier(parser));
    function.identifier = (ast::IdentifierNode *)identifier;

    // Parse possible type parameters
    if (std::holds_alternative<token::LeftBracket>(current(parser).kind)) {
        bind(type_parameters, parse_type_parameters(parser));
        function.type_parameters = type_parameters;
    }

    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        bind(arg, parse_function_argument(parser));

        // Parse type annotation
        if (std::holds_alternative<token::Colon>(current(parser).kind)) {
            advance(parser);
            bind(type, parse_type(parser));
            ast::set_type(arg, type);
        }
        function.args.push_back((ast::FunctionArgumentNode *)arg);

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    // Parse right paren
    match(parse_token(parser, getIndex<token::Kind, token::RightParen>()));

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        // Parse mut
        if (std::holds_alternative<token::Mut>(current(parser).kind)) {
            function.return_type_is_mutable = true;
            advance(parser);
        }
        bind(type, parse_type(parser));
        function.return_type = type;
    }

    // Parse body
    bind(body, parse_block_statement_or_expression(parser));
    function.body = body;

    if (!ast::is_expression(function.body)
        && ast::could_be_expression(function.body)) {
        ast::transform_to_expression(function.body);
    }

    // Check if function is completely typed or not
    for (auto arg : function.args) {
        if (arg->type == ast::Type(ast::NoType{})) {
            function.state = ast::FunctionNotAnalyzed;
            break;
        }
    }
    if (function.return_type == ast::Type(ast::NoType{})) {
        function.state = ast::FunctionNotAnalyzed;
    }

    if (function.state == ast::FunctionCompletelyTyped) {
        if (function.type_parameters.size() > 0) {
            function.state = ast::FunctionGenericCompletelyTyped;
        }
    }

    // Check if function is - with just one argument
    if (function.identifier->value == "-" && function.args.size() == 1) {
        function.identifier->value = "-:negation";
    } else if (function.identifier->value == "[]"
               && function.args[0]->is_mutable) {
        function.identifier->value = "[]:mut";
    }

    parser.ast.push_back(function);
    return parser.ast.last_element();
}

// interface → "interface" IDENTIFIER type_parameters "(" (function_argument ":"
// type) ",")* ")" ":" type
std::optional<ast::Node *> parse::parse_interface(Parser &parser) {
    // Create node
    auto interface = ast::InterfaceNode{
        current(parser).line,
        current(parser).column
    };
    interface.module_path = parser.file;

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Interface>()));

    // Parse indentifier
    bind(identifier, parse_function_identifier(parser));
    interface.identifier = (ast::IdentifierNode *)identifier;

    // Parse type parameters
    bind(type_parameters, parse_type_parameters(parser));
    interface.type_parameters = type_parameters;

    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        bind(arg, parse_function_argument(parser));

        // Parse type annotation
        match(parse_token(parser, getIndex<token::Kind, token::Colon>()));

        bind(type, parse_type(parser));
        ast::set_type(arg, type);

        // Add argument
        interface.args.push_back((ast::FunctionArgumentNode *)arg);

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    // Parse right paren
    match(parse_token(parser, getIndex<token::Kind, token::RightParen>()));

    // Parse type annotation
    match(parse_token(parser, getIndex<token::Kind, token::Colon>()));

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        interface.return_type_is_mutable = true;
        advance(parser);
    }

    bind(type, parse_type(parser));
    interface.return_type = type;

    // Check if interface is - with just one argument
    if (interface.identifier->value == "-" && interface.args.size() == 1) {
        interface.identifier->value = "-:negation";
    } else if (interface.identifier->value == "[]"
               && interface.args[0]->is_mutable) {
        interface.identifier->value = "[]:mut";
    }

    parser.ast.push_back(interface);
    return parser.ast.last_element();
}

// builtin → "builtin" IDENTIFIER type_parameters? "(" (function_argument ":"
// type) ",")* ")" ":" type
std::optional<ast::Node *> parse::parse_builtin(Parser &parser) {
    // Create node
    auto builtin
        = ast::FunctionNode{current(parser).line, current(parser).column};
    builtin.module_path = parser.file;
    builtin.is_builtin = true;

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Builtin>()));

    // Parse indentifier
    bind(identifier, parse_function_identifier(parser));
    builtin.identifier = (ast::IdentifierNode *)identifier;

    // Parse possible type parameter
    if (std::holds_alternative<token::LeftBracket>(current(parser).kind)) {
        // Parse type parameters
        bind(type_parameters, parse_type_parameters(parser));
        builtin.type_parameters = type_parameters;
    }

    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        bind(arg, parse_function_argument(parser));

        // Parse type annotation
        match(parse_token(parser, getIndex<token::Kind, token::Colon>()));
        bind(type, parse_type(parser));
        ast::set_type(arg, type);

        // Add argument
        builtin.args.push_back((ast::FunctionArgumentNode *)arg);

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    // Parse right paren
    match(parse_token(parser, getIndex<token::Kind, token::RightParen>()));

    // Parse type annotation
    match(parse_token(parser, getIndex<token::Kind, token::Colon>()));
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        builtin.return_type_is_mutable = true;
        advance(parser);
    }
    bind(type, parse_type(parser));
    builtin.return_type = type;

    if (builtin.type_parameters.size() > 0) {
        builtin.state = ast::FunctionGenericCompletelyTyped;
    }

    // Check if builtin is - with just one argument
    if (builtin.identifier->value == "-" && builtin.args.size() == 1) {
        builtin.identifier->value = "-:negation";
    } else if (builtin.identifier->value == "[]"
               && builtin.args[0]->is_mutable) {
        builtin.identifier->value = "[]:mut";
    }

    parser.ast.push_back(builtin);
    return parser.ast.last_element();
}

// extern → "extern" IDENTIFIER "(" (IDENTIFIER ":" type "..."? ("," IDENTIFIER
// ":" type "..."?)*)? ")" ":" type
std::optional<ast::Node *> parse::parse_extern(Parser &parser) {
    // Create node
    auto function
        = ast::FunctionNode{current(parser).line, current(parser).column};
    function.module_path = parser.file;
    function.is_extern = true;

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Extern>()));

    // Parse indentifier
    bind(identifier, parse_identifier(parser));
    function.identifier = (ast::IdentifierNode *)identifier;

    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        // Parse variadic
        if (std::holds_alternative<token::Dot>(current(parser).kind)) {
            match(parse_token(parser, getIndex<token::Kind, token::Dot>()));
            match(parse_token(parser, getIndex<token::Kind, token::Dot>()));
            match(parse_token(parser, getIndex<token::Kind, token::Dot>()));
            function.is_extern_and_variadic = true;
            break;
        }

        // Create node
        auto function_argument = ast::FunctionArgumentNode{
            current(parser).line,
            current(parser).column
        };

        // Parse identifier
        bind(arg, parse_identifier(parser));
        function_argument.identifier = (ast::IdentifierNode *)arg;

        // Parse type annotation
        match(parse_token(parser, getIndex<token::Kind, token::Colon>()));

        bind(type, parse_type(parser));
        function_argument.type = type;

        // Store function argument on ast
        parser.ast.push_back(function_argument);
        function.args.push_back(
            (ast::FunctionArgumentNode *)parser.ast.last_element()
        );

        // Parse comma
        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    // Parse right paren
    match(parse_token(parser, getIndex<token::Kind, token::RightParen>()));

    // Parse type annotation
    match(parse_token(parser, getIndex<token::Kind, token::Colon>()));
    bind(type, parse_type(parser));
    function.return_type = type;

    parser.ast.push_back(function);
    return parser.ast.last_element();
}

// link_with → "link_with" STRING
std::optional<ast::Node *> parse::parse_link_with(Parser &parser) {
    // Create node
    auto link_with
        = ast::LinkWithNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::LinkWith>()));

    // Parse string
    bind(string, parse_string(parser));
    link_with.directives = (ast::StringNode *)string;

    parser.ast.push_back(link_with);
    return parser.ast.last_element();
}

// type_definition → "type" IDENTIFIER ("\n"+ IDENTIFIER ": " type)*
std::optional<ast::Node *> parse::parse_type_definition(Parser &parser) {
    // Create node
    auto type = ast::TypeNode{current(parser).line, current(parser).column};
    type.module_path = parser.file;

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Type>()));

    // Parse indentifier
    bind(identifier, parse_identifier(parser));
    type.identifier = (ast::IdentifierNode *)identifier;

    // Parse body
    match(parse_type_definition_body(parser, &type));

    // return
    parser.ast.push_back(type);
    return parser.ast.last_element();
}

// type_definition_body →  (("\n"+ IDENTIFIER ": " type)|(CASE IDENTIFIER
// type_defintion_body))*
std::optional<Ok> parse::parse_type_definition_body(
    Parser &parser, ast::TypeNode *node
) {  // Set new indentation level
    size_t backup = parser.current;
    advance_until_next_statement(parser);
    size_t previous = current_indentation(parser);
    parser.indentation_level.push_back(current(parser).column);
    if (previous >= current_indentation(parser)) {
        parser.indentation_level.pop_back();
        parser.current = backup;
        return Ok{};
    }

    while (!at_end(parser)) {
        // Advance until next field
        size_t backup = parser.current;
        advance_until_next_statement(parser);
        if (at_end(parser)) break;

        // Check indentation
        if (current(parser).column < current_indentation(parser)) {
            parser.current = backup;
            break;
        } else if (current(parser).column > current_indentation(parser)) {
            parser.errors.push_back(errors::unexpected_indent(location(parser))
            );
            return std::nullopt;
        }

        if (std::holds_alternative<token::Identifier>(current(parser).kind)) {
            // Parse field
            bind(field, parse_identifier(parser));

            // Parse colon
            match(parse_token(parser, getIndex<token::Kind, token::Colon>()));

            // Parse type
            bind(type_annotation, parse_type(parser));
            ast::set_type(field, type_annotation);
            node->fields.push_back((ast::IdentifierNode *)field);
        } else if (std::holds_alternative<token::Case>(current(parser).kind)) {
            // Parse keyword
            match(parse_token(parser, getIndex<token::Kind, token::Case>()));

            // Parse identifier
            bind(identifier, parse_identifier(parser));

            // Parse body
            ast::TypeNode new_case
                = ast::TypeNode{location(parser).line, location(parser).column};
            parser.ast.push_back(new_case);
            node->cases.push_back((ast::TypeNode *)parser.ast.last_element());
            node->cases[node->cases.size() - 1]->identifier
                = (ast::IdentifierNode *)identifier;
            match(parse_type_definition_body(
                parser,
                node->cases[node->cases.size() - 1]
            ));
        } else {
            todo();
        }
    }

    // Pop indentation level
    parser.indentation_level.pop_back();

    return Ok{};
}

// type → IDENTIFIER ("[" type (", " type)* "]")*
std::optional<ast::Type> parse::parse_type(Parser &parser) {
    bind(
        type_identifier,
        parse_token(parser, getIndex<token::Kind, token::Identifier>())
    );
    std::string literal = token::getLiteral(type_identifier);
    ast::Type type = ast::Type(literal);

    // If is type variable
    if (!type.is_builtin_type() && islower(literal[0])) {
        return ast::Type(ast::FinalTypeVariable(literal));
    }

    // Else parse possible type parameters
    if (std::holds_alternative<token::LeftBracket>(current(parser).kind)) {
        advance(parser);

        while (!std::holds_alternative<token::RightBracket>(current(parser).kind
               )
               && !at_end(parser)) {
            bind(parameter, parse_type(parser));
            type.as_nominal_type().parameters.push_back(parameter);

            if (std::holds_alternative<token::Comma>(current(parser).kind))
                advance(parser);
            else if (std::holds_alternative<token::NewLine>(current(parser).kind
                     ))
                advance_until_next_statement(parser);
            else
                break;
        }

        // Parse right paren
        match(parse_token(parser, getIndex<token::Kind, token::RightBracket>())
        );
    }

    return type;
}

std::optional<ast::InterfaceType> parse::parse_interface_type(Parser &parser) {
    auto type_identifier
        = parse_token(parser, getIndex<token::Kind, token::Identifier>());

    if (type_identifier.has_value())
        return ast::InterfaceType(token::getLiteral(type_identifier.value()));

    type_identifier = parse_token(parser, getIndex<token::Kind, token::Type>());
    if (type_identifier.has_value())
        return ast::InterfaceType(token::getLiteral(type_identifier.value()));

    return std::nullopt;
}

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
//           | dereference_assignment
//           | declaration
//           | assignment
//           | call
//           | field_assignment
//           | index_assignment
std::optional<ast::Node *> parse::parse_statement(Parser &parser) {
    if (std::holds_alternative<token::Function>(current(parser).kind)) {
        return parse_function(parser);
    } else if (std::holds_alternative<token::Interface>(current(parser).kind)) {
        return parse_interface(parser);
    } else if (std::holds_alternative<token::Builtin>(current(parser).kind)) {
        return parse_builtin(parser);
    } else if (std::holds_alternative<token::Extern>(current(parser).kind)) {
        return parse_extern(parser);
    } else if (std::holds_alternative<token::LinkWith>(current(parser).kind)) {
        return parse_link_with(parser);
    } else if (std::holds_alternative<token::Type>(current(parser).kind)) {
        return parse_type_definition(parser);
    } else if (std::holds_alternative<token::Return>(current(parser).kind)) {
        return parse_return_stmt(parser);
    } else if (std::holds_alternative<token::If>(current(parser).kind)) {
        return parse_if_else(parser);
    } else if (std::holds_alternative<token::While>(current(parser).kind)) {
        return parse_while_stmt(parser);
    } else if (std::holds_alternative<token::Break>(current(parser).kind)) {
        return parse_break_stmt(parser);
    } else if (std::holds_alternative<token::Continue>(current(parser).kind)) {
        return parse_continue_stmt(parser);
    } else if (std::holds_alternative<token::Use>(current(parser).kind)) {
        return parse_use_stmt(parser);
    } else if (std::holds_alternative<token::Include>(current(parser).kind)) {
        return parse_use_stmt(parser);
    } else if (std::holds_alternative<token::Star>(current(parser).kind)) {
        return parse_dereference_assignment(parser);
    } else if (std::holds_alternative<token::Identifier>(current(parser).kind)
               || std::holds_alternative<token::LeftParen>(current(parser).kind
               )) {
        if (sequenceMatch(
                parser,
                {getIndex<token::Kind, token::Identifier>(),
                 getIndex<token::Kind, token::Equal>()}
            )) {
            return parse_declaration(parser);
        } else if (sequenceMatch(
                       parser,
                       {getIndex<token::Kind, token::Identifier>(),
                        getIndex<token::Kind, token::Be>()}
                   )) {
            return parse_declaration(parser);
        } else if (sequenceMatch(
                       parser,
                       {getIndex<token::Kind, token::Identifier>(),
                        getIndex<token::Kind, token::ColonEqual>()}
                   )) {
            return parse_assignment(parser);
        } else {
            bind(result2, parse_grouping_or_assignable(parser));
            if (std::holds_alternative<ast::CallNode>(*result2)) {
                if (std::get<ast::CallNode>(*result2).identifier->value
                    == "[]") {
                    return parse_index_assignment(parser, result2);
                } else {
                    return result2;
                }
            } else if (std::holds_alternative<ast::FieldAccessNode>(*result2)) {
                return parse_field_assignment(parser, result2);
            }
        }
    }

    parser.errors.push_back(errors::expecting_statement(location(parser)));
    return std::nullopt;
}

// block_or_statement → statement
//                    | ("\n")+ block
std::optional<ast::Node *> parse::parse_block_or_statement(Parser &parser) {
    if (!std::holds_alternative<token::NewLine>(current(parser).kind)) {
        ast::BlockNode block = {current(parser).line, current(parser).column};
        auto statement = parse_statement(parser);
        if (statement.has_value()) {
            block.statements.push_back(statement.value());
            parser.ast.push_back(block);
            return parser.ast.last_element();
        }
        return statement;
    }
    return parse_block(parser);
}

// block_or_statement_or_expression → ("\n")+ block
//                                  | ("\n")+ expression
//                                  | expression
//                                  | statement
std::optional<ast::Node *> parse::parse_block_statement_or_expression(
    Parser &parser
) {
    auto position = parser.current;
    auto errors = parser.errors;

    if (std::holds_alternative<token::NewLine>(current(parser).kind)) {
        auto block = parse_block(parser);
        if (block.has_value()) return block.value();
        auto block_position = parser.current;
        auto block_errors = parser.errors;

        parser.current = position;
        parser.errors = errors;
        auto expression = parse_expression(parser);
        if (expression.has_value())
            return expression.value();
        else {
            parser.current = block_position;
            parser.errors = block_errors;
            return std::nullopt;
        }
    } else {
        auto expression = parse_expression(parser);
        if (expression.has_value()) return expression.value();

        position = parser.current;
        errors = parser.errors;
        ast::BlockNode block = {current(parser).line, current(parser).column};

        auto statement = parse_statement(parser);
        if (statement.has_value()) {
            block.statements.push_back(statement.value());
            parser.ast.push_back(block);
            return parser.ast.last_element();
        } else {
            return std::nullopt;
        }
    }
}

// declaration → IDENTIFIER ("be"|"=") expression (": " type)?
std::optional<ast::Node *> parse::parse_declaration(Parser &parser) {
    // Create node
    auto declaration
        = ast::DeclarationNode{current(parser).line, current(parser).column};

    // Parse identifier
    bind(identifier, parse_identifier(parser));
    declaration.identifier = (ast::IdentifierNode *)identifier;

    // Parse equal or be
    if (std::holds_alternative<token::Equal>(current(parser).kind)) {
        declaration.is_mutable = true;
    } else if (std::holds_alternative<token::Be>(current(parser).kind)) {
        declaration.is_mutable = false;
    } else {
        assert(false);
    }
    advance(parser);

    // Parse expression
    bind(expression, parse_expression(parser));
    declaration.expression = expression;

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        bind(type, parse_type(parser));
        ast::set_type(declaration.expression, type);
    }

    parser.ast.push_back(declaration);
    return parser.ast.last_element();
}

// assignment → IDENTIFIER ":=" expression (":" type)?
std::optional<ast::Node *> parse::parse_assignment(Parser &parser) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse identifier
    bind(identifier, parse_identifier(parser));
    assignment.assignable = identifier;

    // Parse equal
    match(parse_token(parser, getIndex<token::Kind, token::ColonEqual>()));

    // Parse expression
    bind(expression, parse_expression(parser));
    assignment.expression = expression;

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        bind(type, parse_type(parser));
        ast::set_type(assignment.expression, type);
    }

    parser.ast.push_back(assignment);
    return parser.ast.last_element();
}

// field_assignment → field_assignment "=" expression (":" type)?
std::optional<ast::Node *> parse::parse_field_assignment(
    Parser &parser, ast::Node *identifier
) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse field access
    assignment.assignable = identifier;

    // Parse equal
    match(parse_token(parser, getIndex<token::Kind, token::Equal>()));

    // Parse expression
    bind(expression, parse_expression(parser));
    assignment.expression = expression;

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        bind(type, parse_type(parser));
        ast::set_type(assignment.expression, type);

        if (std::holds_alternative<ast::IfElseNode>(*assignment.expression)) {
            auto &if_else = std::get<ast::IfElseNode>(*assignment.expression);
            ast::set_type(
                if_else.if_branch,
                ast::get_type(assignment.expression)
            );
            ast::set_type(
                if_else.else_branch.value(),
                ast::get_type(assignment.expression)
            );
        }
    }

    parser.ast.push_back(assignment);
    return parser.ast.last_element();
}

// dereference_assignment → dereference "=" expression (":" type)?
std::optional<ast::Node *> parse::parse_dereference_assignment(Parser &parser) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse dereference
    bind(identifier, parse_dereference(parser));
    assignment.assignable = identifier;

    // Parse equal
    match(parse_token(parser, getIndex<token::Kind, token::Equal>()));

    // Parse expression
    bind(expression, parse_expression(parser));
    assignment.expression = expression;

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        bind(type, parse_type(parser));
        ast::set_type(assignment.expression, type);

        if (std::holds_alternative<ast::IfElseNode>(*assignment.expression)) {
            auto &if_else = std::get<ast::IfElseNode>(*assignment.expression);
            ast::set_type(
                if_else.if_branch,
                ast::get_type(assignment.expression)
            );
            ast::set_type(
                if_else.else_branch.value(),
                ast::get_type(assignment.expression)
            );
        }
    }

    parser.ast.push_back(assignment);
    return parser.ast.last_element();
}

// index_assignment → index_access "=" expression (":" type)?
std::optional<ast::Node *> parse::parse_index_assignment(
    Parser &parser, ast::Node *index_access
) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse index access
    assignment.assignable = index_access;
    ((ast::CallNode *)index_access)->args[0]->is_mutable = true;
    ((ast::CallNode *)index_access)->identifier->value = "[]:mut";

    // Parse equal
    match(parse_token(parser, getIndex<token::Kind, token::Equal>()));

    // Parse expression
    bind(expression, parse_expression(parser));
    assignment.expression = expression;

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        bind(type, parse_type(parser));
        ast::set_type(assignment.expression, type);

        if (std::holds_alternative<ast::IfElseNode>(*assignment.expression)) {
            auto &if_else = std::get<ast::IfElseNode>(*assignment.expression);
            ast::set_type(
                if_else.if_branch,
                ast::get_type(assignment.expression)
            );
            ast::set_type(
                if_else.else_branch.value(),
                ast::get_type(assignment.expression)
            );
        }
    }

    parser.ast.push_back(assignment);
    return parser.ast.last_element();
}

// return → "return" expression?
std::optional<ast::Node *> parse::parse_return_stmt(Parser &parser) {
    // Create node
    auto return_stmt
        = ast::ReturnNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Return>()));

    // Parse expression
    if (!at_end(parser)
        && !std::holds_alternative<token::NewLine>(current(parser).kind)) {
        bind(expression, parse_expression(parser));
        return_stmt.expression = expression;
    }

    parser.ast.push_back(return_stmt);
    return parser.ast.last_element();
}

// break → "break"
std::optional<ast::Node *> parse::parse_break_stmt(Parser &parser) {
    // Create node
    auto break_node
        = ast::BreakNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Break>()));

    parser.ast.push_back(break_node);
    return parser.ast.last_element();
}

// continue → "continue"
std::optional<ast::Node *> parse::parse_continue_stmt(Parser &parser) {
    // Create node
    auto continue_node
        = ast::ContinueNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::Continue>()));

    parser.ast.push_back(continue_node);
    return parser.ast.last_element();
}

// if_else → "if" expression block_or_statement ("\n"* "else"
// block_or_statement)
std::optional<ast::Node *> parse::parse_if_else(Parser &parser) {
    size_t indentation_level = current(parser).column;

    // Create node
    auto if_else
        = ast::IfElseNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::If>()));

    // Parse condition
    bind(condition, parse_expression(parser));
    if_else.condition = condition;

    // Parse if branch
    bind(block, parse_block_or_statement(parser));
    if_else.if_branch = block;

    // Adance until new statement
    auto position_backup = parser.current;
    advance_until_next_statement(parser);

    // Match indentation
    if (current(parser).column == indentation_level
        && std::holds_alternative<token::Else>(current(parser).kind)) {
        advance(parser);

        // Parse else branch
        bind(block, parse_block_or_statement(parser));
        if_else.else_branch = block;
    } else {
        parser.current = position_backup;
    }

    parser.ast.push_back(if_else);
    return parser.ast.last_element();
}

// while → "while" expression block
std::optional<ast::Node *> parse::parse_while_stmt(Parser &parser) {
    // Create node
    auto while_stmt
        = ast::WhileNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::While>()));

    // Parse condition
    bind(condition, parse_expression(parser));
    while_stmt.condition = condition;

    // Parse block
    bind(block, parse_block(parser));
    while_stmt.block = block;

    // Return
    parser.ast.push_back(while_stmt);
    return parser.ast.last_element();
}

// use → ("use"|"include") string
std::optional<ast::Node *> parse::parse_use_stmt(Parser &parser) {
    // Create node
    auto use_stmt = ast::UseNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Use>());
    if (!keyword.has_value()) {
        match(parse_token(parser, getIndex<token::Kind, token::Include>()));
        use_stmt.include = true;
    }

    // Parse path
    bind(path, parse_string(parser));
    use_stmt.path = (ast::StringNode *)path;

    parser.ast.push_back(use_stmt);
    return parser.ast.last_element();
}

// call_argument → "mut"? IDENTIFIER ":" expression
//               → "mut"? expression
std::optional<ast::Node *> parse::parse_call_argument(Parser &parser) {
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        arg.is_mutable = true;
        advance(parser);
    }

    // Parse expression
    bind(expression, parse_expression(parser));
    arg.expression = expression;

    // Parse expression if previous was a identifier
    if (std::holds_alternative<ast::IdentifierNode>(*arg.expression)
        && std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);
        arg.identifier = (ast::IdentifierNode *)arg.expression;

        bind(expression, parse_expression(parser));
        arg.expression = expression;
    }

    parser.ast.push_back(arg);
    return parser.ast.last_element();
}

// call → IDENTIFIER "(" (call_argument ((","|("\n"+)) call_argument)*)* ")"
std::optional<ast::Node *> parse::parse_call(
    Parser &parser, ast::Node *identifier
) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    call.identifier = (ast::IdentifierNode *)identifier;

    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        bind(arg, parse_call_argument(parser));
        call.args.push_back((ast::CallArgumentNode *)arg);

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else if (std::holds_alternative<token::NewLine>(current(parser).kind
                   )) {
            advance_until_next_statement(parser);
        } else {
            break;
        }
    }

    // Parse right paren
    bind(
        right_paren,
        parse_token(parser, getIndex<token::Kind, token::RightParen>())
    );
    call.end_line = right_paren.line;

    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// struct → IDENTIFIER "{" (IDENTIFIER ": " expression (","|("\n"+)))*  "}"
std::optional<ast::Node *> parse::parse_struct(Parser &parser) {
    // Create node
    auto struct_literal
        = ast::StructLiteralNode{current(parser).line, current(parser).column};

    // Parse indentifier
    bind(identifier, parse_identifier(parser));
    struct_literal.identifier = (ast::IdentifierNode *)identifier;

    // Parse left curly
    match(parse_token(parser, getIndex<token::Kind, token::LeftCurly>()));

    // Parse fields
    while (!std::holds_alternative<token::RightCurly>(current(parser).kind)
           && !at_end(parser)) {
        advance_until_next_statement(parser);

        bind(identifier, parse_identifier(parser));
        match(parse_token(parser, getIndex<token::Kind, token::Colon>()));

        bind(expression, parse_expression(parser));
        struct_literal.fields[(ast::IdentifierNode *)identifier] = expression;

        if (std::holds_alternative<token::Comma>(current(parser).kind)) {
            advance(parser);
        } else if (std::holds_alternative<token::NewLine>(current(parser).kind
                   )) {
            advance_until_next_statement(parser);
        } else {
            break;
        }
    }

    // Parse right curly
    bind(
        right_curly,
        parse_token(parser, getIndex<token::Kind, token::RightCurly>())
    );
    struct_literal.end_line = right_curly.line;

    parser.ast.push_back(struct_literal);
    return parser.ast.last_element();
}

// expression → if_else_expression
//            | new
//            | not
//            | binary
std::optional<ast::Node *> parse::parse_expression(Parser &parser) {
    advance_until_next_statement(parser);

    if (std::holds_alternative<token::If>(current(parser).kind)) {
        return parse_if_else_expr(parser);
    } else if (std::holds_alternative<token::New>(current(parser).kind)) {
        return parse_new_expr(parser);
    } else if (std::holds_alternative<token::Not>(current(parser).kind)) {
        return parse_not_expr(parser);
    } else {
        return parse_binary(parser);
    }
}

// if_else_expression → "if" expression "\n"* expression ("else" "\n"*
// expression)?
std::optional<ast::Node *> parse::parse_if_else_expr(Parser &parser) {
    size_t indentation_level = current(parser).column;

    // Create node
    auto if_else
        = ast::IfElseNode{current(parser).line, current(parser).column};

    // Parse keyword
    match(parse_token(parser, getIndex<token::Kind, token::If>()));

    // Parse condition
    bind(condition, parse_expression(parser));
    if_else.condition = condition;

    // Parse if branch
    advance_until_next_statement(parser);
    bind(expression, parse_expression(parser));
    if_else.if_branch = expression;

    // Adance until new statement
    auto position_backup = parser.current;
    advance_until_next_statement(parser);

    // Match else
    if (std::holds_alternative<token::Else>(current(parser).kind)) {
        advance(parser);

        // Parse else branch
        advance_until_next_statement(parser);
        bind(expression, parse_expression(parser));
        if_else.else_branch = expression;

        parser.ast.push_back(if_else);
        return parser.ast.last_element();
    } else {
        parser.current = position_backup;
        parser.errors.push_back(
            errors::generic_error(location(parser), "Expecting else branch")
        );
        return std::nullopt;
    }
}

// not → "not" expression
std::optional<ast::Node *> parse::parse_not_expr(Parser &parser) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    bind(
        identifier,
        parse_identifier(parser, getIndex<token::Kind, token::Not>())
    );
    call.identifier = (ast::IdentifierNode *)identifier;

    // Parse expression
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    bind(expression, parse_expression(parser));
    arg.expression = expression;
    parser.ast.push_back(arg);
    call.args.push_back((ast::CallArgumentNode *)parser.ast.last_element());

    // Push node to ast
    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// new → "new" expression
std::optional<ast::Node *> parse::parse_new_expr(Parser &parser) {
    // Create node
    auto new_node = ast::NewNode{current(parser).line, current(parser).column};

    // Parse token
    match(parse_token(parser, getIndex<token::Kind, token::New>()));

    // Parse expression
    bind(expression, parse_expression(parser));
    new_node.expression = expression;

    // Push node to ast
    parser.ast.push_back(new_node);
    return parser.ast.last_element();
}

// binary → equality
// equality → comparison (("=="|"!=") comparison)*
// comparison → term ((">"|">="|"<"|"<=") term)*
// term → factor (("+"|"-") factor)*
// factor → primary (("*"|"/"|"%") primary)*
std::optional<ast::Node *> parse::parse_binary(Parser &parser, int precedence) {
    static std::map<size_t, int> operators;
    operators[getIndex<token::Kind, token::Or>()] = 1;
    operators[getIndex<token::Kind, token::And>()] = 2;
    operators[getIndex<token::Kind, token::EqualEqual>()] = 3;
    operators[getIndex<token::Kind, token::Less>()] = 4;
    operators[getIndex<token::Kind, token::LessEqual>()] = 4;
    operators[getIndex<token::Kind, token::Greater>()] = 4;
    operators[getIndex<token::Kind, token::GreaterEqual>()] = 4;
    operators[getIndex<token::Kind, token::Plus>()] = 5;
    operators[getIndex<token::Kind, token::Minus>()] = 5;
    operators[getIndex<token::Kind, token::Star>()] = 6;
    operators[getIndex<token::Kind, token::Slash>()] = 6;
    operators[getIndex<token::Kind, token::Modulo>()] = 6;

    if (precedence > std::max_element(
                         operators.begin(),
                         operators.end(),
                         [](auto a, auto b) { return a.second < b.second; }
        )->second) {
        return parse_primary(parser);
    } else {
        // Parse left
        auto left_node = ast::CallArgumentNode{
            current(parser).line,
            current(parser).column
        };
        bind(left_expression, parse_binary(parser, precedence + 1));
        left_node.expression = left_expression;

        while (true) {
            // Create call node
            auto call
                = ast::CallNode{current(parser).line, current(parser).column};

            // Parse operator
            auto op = current(parser).kind;
            if (operators.find(op.index()) == operators.end()
                || operators[op.index()] != precedence)
                break;

            bind(identifier, parse_identifier(parser, op.index()));

            // Parse right
            auto right_node = ast::CallArgumentNode{
                current(parser).line,
                current(parser).column
            };
            bind(right_expression, parse_binary(parser, precedence + 1));
            right_node.expression = right_expression;

            // Add identifier to call
            call.identifier = (ast::IdentifierNode *)identifier;

            // Add left node to call
            parser.ast.push_back(left_node);
            call.args.push_back(
                (ast::CallArgumentNode *)parser.ast.last_element()
            );

            // Add right node to call
            parser.ast.push_back(right_node);
            call.args.push_back(
                (ast::CallArgumentNode *)parser.ast.last_element()
            );
            call.end_line = right_node.line;

            // Iterate
            parser.ast.push_back(call);
            left_node = ast::CallArgumentNode{call.column, call.line};
            left_node.expression = parser.ast.last_element();
        }

        return left_node.expression;
    }
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
//         | interpolated_string
//         | call
//         | struct
//         | grouping_or_assignable
std::optional<ast::Node *> parse::parse_primary(Parser &parser) {
    if (std::holds_alternative<token::Minus>(current(parser).kind)) {
        return parse_negation(parser);
    } else if (std::holds_alternative<token::Star>(current(parser).kind)) {
        return parse_dereference(parser);
    } else if (std::holds_alternative<token::Ampersand>(current(parser).kind)) {
        return parse_address_of(parser);
    } else if (std::holds_alternative<token::LeftParen>(current(parser).kind)) {
        return parse_grouping_or_assignable(parser);
    } else if (std::holds_alternative<token::LeftBracket>(current(parser).kind
               )) {
        return parse_array(parser);
    } else if (std::holds_alternative<token::Float>(current(parser).kind)) {
        return parse_float(parser);
    } else if (std::holds_alternative<token::Integer>(current(parser).kind)) {
        return parse_integer(parser);
    } else if (std::holds_alternative<token::True>(current(parser).kind)) {
        return parse_boolean(parser);
    } else if (std::holds_alternative<token::False>(current(parser).kind)) {
        return parse_boolean(parser);
    } else if (std::holds_alternative<token::String>(current(parser).kind)) {
        return parse_string(parser);
    } else if (std::holds_alternative<token::StringLeft>(current(parser).kind
               )) {
        return parse_interpolated_string(parser);
    } else if (std::holds_alternative<token::Identifier>(current(parser).kind
               )) {
        if (sequenceMatch(
                parser,
                {getIndex<token::Kind, token::Identifier>(),
                 getIndex<token::Kind, token::LeftCurly>()}
            )) {
            return parse_struct(parser);
        } else {
            return parse_grouping_or_assignable(parser);
        }
    }

    parser.errors.push_back(errors::unexpected_character(location(parser)));
    return std::nullopt;
}

// grouping_or_assignable → grouping
//                        | identifier
//                        | index_access
//                        | field_access
//                        | call
std::optional<ast::Node *> parse::parse_grouping_or_assignable(Parser &parser) {
    // Parse indentifier
    std::optional<ast::Node *> assignable = std::nullopt;
    if (std::holds_alternative<token::Identifier>(current(parser).kind)) {
        assignable = parse_identifier(parser);
    }
    // Parse grouping
    else if (std::holds_alternative<token::LeftParen>(current(parser).kind)) {
        assignable = parse_grouping(parser);
    } else {
        unreachable();
    }
    if (!assignable.has_value()) return std::nullopt;

    // Iterate over assignables
    while (true) {
        if (std::holds_alternative<token::Dot>(current(parser).kind)) {
            assignable = parse_field_access(parser, assignable.value());
        } else if (std::holds_alternative<token::LeftBracket>(
                       current(parser).kind
                   )) {
            assignable = parse_index_access(parser, assignable.value());
        } else if (std::holds_alternative<token::LeftParen>(current(parser).kind
                   )) {
            assignable = parse_call(parser, assignable.value());
        } else {
            return assignable;
        }

        if (!assignable.has_value()) return std::nullopt;
    }
}

// negation → "-" primary
std::optional<ast::Node *> parse::parse_negation(Parser &parser) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    bind(
        identifier,
        parse_identifier(parser, getIndex<token::Kind, token::Minus>())
    );
    call.identifier = (ast::IdentifierNode *)identifier;
    call.identifier->value = "-:negation";

    // Parse expression
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    bind(expression, parse_primary(parser));
    arg.expression = expression;
    parser.ast.push_back(arg);
    call.args.push_back((ast::CallArgumentNode *)parser.ast.last_element());

    // Add call to ast
    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// address_of → "&" (field_access|identifier|index_access)
std::optional<ast::Node *> parse::parse_address_of(Parser &parser) {
    // Create node
    auto address_of
        = ast::AddressOfNode{current(parser).line, current(parser).column};
    advance(parser);

    // Parse expression
    bind(expression, parse_grouping_or_assignable(parser));
    address_of.expression = expression;

    // Add node to ast
    parser.ast.push_back(address_of);
    return parser.ast.last_element();
}

// dereference → "*" (dereference|assignable)
std::optional<ast::Node *> parse::parse_dereference(Parser &parser) {
    // Create node
    auto dereference
        = ast::DereferenceNode{current(parser).line, current(parser).column};

    // Parse dereference operator
    assert(std::holds_alternative<token::Star>(current(parser).kind));
    advance(parser);

    // Parse expression
    std::optional<ast::Node *> expression;
    if (std::holds_alternative<token::Star>(current(parser).kind)) {
        expression = parse_dereference(parser);
    } else {
        expression = parse_grouping_or_assignable(parser);
    }
    if (!expression.has_value()) return std::nullopt;
    dereference.expression = expression.value();

    // Add node to ast
    parser.ast.push_back(dereference);
    return parser.ast.last_element();
}

// grouping → "(" expression ")"
std::optional<ast::Node *> parse::parse_grouping(Parser &parser) {
    // Parse left paren
    match(parse_token(parser, getIndex<token::Kind, token::LeftParen>()));

    // Parse expression
    bind(expression, parse_expression(parser));

    // Parse right paren
    match(parse_token(parser, getIndex<token::Kind, token::RightParen>()));

    return expression;
}

// float → FLOAT
std::optional<ast::Node *> parse::parse_float(Parser &parser) {
    // Create node
    auto float_node
        = ast::FloatNode{current(parser).line, current(parser).column};

    // Parse float
    if (!std::holds_alternative<token::Float>(current(parser).kind))
        assert(false);
    float_node.value = atof(token::getLiteral(current(parser)).c_str());

    advance(parser);
    parser.ast.push_back(float_node);
    return parser.ast.last_element();
}

// integer → INTEGER
std::optional<ast::Node *> parse::parse_integer(Parser &parser) {
    // Create node
    auto integer
        = ast::IntegerNode{current(parser).line, current(parser).column};

    // Parse integer
    if (!std::holds_alternative<token::Integer>(current(parser).kind))
        assert(false);
    char *ptr;
    integer.value
        = strtol(token::getLiteral(current(parser)).c_str(), &ptr, 10);

    advance(parser);
    parser.ast.push_back(integer);
    return parser.ast.last_element();
}

// boolean → "true"|"false"
std::optional<ast::Node *> parse::parse_boolean(Parser &parser) {
    // Create node
    auto boolean
        = ast::BooleanNode{current(parser).line, current(parser).column};

    // Parse boolean
    if (!std::holds_alternative<token::True>(current(parser).kind)
        && !std::holds_alternative<token::False>(current(parser).kind)) {
        assert(false);
    }
    boolean.value = std::holds_alternative<token::True>(current(parser).kind);

    advance(parser);
    parser.ast.push_back(boolean);
    return parser.ast.last_element();
}

// identifier → IDENTIFIER
//            | AND
//            | OR
//            | NOT
std::optional<ast::Node *> parse::parse_identifier(Parser &parser) {
    if (std::holds_alternative<token::And>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::And>());
    } else if (std::holds_alternative<token::Or>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Or>());
    } else if (std::holds_alternative<token::Not>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Not>());
    } else {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::Identifier>()
        );
    }
}

// identifier → IDENTIFIER
//            | AND
//            | OR
//            | NOT
//            | PLUS
//            | MINUS
//            | STAR
//            | SLASH
//            | MODULO
//            | EQUAL
//            | EQUAL_EQUAL
//            | NOT_EQUAL
//            | LESS
//            | LESS_EQUAL
//            | GREATER
//            | GREATER_EQUAL
std::optional<ast::Node *> parse::parse_function_identifier(Parser &parser) {
    if (std::holds_alternative<token::And>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::And>());
    } else if (std::holds_alternative<token::Or>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Or>());
    } else if (std::holds_alternative<token::Not>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Not>());
    } else if (std::holds_alternative<token::Plus>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Plus>());
    } else if (std::holds_alternative<token::Minus>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Minus>());
    } else if (std::holds_alternative<token::Star>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Star>());
    } else if (std::holds_alternative<token::Slash>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Slash>());
    } else if (std::holds_alternative<token::Modulo>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Modulo>());
    } else if (std::holds_alternative<token::EqualEqual>(current(parser).kind
               )) {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::EqualEqual>()
        );
    } else if (std::holds_alternative<token::NotEqual>(current(parser).kind)) {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::NotEqual>()
        );
    } else if (std::holds_alternative<token::Less>(current(parser).kind)) {
        return parse_identifier(parser, getIndex<token::Kind, token::Less>());
    } else if (std::holds_alternative<token::LessEqual>(current(parser).kind)) {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::LessEqual>()
        );
    } else if (std::holds_alternative<token::Greater>(current(parser).kind)) {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::Greater>()
        );
    } else if (std::holds_alternative<token::GreaterEqual>(current(parser).kind
               )) {
        return parse_identifier(
            parser,
            getIndex<token::Kind, token::GreaterEqual>()
        );
    } else if (sequenceMatch(
                   parser,
                   {getIndex<token::Kind, token::LeftBracket>(),
                    getIndex<token::Kind, token::RightBracket>()}
               )) {
        auto identifier
            = ast::IdentifierNode{current(parser).line, current(parser).column};
        identifier.value = "[]";
        advance(parser);
        advance(parser);
        parser.ast.push_back(identifier);
        return parser.ast.last_element();
    }
    return parse_identifier(parser, getIndex<token::Kind, token::Identifier>());
}

std::optional<ast::Node *> parse::parse_identifier(
    Parser &parser, size_t token
) {
    // Create node
    auto identifier
        = ast::IdentifierNode{current(parser).line, current(parser).column};

    // Parse identifier
    bind(result, parse_token(parser, token));
    identifier.value = token::getLiteral(result);

    parser.ast.push_back(identifier);
    return parser.ast.last_element();
}

// string → STRING
std::optional<ast::Node *> parse::parse_string(Parser &parser) {
    // Create node
    auto string = ast::StringNode{current(parser).line, current(parser).column};

    // Parse string
    bind(result, parse_token(parser, getIndex<token::Kind, token::String>()));
    string.value = token::getLiteral(result);

    parser.ast.push_back(string);
    return parser.ast.last_element();
}

// interpolated_string → STRING_LEFT expression (STRING_MIDDLE expression)*
// STRING_RIGHT
std::optional<ast::Node *> parse::parse_interpolated_string(Parser &parser
) {  // Create node
    auto string = ast::InterpolatedStringNode{
        current(parser).line,
        current(parser).column
    };

    // Parse string
    bind(
        result,
        parse_token(parser, getIndex<token::Kind, token::StringLeft>())
    );
    string.strings.push_back(token::getLiteral(result));

    while (true) {
        // Parse expression
        bind(expression, parse_expression(parser));
        string.expressions.push_back(expression);

        if (std::holds_alternative<token::StringMiddle>(current(parser).kind)) {
            bind(
                result,
                parse_token(
                    parser,
                    getIndex<token::Kind, token::StringMiddle>()
                )
            );
            string.strings.push_back(token::getLiteral(result));
        } else {
            break;
        }
    }

    // Parse string
    bind(
        result2,
        parse_token(parser, getIndex<token::Kind, token::StringRight>())
    );
    string.strings.push_back(token::getLiteral(result2));

    parser.ast.push_back(string);
    return parser.ast.last_element();
}

// array → "[" (expression ("," expression)*)* "]"
std::optional<ast::Node *> parse::parse_array(Parser &parser) {
    // Create node
    auto array = ast::ArrayNode{current(parser).line, current(parser).column};

    // Parse left bracket
    assert(std::holds_alternative<token::LeftBracket>(current(parser).kind));
    advance(parser);

    // Parse elements
    while (!std::holds_alternative<token::RightBracket>(current(parser).kind)
           && !at_end(parser)) {
        bind(expression, parse_expression(parser));
        array.elements.push_back(expression);

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else if (std::holds_alternative<token::NewLine>(current(parser).kind))
            advance_until_next_statement(parser);
        else
            break;
    }

    // Parse right bracket
    match(parse_token(parser, getIndex<token::Kind, token::RightBracket>()));

    parser.ast.push_back(array);
    return parser.ast.last_element();
}

// field_access → assignable "." IDENTFIER ("." IDENTIFIER)*
std::optional<ast::Node *> parse::parse_field_access(
    Parser &parser, ast::Node *accessed
) {  // Create node
    auto field_access
        = ast::FieldAccessNode{current(parser).line, current(parser).column};

    // Parse identifier
    field_access.accessed = accessed;

    match(parse_token(parser, getIndex<token::Kind, token::Dot>()));

    while (std::holds_alternative<token::Identifier>(current(parser).kind)) {
        // Parse identifier
        bind(identifier, parse_identifier(parser));
        field_access.fields_accessed.push_back((ast::IdentifierNode *)identifier
        );

        if (std::holds_alternative<token::Dot>(current(parser).kind)) {
            advance(parser);
        } else {
            break;
        }
    }

    parser.ast.push_back(field_access);
    return parser.ast.last_element();
}

// index_access → assignable "[" expression "]"
std::optional<ast::Node *> parse::parse_index_access(
    Parser &parser, ast::Node *expression
) {
    // Create node
    auto index_access
        = ast::CallNode{current(parser).line, current(parser).column};

    // Create identifier node
    auto identifier
        = ast::IdentifierNode{current(parser).line, current(parser).column};
    identifier.value = "[]";
    parser.ast.push_back(identifier);
    index_access.identifier = (ast::IdentifierNode *)parser.ast.last_element();

    // Parse expression being indexed
    auto arg1
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    arg1.expression = expression;
    parser.ast.push_back(arg1);
    index_access.args.push_back(
        (ast::CallArgumentNode *)parser.ast.last_element()
    );

    // Parse left bracket
    assert(std::holds_alternative<token::LeftBracket>(current(parser).kind));
    advance(parser);

    // Parse index expression
    auto arg2
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    bind(index, parse_expression(parser));
    arg2.expression = index;
    parser.ast.push_back(arg2);
    index_access.args.push_back(
        (ast::CallArgumentNode *)parser.ast.last_element()
    );

    // Parse right bracket
    bind(
        right_bracket,
        parse_token(parser, getIndex<token::Kind, token::RightBracket>())
    );

    parser.ast.push_back(index_access);
    return parser.ast.last_element();
}

std::optional<token::Token> parse::parse_token(Parser &parser, size_t token) {
    if (current(parser).kind.index() != token) {
        parser.errors.push_back(errors::unexpected_character(location(parser)));
        return std::nullopt;
    }
    auto curr = current(parser);
    advance(parser);
    return curr;
}