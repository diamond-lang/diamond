#include "parser.hpp"

#include <filesystem>
#include <map>
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

    std::variant<ast::Node *, Error> parse_program(Parser &parser);
    std::variant<ast::Node *, Error> parse_block(Parser &parser);
    std::variant<std::vector<ast::TypeParameter>, Error> parse_type_parameters(
        Parser &parser
    );
    std::variant<ast::Node *, Error> parse_function_argument(Parser &parser);
    std::variant<ast::Node *, Error> parse_function(Parser &parser);
    std::variant<ast::Node *, Error> parse_interface(Parser &parser);
    std::variant<ast::Node *, Error> parse_builtin(Parser &parser);
    std::variant<ast::Node *, Error> parse_extern(Parser &parser);
    std::variant<ast::Node *, Error> parse_link_with(Parser &parser);
    std::variant<ast::Node *, Error> parse_type_definition(Parser &parser);
    std::variant<Ok, Error> parse_type_definition_body(
        Parser &parser, ast::TypeNode *node
    );
    std::variant<ast::Type, Error> parse_type(Parser &parser);
    std::variant<ast::InterfaceType, Error> parse_interface_type(Parser &parser
    );
    std::variant<ast::Node *, Error> parse_statement(Parser &parser);
    std::variant<ast::Node *, Error> parse_block_or_statement(Parser &parser);
    std::variant<ast::Node *, Error> parse_block_statement_or_expression(
        Parser &parser
    );
    std::variant<ast::Node *, Error> parse_declaration(Parser &parser);
    std::variant<ast::Node *, Error> parse_assignment(Parser &parser);
    std::variant<ast::Node *, Error> parse_field_assignment(
        Parser &parser, ast::Node *identifier
    );
    std::variant<ast::Node *, Error> parse_dereference_assignment(Parser &parser
    );
    std::variant<ast::Node *, Error> parse_index_assignment(
        Parser &parser, ast::Node *index_access
    );
    std::variant<ast::Node *, Error> parse_return_stmt(Parser &parser);
    std::variant<ast::Node *, Error> parse_break_stmt(Parser &parser);
    std::variant<ast::Node *, Error> parse_continue_stmt(Parser &parser);
    std::variant<ast::Node *, Error> parse_if_else(Parser &parser);
    std::variant<ast::Node *, Error> parse_while_stmt(Parser &parser);
    std::variant<ast::Node *, Error> parse_use_stmt(Parser &parser);
    std::variant<ast::Node *, Error> parse_call_argument(Parser &parser);
    std::variant<ast::Node *, Error> parse_call(
        Parser &parser, ast::Node *identifier
    );
    std::variant<ast::Node *, Error> parse_struct(Parser &parser);
    std::variant<ast::Node *, Error> parse_expression(Parser &parser);
    std::variant<ast::Node *, Error> parse_if_else_expr(Parser &parser);
    std::variant<ast::Node *, Error> parse_not_expr(Parser &parser);
    std::variant<ast::Node *, Error> parse_new_expr(Parser &parser);
    std::variant<ast::Node *, Error> parse_binary(
        Parser &parser, int precedence = 1
    );
    std::variant<ast::Node *, Error> parse_primary(Parser &parser);
    std::variant<ast::Node *, Error> parse_grouping_or_assignable(Parser &parser
    );
    std::variant<ast::Node *, Error> parse_negation(Parser &parser);
    std::variant<ast::Node *, Error> parse_address_of(Parser &parser);
    std::variant<ast::Node *, Error> parse_dereference(Parser &parser);
    std::variant<ast::Node *, Error> parse_grouping(Parser &parser);
    std::variant<ast::Node *, Error> parse_float(Parser &parser);
    std::variant<ast::Node *, Error> parse_integer(Parser &parser);
    std::variant<ast::Node *, Error> parse_boolean(Parser &parser);
    std::variant<ast::Node *, Error> parse_identifier(Parser &parser);
    std::variant<ast::Node *, Error> parse_function_identifier(Parser &parser);
    std::variant<ast::Node *, Error> parse_identifier(
        Parser &parser, size_t token
    );
    std::variant<ast::Node *, Error> parse_string(Parser &parser);
    std::variant<ast::Node *, Error> parse_interpolated_string(Parser &parser);
    std::variant<ast::Node *, Error> parse_array(Parser &parser);
    std::variant<ast::Node *, Error> parse_field_access(
        Parser &parser, ast::Node *accessed
    );
    std::variant<ast::Node *, Error> parse_index_access(
        Parser &parser, ast::Node *expression
    );
    std::variant<token::Token, Error> parse_token(Parser &parser, size_t token);

    token::Token current(Parser &parser);
    size_t current_indentation(Parser &parser);
    void advance(Parser &parser);
    void advance_until_next_statement(Parser &parser);
    bool at_end(Parser &parser);
    bool match(Parser &parser, std::vector<size_t> tokens);
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

bool parse::match(Parser &parser, std::vector<size_t> tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (parser.current + i >= parser.tokens.size()
            || parser.tokens[parser.current + i].kind.index() != tokens[i]) {
            return false;
        }
    }
    return true;
}

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
    if (std::holds_alternative<Error>(parsing_result)) return parser.errors;

    ast.module_path = module_path;
    ast.program = (ast::BlockNode *)std::get<ast::Node *>(parsing_result);
    ast.modules[module_path.string()]
        = (ast::BlockNode *)std::get<ast::Node *>(parsing_result);

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
    if (std::holds_alternative<Error>(parsing_result)) return parser.errors;

    ast.modules[file.string()]
        = (ast::BlockNode *)std::get<ast::Node *>(parsing_result);
    return Ok{};
}

// program → block
std::variant<ast::Node *, Error> parse::parse_program(Parser &parser) {
    return parse_block(parser);
}

// block → statement*
std::variant<ast::Node *, Error> parse::parse_block(Parser &parser) {
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
            );  // tested in errors/expecting_new_indentation_level.dm
            return Error{};
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
            );  // tested in test/errors/unexpected_indentation_1.dm and test/errors/unexpected_indentation_2.dm
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
        if (std::holds_alternative<ast::Node *>(result)) {
            if (std::holds_alternative<ast::FunctionNode>(
                    *std::get<ast::Node *>(result)
                )) {
                block.functions.push_back(
                    (ast::FunctionNode *)std::get<ast::Node *>(result)
                );
            } else if (std::holds_alternative<ast::InterfaceNode>(
                           *std::get<ast::Node *>(result)
                       )) {
                block.interfaces.push_back(
                    (ast::InterfaceNode *)std::get<ast::Node *>(result)
                );
            } else if (std::holds_alternative<ast::TypeNode>(
                           *std::get<ast::Node *>(result)
                       )) {
                block.types.push_back(
                    (ast::TypeNode *)std::get<ast::Node *>(result)
                );
            } else if (std::holds_alternative<ast::UseNode>(
                           *std::get<ast::Node *>(result)
                       )) {
                block.use_statements.push_back(
                    (ast::UseNode *)std::get<ast::Node *>(result)
                );
            } else if (std::holds_alternative<ast::LinkWithNode>(
                           *std::get<ast::Node *>(result)
                       )) {
                parser.ast.link_with.push_back(
                    std::get<ast::LinkWithNode>(*std::get<ast::Node *>(result))
                        .directives->value
                );
            } else {
                // If we are printing an interpolated string desugares it
                if (std::holds_alternative<ast::CallNode>(
                        *std::get<ast::Node *>(result)
                    )
                    && std::get<ast::CallNode>(*std::get<ast::Node *>(result))
                               .identifier->value
                           == "print"
                    && std::get<ast::CallNode>(*std::get<ast::Node *>(result))
                               .args.size()
                           == 1
                    && std::holds_alternative<ast::InterpolatedStringNode>(
                        *std::get<ast::CallNode>(*std::get<ast::Node *>(result))
                             .args[0]
                             ->expression
                    )) {
                    ast::CallNode &print_call
                        = std::get<ast::CallNode>(*std::get<ast::Node *>(result)
                        );
                    ast::InterpolatedStringNode &interpolated_string
                        = std::get<ast::InterpolatedStringNode>(
                            *std::get<ast::CallNode>(
                                 *std::get<ast::Node *>(result)
                            )
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
                    block.statements.push_back(std::get<ast::Node *>(result));
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
        return Error{};
    else {
        parser.ast.push_back(block);
        return parser.ast.last_element();
    }
}

// type_parameters → "[" identifier (":" interface_type)? ("," identifier (":"
// interface_type)?)? "]"
std::variant<std::vector<ast::TypeParameter>, Error> parse::
    parse_type_parameters(Parser &parser) {
    std::vector<ast::TypeParameter> type_parameters;

    // Parse left bracket
    auto left_bracket
        = parse_token(parser, getIndex<token::Kind, token::LeftBracket>());
    if (std::holds_alternative<Error>(left_bracket)) return Error{};

    // Parse type parameters
    while (!std::holds_alternative<token::RightBracket>(current(parser).kind)
           && !at_end(parser)) {
        auto parameter = parse_type(parser);
        if (std::holds_alternative<Error>(parameter))
            return std::get<Error>(parameter);

        assert(std::get<ast::Type>(parameter).is_final_type_variable());

        ast::TypeParameter type_parameter;
        type_parameter.type = std::get<ast::Type>(parameter);

        if (std::holds_alternative<token::Colon>(current(parser).kind)) {
            advance(parser);

            auto interface_type = parse_interface_type(parser);
            if (std::holds_alternative<Error>(interface_type)) return Error{};
            type_parameter.interface.insert(
                std::get<ast::InterfaceType>(interface_type)
            );
        }

        type_parameters.push_back(type_parameter);

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    // Parse right bracket
    auto right_bracket
        = parse_token(parser, getIndex<token::Kind, token::RightBracket>());
    if (std::holds_alternative<Error>(right_bracket)) return Error{};

    // Return
    return type_parameters;
}

std::variant<ast::Node *, Error> parse::parse_function_argument(Parser &parser
) {
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
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    function_argument.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    parser.ast.push_back(function_argument);
    return parser.ast.last_element();
}

// function → "function" IDENTIFIER type_parameters? "(" (function_argument (":"
// type)? ",")* ")" (":" type)? block_statement_or_expression
std::variant<ast::Node *, Error> parse::parse_function(Parser &parser) {
    // Create node
    auto function
        = ast::FunctionNode{current(parser).line, current(parser).column};
    function.module_path = parser.file;

    // Parse keyword
    auto keyword
        = parse_token(parser, getIndex<token::Kind, token::Function>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse indentifier
    auto identifier = parse_function_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    function.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse possible type parameters
    if (std::holds_alternative<token::LeftBracket>(current(parser).kind)) {
        auto type_parameters = parse_type_parameters(parser);
        if (std::holds_alternative<Error>(type_parameters)) return Error{};
        function.type_parameters
            = std::get<std::vector<ast::TypeParameter>>(type_parameters);
    }

    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        auto arg = parse_function_argument(parser);
        if (std::holds_alternative<Error>(arg)) return arg;

        // Parse type annotation
        if (std::holds_alternative<token::Colon>(current(parser).kind)) {
            advance(parser);

            auto type = parse_type(parser);
            if (std::holds_alternative<Error>(type)) return Error{};

            ast::set_type(
                std::get<ast::Node *>(arg),
                std::get<ast::Type>(type)
            );
        }
        function.args.push_back(
            (ast::FunctionArgumentNode *)std::get<ast::Node *>(arg)
        );

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        // Parse mut
        if (std::holds_alternative<token::Mut>(current(parser).kind)) {
            function.return_type_is_mutable = true;
            advance(parser);
        }

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        function.return_type = std::get<ast::Type>(type);
    }

    // Parse body
    auto body = parse_block_statement_or_expression(parser);
    if (std::holds_alternative<Error>(body)) return Error{};
    function.body = std::get<ast::Node *>(body);

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
std::variant<ast::Node *, Error> parse::parse_interface(Parser &parser) {
    // Create node
    auto interface = ast::InterfaceNode{
        current(parser).line,
        current(parser).column
    };
    interface.module_path = parser.file;

    // Parse keyword
    auto keyword
        = parse_token(parser, getIndex<token::Kind, token::Interface>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse indentifier
    auto identifier = parse_function_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return Error{};
    interface.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse type parameters
    auto type_parameters = parse_type_parameters(parser);
    if (std::holds_alternative<Error>(type_parameters)) return Error{};
    interface.type_parameters
        = std::get<std::vector<ast::TypeParameter>>(type_parameters);

    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        auto arg = parse_function_argument(parser);
        if (std::holds_alternative<Error>(arg)) return arg;

        // Parse type annotation
        auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
        if (std::holds_alternative<Error>(colon)) return Error{};

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(std::get<ast::Node *>(arg), std::get<ast::Type>(type));

        // Add argument
        interface.args.push_back(
            (ast::FunctionArgumentNode *)std::get<ast::Node *>(arg)
        );

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};

    // Parse type annotation
    auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
    if (std::holds_alternative<Error>(colon)) return Error{};

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        interface.return_type_is_mutable = true;
        advance(parser);
    }

    auto type = parse_type(parser);
    if (std::holds_alternative<Error>(type)) return Error{};

    interface.return_type = std::get<ast::Type>(type);

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
std::variant<ast::Node *, Error> parse::parse_builtin(Parser &parser) {
    // Create node
    auto builtin
        = ast::FunctionNode{current(parser).line, current(parser).column};
    builtin.module_path = parser.file;
    builtin.is_builtin = true;

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Builtin>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse indentifier
    auto identifier = parse_function_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    builtin.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse possible type parameter
    if (std::holds_alternative<token::LeftBracket>(current(parser).kind)) {
        // Parse type parameters
        auto type_parameters = parse_type_parameters(parser);
        if (std::holds_alternative<Error>(type_parameters)) return Error{};
        builtin.type_parameters
            = std::get<std::vector<ast::TypeParameter>>(type_parameters);
    }

    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        auto arg = parse_function_argument(parser);
        if (std::holds_alternative<Error>(arg)) return arg;

        // Parse type annotation
        auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
        if (std::holds_alternative<Error>(colon)) return Error{};

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(std::get<ast::Node *>(arg), std::get<ast::Type>(type));

        // Add argument
        builtin.args.push_back(
            (ast::FunctionArgumentNode *)std::get<ast::Node *>(arg)
        );

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};

    // Parse type annotation
    auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
    if (std::holds_alternative<Error>(colon)) return Error{};

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        builtin.return_type_is_mutable = true;
        advance(parser);
    }

    auto type = parse_type(parser);
    if (std::holds_alternative<Error>(type)) return Error{};

    builtin.return_type = std::get<ast::Type>(type);

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
std::variant<ast::Node *, Error> parse::parse_extern(Parser &parser) {
    // Create node
    auto function
        = ast::FunctionNode{current(parser).line, current(parser).column};
    function.module_path = parser.file;
    function.is_extern = true;

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Extern>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse indentifier
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    function.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        // Parse variadic
        if (std::holds_alternative<token::Dot>(current(parser).kind)) {
            auto result
                = parse_token(parser, getIndex<token::Kind, token::Dot>());
            if (std::holds_alternative<Error>(result))
                return std::get<Error>(result);
            result = parse_token(parser, getIndex<token::Kind, token::Dot>());
            if (std::holds_alternative<Error>(result))
                return std::get<Error>(result);
            result = parse_token(parser, getIndex<token::Kind, token::Dot>());
            if (std::holds_alternative<Error>(result))
                return std::get<Error>(result);

            function.is_extern_and_variadic = true;
            break;
        }

        // Create node
        auto function_argument = ast::FunctionArgumentNode{
            current(parser).line,
            current(parser).column
        };

        // Parse identifier
        auto arg = parse_identifier(parser);
        if (std::holds_alternative<Error>(arg)) return arg;
        function_argument.identifier
            = (ast::IdentifierNode *)std::get<ast::Node *>(arg);

        // Parse type annotation
        auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
        if (std::holds_alternative<Error>(colon)) return Error{};

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};
        function_argument.type = std::get<ast::Type>(type);

        // Store function argument on ast
        parser.ast.push_back(function_argument);
        function.args.push_back(
            (ast::FunctionArgumentNode *)parser.ast.last_element()
        );

        // Parse comma
        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};

    // Parse type annotation
    auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
    if (std::holds_alternative<Error>(colon)) return Error{};

    auto type = parse_type(parser);
    if (std::holds_alternative<Error>(type)) return Error{};

    function.return_type = std::get<ast::Type>(type);

    parser.ast.push_back(function);
    return parser.ast.last_element();
}

// link_with → "link_with" STRING
std::variant<ast::Node *, Error> parse::parse_link_with(Parser &parser) {
    // Create node
    auto link_with
        = ast::LinkWithNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword
        = parse_token(parser, getIndex<token::Kind, token::LinkWith>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse string
    auto string = parse_string(parser);
    if (std::holds_alternative<Error>(string)) return Error{};

    link_with.directives = (ast::StringNode *)std::get<ast::Node *>(string);

    parser.ast.push_back(link_with);
    return parser.ast.last_element();
}

// type_definition → "type" IDENTIFIER ("\n"+ IDENTIFIER ": " type)*
std::variant<ast::Node *, Error> parse::parse_type_definition(Parser &parser) {
    // Create node
    auto type = ast::TypeNode{current(parser).line, current(parser).column};
    type.module_path = parser.file;

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Type>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse indentifier
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    type.identifier = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse body
    auto body = parse_type_definition_body(parser, &type);
    if (std::holds_alternative<Error>(body)) return std::get<Error>(body);

    // return
    parser.ast.push_back(type);
    return parser.ast.last_element();
}

// type_definition_body →  (("\n"+ IDENTIFIER ": " type)|(CASE IDENTIFIER
// type_defintion_body))*
std::variant<Ok, Error> parse::parse_type_definition_body(
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
            return Error{};
        }

        if (std::holds_alternative<token::Identifier>(current(parser).kind)) {
            // Parse field
            auto field = parse_identifier(parser);
            if (std::holds_alternative<Error>(field)) return Error{};

            // Parse colon
            auto colon
                = parse_token(parser, getIndex<token::Kind, token::Colon>());
            if (std::holds_alternative<Error>(colon)) return Error{};

            auto type_annotation = parse_type(parser);
            if (std::holds_alternative<Error>(type_annotation)) return Error{};

            ast::set_type(
                std::get<ast::Node *>(field),
                std::get<ast::Type>(type_annotation)
            );
            node->fields.push_back(
                (ast::IdentifierNode *)std::get<ast::Node *>(field)
            );
        } else if (std::holds_alternative<token::Case>(current(parser).kind)) {
            // Parse keyword
            auto keyword
                = parse_token(parser, getIndex<token::Kind, token::Case>());
            if (std::holds_alternative<Error>(keyword)) return Error{};

            // Parse identifier
            auto identifier = parse_identifier(parser);
            if (std::holds_alternative<Error>(identifier)) return Error{};

            // Parse body
            ast::TypeNode new_case
                = ast::TypeNode{location(parser).line, location(parser).column};
            parser.ast.push_back(new_case);
            node->cases.push_back((ast::TypeNode *)parser.ast.last_element());
            node->cases[node->cases.size() - 1]->identifier
                = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);
            auto body = parse_type_definition_body(
                parser,
                node->cases[node->cases.size() - 1]
            );
            if (std::holds_alternative<Error>(body))
                return std::get<Error>(body);
        } else {
            todo();
        }
    }

    // Pop indentation level
    parser.indentation_level.pop_back();

    return Ok{};
}

// type → IDENTIFIER ("[" type (", " type)* "]")*
std::variant<ast::Type, Error> parse::parse_type(Parser &parser) {
    auto type_identifier
        = parse_token(parser, getIndex<token::Kind, token::Identifier>());
    if (std::holds_alternative<Error>(type_identifier)) return Error{};
    std::string literal
        = token::getLiteral(std::get<token::Token>(type_identifier));
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
            auto parameter = parse_type(parser);
            if (std::holds_alternative<Error>(parameter)) return parameter;

            type.as_nominal_type().parameters.push_back(
                std::get<ast::Type>(parameter)
            );

            if (std::holds_alternative<token::Comma>(current(parser).kind))
                advance(parser);
            else if (std::holds_alternative<token::NewLine>(current(parser).kind
                     ))
                advance_until_next_statement(parser);
            else
                break;
        }

        // Parse right paren
        auto right_paren
            = parse_token(parser, getIndex<token::Kind, token::RightBracket>());
        if (std::holds_alternative<Error>(right_paren)) return Error{};
    }

    return type;
}

std::variant<ast::InterfaceType, Error> parse::parse_interface_type(
    Parser &parser
) {
    auto type_identifier
        = parse_token(parser, getIndex<token::Kind, token::Identifier>());
    if (std::holds_alternative<token::Token>(type_identifier))
        return ast::InterfaceType(
            token::getLiteral(std::get<token::Token>(type_identifier))
        );

    type_identifier = parse_token(parser, getIndex<token::Kind, token::Type>());
    if (std::holds_alternative<token::Token>(type_identifier))
        return ast::InterfaceType(
            token::getLiteral(std::get<token::Token>(type_identifier))
        );

    return Error{};
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
std::variant<ast::Node *, Error> parse::parse_statement(Parser &parser) {
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
        if (match(
                parser,
                {getIndex<token::Kind, token::Identifier>(),
                 getIndex<token::Kind, token::Equal>()}
            )) {
            return parse_declaration(parser);
        } else if (match(
                       parser,
                       {getIndex<token::Kind, token::Identifier>(),
                        getIndex<token::Kind, token::Be>()}
                   )) {
            return parse_declaration(parser);
        } else if (match(
                       parser,
                       {getIndex<token::Kind, token::Identifier>(),
                        getIndex<token::Kind, token::ColonEqual>()}
                   )) {
            return parse_assignment(parser);
        } else {
            auto result = parse_grouping_or_assignable(parser);
            if (std::holds_alternative<Error>(result)) return result;

            if (std::holds_alternative<ast::CallNode>(
                    *std::get<ast::Node *>(result)
                )) {
                if (std::get<ast::CallNode>(*std::get<ast::Node *>(result))
                        .identifier->value
                    == "[]") {
                    return parse_index_assignment(
                        parser,
                        std::get<ast::Node *>(result)
                    );
                } else {
                    return result;
                }

            } else if (std::holds_alternative<ast::FieldAccessNode>(
                           *std::get<ast::Node *>(result)
                       )) {
                return parse_field_assignment(
                    parser,
                    std::get<ast::Node *>(result)
                );
            }
        }
    }

    parser.errors.push_back(errors::expecting_statement(location(parser)));
    return Error{};
}

// block_or_statement → statement
//                    | ("\n")+ block
std::variant<ast::Node *, Error> parse::parse_block_or_statement(Parser &parser
) {
    if (!std::holds_alternative<token::NewLine>(current(parser).kind)) {
        ast::BlockNode block = {current(parser).line, current(parser).column};
        auto statement = parse_statement(parser);
        if (std::get<ast::Node *>(statement)) {
            block.statements.push_back(std::get<ast::Node *>(statement));
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
std::variant<ast::Node *, Error> parse::parse_block_statement_or_expression(
    Parser &parser
) {
    auto position = parser.current;
    auto errors = parser.errors;

    if (std::holds_alternative<token::NewLine>(current(parser).kind)) {
        auto block = parse_block(parser);
        if (std::holds_alternative<ast::Node *>(block))
            return std::get<ast::Node *>(block);
        auto block_position = parser.current;
        auto block_errors = parser.errors;

        parser.current = position;
        parser.errors = errors;
        auto expression = parse_expression(parser);
        if (std::holds_alternative<ast::Node *>(expression))
            return std::get<ast::Node *>(expression);
        else {
            parser.current = block_position;
            parser.errors = block_errors;
            return Error{};
        }
    } else {
        auto expression = parse_expression(parser);
        if (std::holds_alternative<ast::Node *>(expression))
            return std::get<ast::Node *>(expression);

        position = parser.current;
        errors = parser.errors;
        ast::BlockNode block = {current(parser).line, current(parser).column};

        auto statement = parse_statement(parser);
        if (std::holds_alternative<ast::Node *>(statement)) {
            block.statements.push_back(std::get<ast::Node *>(statement));
            parser.ast.push_back(block);
            return parser.ast.last_element();
        } else {
            return Error{};
        }
    }
}

// declaration → IDENTIFIER ("be"|"=") expression (": " type)?
std::variant<ast::Node *, Error> parse::parse_declaration(Parser &parser) {
    // Create node
    auto declaration
        = ast::DeclarationNode{current(parser).line, current(parser).column};

    // Parse identifier
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return Error{};
    declaration.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

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
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;
    declaration.expression = std::get<ast::Node *>(expression);

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(declaration.expression, std::get<ast::Type>(type));
    }

    parser.ast.push_back(declaration);
    return parser.ast.last_element();
}

// assignment → IDENTIFIER ":=" expression (":" type)?
std::variant<ast::Node *, Error> parse::parse_assignment(Parser &parser) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse identifier
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return Error{};
    assignment.assignable = std::get<ast::Node *>(identifier);

    // Parse equal
    auto equal
        = parse_token(parser, getIndex<token::Kind, token::ColonEqual>());
    if (std::holds_alternative<Error>(equal)) return Error{};

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;
    assignment.expression = std::get<ast::Node *>(expression);

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(assignment.expression, std::get<ast::Type>(type));
    }

    parser.ast.push_back(assignment);
    return parser.ast.last_element();
}

// field_assignment → field_assignment "=" expression (":" type)?
std::variant<ast::Node *, Error> parse::parse_field_assignment(
    Parser &parser, ast::Node *identifier
) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse field access
    assignment.assignable = identifier;

    // Parse equal
    auto equal = parse_token(parser, getIndex<token::Kind, token::Equal>());
    if (std::holds_alternative<Error>(equal)) return Error{};

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;
    assignment.expression = std::get<ast::Node *>(expression);

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(assignment.expression, std::get<ast::Type>(type));

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
std::variant<ast::Node *, Error> parse::parse_dereference_assignment(
    Parser &parser
) {
    // Create node
    auto assignment
        = ast::AssignmentNode{current(parser).line, current(parser).column};

    // Parse dereference
    auto identifier = parse_dereference(parser);
    if (std::holds_alternative<Error>(identifier)) return Error{};
    assignment.assignable = std::get<ast::Node *>(identifier);

    // Parse equal
    auto equal = parse_token(parser, getIndex<token::Kind, token::Equal>());
    if (std::holds_alternative<Error>(equal)) return Error{};

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;
    assignment.expression = std::get<ast::Node *>(expression);

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(assignment.expression, std::get<ast::Type>(type));

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
std::variant<ast::Node *, Error> parse::parse_index_assignment(
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
    auto equal = parse_token(parser, getIndex<token::Kind, token::Equal>());
    if (std::holds_alternative<Error>(equal)) return Error{};

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;
    assignment.expression = std::get<ast::Node *>(expression);

    // Parse type annotation
    if (std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);

        auto type = parse_type(parser);
        if (std::holds_alternative<Error>(type)) return Error{};

        ast::set_type(assignment.expression, std::get<ast::Type>(type));

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
std::variant<ast::Node *, Error> parse::parse_return_stmt(Parser &parser) {
    // Create node
    auto return_stmt
        = ast::ReturnNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Return>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse expression
    if (!at_end(parser)
        && !std::holds_alternative<token::NewLine>(current(parser).kind)) {
        auto expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return expression;
        return_stmt.expression = std::get<ast::Node *>(expression);
    }

    parser.ast.push_back(return_stmt);
    return parser.ast.last_element();
}

// break → "break"
std::variant<ast::Node *, Error> parse::parse_break_stmt(Parser &parser) {
    // Create node
    auto break_node
        = ast::BreakNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Break>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    parser.ast.push_back(break_node);
    return parser.ast.last_element();
}

// continue → "continue"
std::variant<ast::Node *, Error> parse::parse_continue_stmt(Parser &parser) {
    // Create node
    auto continue_node
        = ast::ContinueNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword
        = parse_token(parser, getIndex<token::Kind, token::Continue>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    parser.ast.push_back(continue_node);
    return parser.ast.last_element();
}

// if_else → "if" expression block_or_statement ("\n"* "else"
// block_or_statement)
std::variant<ast::Node *, Error> parse::parse_if_else(Parser &parser) {
    size_t indentation_level = current(parser).column;

    // Create node
    auto if_else
        = ast::IfElseNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::If>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse condition
    auto condition = parse_expression(parser);
    if (std::holds_alternative<Error>(condition)) return condition;
    if_else.condition = std::get<ast::Node *>(condition);

    // Parse if branch
    auto block = parse_block_or_statement(parser);
    if (std::holds_alternative<Error>(block)) return Error{};
    if_else.if_branch = std::get<ast::Node *>(block);

    // Adance until new statement
    auto position_backup = parser.current;
    advance_until_next_statement(parser);

    // Match indentation
    if (current(parser).column == indentation_level
        && std::holds_alternative<token::Else>(current(parser).kind)) {
        advance(parser);

        // Parse else branch
        auto block = parse_block_or_statement(parser);
        if (std::holds_alternative<Error>(block)) return Error{};
        if_else.else_branch = std::get<ast::Node *>(block);
    } else {
        parser.current = position_backup;
    }

    parser.ast.push_back(if_else);
    return parser.ast.last_element();
}

// while → "while" expression block
std::variant<ast::Node *, Error> parse::parse_while_stmt(Parser &parser) {
    // Create node
    auto while_stmt
        = ast::WhileNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::While>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse condition
    auto condition = parse_expression(parser);
    if (std::holds_alternative<Error>(condition)) return condition;
    while_stmt.condition = std::get<ast::Node *>(condition);

    // Parse block
    auto block = parse_block(parser);
    if (std::holds_alternative<Error>(block)) return Error{};
    while_stmt.block = std::get<ast::Node *>(block);

    // Return
    parser.ast.push_back(while_stmt);
    return parser.ast.last_element();
}

// use → ("use"|"include") string
std::variant<ast::Node *, Error> parse::parse_use_stmt(Parser &parser) {
    // Create node
    auto use_stmt = ast::UseNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::Use>());
    if (std::holds_alternative<Error>(keyword)) {
        keyword = parse_token(parser, getIndex<token::Kind, token::Include>());
        if (std::holds_alternative<Error>(keyword)) return Error{};
        use_stmt.include = true;
    }

    // Parse path
    auto path = parse_string(parser);
    if (std::holds_alternative<Error>(path)) return path;
    use_stmt.path = (ast::StringNode *)std::get<ast::Node *>(path);

    parser.ast.push_back(use_stmt);
    return parser.ast.last_element();
}

// call_argument → "mut"? IDENTIFIER ":" expression
//               → "mut"? expression
std::variant<ast::Node *, Error> parse::parse_call_argument(Parser &parser) {
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};

    // Parse mut
    if (std::holds_alternative<token::Mut>(current(parser).kind)) {
        arg.is_mutable = true;
        advance(parser);
    }

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    arg.expression = std::get<ast::Node *>(expression);

    // Parse expression if previous was a identifier
    if (std::holds_alternative<ast::IdentifierNode>(*arg.expression)
        && std::holds_alternative<token::Colon>(current(parser).kind)) {
        advance(parser);
        arg.identifier = (ast::IdentifierNode *)arg.expression;

        expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return Error{};
        arg.expression = std::get<ast::Node *>(expression);
    }

    parser.ast.push_back(arg);
    return parser.ast.last_element();
}

// call → IDENTIFIER "(" (call_argument ((","|("\n"+)) call_argument)*)* ")"
std::variant<ast::Node *, Error> parse::parse_call(
    Parser &parser, ast::Node *identifier
) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    call.identifier = (ast::IdentifierNode *)identifier;

    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse args
    while (!std::holds_alternative<token::RightParen>(current(parser).kind)
           && !at_end(parser)) {
        auto arg = parse_call_argument(parser);
        if (std::holds_alternative<Error>(arg)) return Error{};
        call.args.push_back((ast::CallArgumentNode *)std::get<ast::Node *>(arg)
        );

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else if (std::holds_alternative<token::NewLine>(current(parser).kind))
            advance_until_next_statement(parser);
        else
            break;
    }

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};
    call.end_line = std::get<token::Token>(right_paren).line;

    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// struct → IDENTIFIER "{" (IDENTIFIER ": " expression (","|("\n"+)))*  "}"
std::variant<ast::Node *, Error> parse::parse_struct(Parser &parser) {
    // Create node
    auto struct_literal
        = ast::StructLiteralNode{current(parser).line, current(parser).column};

    // Parse indentifier
    auto identifier = parse_identifier(parser);
    if (std::holds_alternative<Error>(identifier)) return identifier;
    struct_literal.identifier
        = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse left curly
    auto left_curly
        = parse_token(parser, getIndex<token::Kind, token::LeftCurly>());
    if (std::holds_alternative<Error>(left_curly)) return Error{};

    // Parse fields
    while (!std::holds_alternative<token::RightCurly>(current(parser).kind)
           && !at_end(parser)) {
        advance_until_next_statement(parser);

        auto identifier = parse_identifier(parser);
        if (std::holds_alternative<Error>(identifier)) return Error{};

        auto colon = parse_token(parser, getIndex<token::Kind, token::Colon>());
        if (std::holds_alternative<Error>(colon)) return Error{};

        auto expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return Error{};

        struct_literal
            .fields[(ast::IdentifierNode *)std::get<ast::Node *>(identifier)]
            = std::get<ast::Node *>(expression);

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else if (std::holds_alternative<token::NewLine>(current(parser).kind))
            advance_until_next_statement(parser);
        else
            break;
    }

    // Parse right curly
    auto right_curly
        = parse_token(parser, getIndex<token::Kind, token::RightCurly>());
    if (std::holds_alternative<Error>(right_curly)) return Error{};
    struct_literal.end_line = std::get<token::Token>(right_curly).line;

    parser.ast.push_back(struct_literal);
    return parser.ast.last_element();
}

// expression → if_else_expression
//            | new
//            | not
//            | binary
std::variant<ast::Node *, Error> parse::parse_expression(Parser &parser) {
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
std::variant<ast::Node *, Error> parse::parse_if_else_expr(Parser &parser) {
    size_t indentation_level = current(parser).column;

    // Create node
    auto if_else
        = ast::IfElseNode{current(parser).line, current(parser).column};

    // Parse keyword
    auto keyword = parse_token(parser, getIndex<token::Kind, token::If>());
    if (std::holds_alternative<Error>(keyword)) return Error{};

    // Parse condition
    auto condition = parse_expression(parser);
    if (std::holds_alternative<Error>(condition)) return condition;
    if_else.condition = std::get<ast::Node *>(condition);

    // Parse if branch
    advance_until_next_statement(parser);
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    if_else.if_branch = std::get<ast::Node *>(expression);

    // Adance until new statement
    auto position_backup = parser.current;
    advance_until_next_statement(parser);

    // Match else
    if (std::holds_alternative<token::Else>(current(parser).kind)) {
        advance(parser);

        // Parse else branch
        advance_until_next_statement(parser);
        auto expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return Error{};
        if_else.else_branch = std::get<ast::Node *>(expression);

        parser.ast.push_back(if_else);
        return parser.ast.last_element();
    } else {
        parser.current = position_backup;
        parser.errors.push_back(
            errors::generic_error(location(parser), "Expecting else branch")
        );
        return Error{};
    }
}

// not → "not" expression
std::variant<ast::Node *, Error> parse::parse_not_expr(Parser &parser) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    auto identifier
        = parse_identifier(parser, getIndex<token::Kind, token::Not>());
    if (std::holds_alternative<Error>(identifier)) return identifier;
    call.identifier = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

    // Parse expression
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    arg.expression = std::get<ast::Node *>(expression);
    parser.ast.push_back(arg);
    call.args.push_back((ast::CallArgumentNode *)parser.ast.last_element());

    // Push node to ast
    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// new → "new" expression
std::variant<ast::Node *, Error> parse::parse_new_expr(Parser &parser) {
    // Create node
    auto new_node = ast::NewNode{current(parser).line, current(parser).column};

    // Parse token
    auto keyword = parse_token(parser, getIndex<token::Kind, token::New>());
    if (std::holds_alternative<Error>(keyword)) return std::get<Error>(keyword);

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    new_node.expression = std::get<ast::Node *>(expression);

    // Push node to ast
    parser.ast.push_back(new_node);
    return parser.ast.last_element();
}

// binary → equality
// equality → comparison (("=="|"!=") comparison)*
// comparison → term ((">"|">="|"<"|"<=") term)*
// term → factor (("+"|"-") factor)*
// factor → primary (("*"|"/"|"%") primary)*
std::variant<ast::Node *, Error> parse::parse_binary(
    Parser &parser, int precedence
) {
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
        auto left_expression = parse_binary(parser, precedence + 1);
        if (std::holds_alternative<Error>(left_expression)) return Error{};
        left_node.expression = std::get<ast::Node *>(left_expression);

        while (true) {
            // Create call node
            auto call
                = ast::CallNode{current(parser).line, current(parser).column};

            // Parse operator
            auto op = current(parser).kind;
            if (operators.find(op.index()) == operators.end()
                || operators[op.index()] != precedence)
                break;

            auto identifier = parse_identifier(parser, op.index());
            if (std::holds_alternative<Error>(identifier)) return identifier;

            // Parse right
            auto right_node = ast::CallArgumentNode{
                current(parser).line,
                current(parser).column
            };
            auto right_expression = parse_binary(parser, precedence + 1);
            if (std::holds_alternative<Error>(right_expression)) return Error{};
            right_node.expression = std::get<ast::Node *>(right_expression);

            // Add identifier to call
            call.identifier
                = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);

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
std::variant<ast::Node *, Error> parse::parse_primary(Parser &parser) {
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
        if (match(
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
    return Error{};
}

// grouping_or_assignable → grouping
//                        | identifier
//                        | index_access
//                        | field_access
//                        | call
std::variant<ast::Node *, Error> parse::parse_grouping_or_assignable(
    Parser &parser
) {
    // Parse indentifier
    std::variant<ast::Node *, Error> assignable;
    if (std::holds_alternative<token::Identifier>(current(parser).kind)) {
        assignable = parse_identifier(parser);
    }
    // Parse grouping
    else if (std::holds_alternative<token::LeftParen>(current(parser).kind)) {
        assignable = parse_grouping(parser);
    } else {
        assert(false);
    }

    if (std::holds_alternative<Error>(assignable)) return assignable;

    // Iterate over assignables
    while (true) {
        if (std::holds_alternative<token::Dot>(current(parser).kind)) {
            assignable
                = parse_field_access(parser, std::get<ast::Node *>(assignable));
        } else if (std::holds_alternative<token::LeftBracket>(
                       current(parser).kind
                   )) {
            assignable
                = parse_index_access(parser, std::get<ast::Node *>(assignable));
        } else if (std::holds_alternative<token::LeftParen>(current(parser).kind
                   )) {
            assignable = parse_call(parser, std::get<ast::Node *>(assignable));
        } else {
            return assignable;
        }

        if (std::holds_alternative<Error>(assignable)) return assignable;
    }
}

// negation → "-" primary
std::variant<ast::Node *, Error> parse::parse_negation(Parser &parser) {
    // Create node
    auto call = ast::CallNode{current(parser).line, current(parser).column};

    // Parse indentifier
    auto identifier
        = parse_identifier(parser, getIndex<token::Kind, token::Minus>());
    if (std::holds_alternative<Error>(identifier)) return identifier;
    call.identifier = (ast::IdentifierNode *)std::get<ast::Node *>(identifier);
    call.identifier->value = "-:negation";

    // Parse expression
    auto arg
        = ast::CallArgumentNode{current(parser).line, current(parser).column};
    auto expression = parse_primary(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    arg.expression = std::get<ast::Node *>(expression);
    parser.ast.push_back(arg);
    call.args.push_back((ast::CallArgumentNode *)parser.ast.last_element());

    // Add call to ast
    parser.ast.push_back(call);
    return parser.ast.last_element();
}

// address_of → "&" (field_access|identifier|index_access)
std::variant<ast::Node *, Error> parse::parse_address_of(Parser &parser) {
    // Create node
    auto address_of
        = ast::AddressOfNode{current(parser).line, current(parser).column};
    advance(parser);

    // Parse expression
    std::variant<ast::Node *, Error> expression
        = parse_grouping_or_assignable(parser);
    if (std::holds_alternative<Error>(expression)) return Error{};
    address_of.expression = std::get<ast::Node *>(expression);

    // Add node to ast
    parser.ast.push_back(address_of);
    return parser.ast.last_element();
}

// dereference → "*" (dereference|assignable)
std::variant<ast::Node *, Error> parse::parse_dereference(Parser &parser) {
    // Create node
    auto dereference
        = ast::DereferenceNode{current(parser).line, current(parser).column};

    // Parse dereference operator
    assert(std::holds_alternative<token::Star>(current(parser).kind));
    advance(parser);

    // Parse expression
    std::variant<ast::Node *, Error> expression;
    if (std::holds_alternative<token::Star>(current(parser).kind)) {
        expression = parse_dereference(parser);
    } else {
        expression = parse_grouping_or_assignable(parser);
    }
    if (std::holds_alternative<Error>(expression)) return Error{};
    dereference.expression = std::get<ast::Node *>(expression);

    // Add node to ast
    parser.ast.push_back(dereference);
    return parser.ast.last_element();
}

// grouping → "(" expression ")"
std::variant<ast::Node *, Error> parse::parse_grouping(Parser &parser) {
    // Parse left paren
    auto left_paren
        = parse_token(parser, getIndex<token::Kind, token::LeftParen>());
    if (std::holds_alternative<Error>(left_paren)) return Error{};

    // Parse expression
    auto expression = parse_expression(parser);
    if (std::holds_alternative<Error>(expression)) return expression;

    // Parse right paren
    auto right_paren
        = parse_token(parser, getIndex<token::Kind, token::RightParen>());
    if (std::holds_alternative<Error>(right_paren)) return Error{};

    return std::get<ast::Node *>(expression);
}

// float → FLOAT
std::variant<ast::Node *, Error> parse::parse_float(Parser &parser) {
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
std::variant<ast::Node *, Error> parse::parse_integer(Parser &parser) {
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
std::variant<ast::Node *, Error> parse::parse_boolean(Parser &parser) {
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
std::variant<ast::Node *, Error> parse::parse_identifier(Parser &parser) {
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
std::variant<ast::Node *, Error> parse::parse_function_identifier(Parser &parser
) {
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
    } else if (match(
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

std::variant<ast::Node *, Error> parse::parse_identifier(
    Parser &parser, size_t token
) {
    // Create node
    auto identifier
        = ast::IdentifierNode{current(parser).line, current(parser).column};

    // Parse identifier
    auto result = parse_token(parser, token);
    if (std::holds_alternative<Error>(result)) return Error{};
    identifier.value = token::getLiteral(std::get<token::Token>(result));

    parser.ast.push_back(identifier);
    return parser.ast.last_element();
}

// string → STRING
std::variant<ast::Node *, Error> parse::parse_string(Parser &parser) {
    // Create node
    auto string = ast::StringNode{current(parser).line, current(parser).column};

    // Parse string
    auto result = parse_token(parser, getIndex<token::Kind, token::String>());
    if (std::holds_alternative<Error>(result)) return Error{};
    string.value = token::getLiteral(std::get<token::Token>(result));

    parser.ast.push_back(string);
    return parser.ast.last_element();
}

// interpolated_string → STRING_LEFT expression (STRING_MIDDLE expression)*
// STRING_RIGHT
std::variant<ast::Node *, Error> parse::parse_interpolated_string(Parser &parser
) {  // Create node
    auto string = ast::InterpolatedStringNode{
        current(parser).line,
        current(parser).column
    };

    // Parse string
    auto result
        = parse_token(parser, getIndex<token::Kind, token::StringLeft>());
    if (std::holds_alternative<Error>(result)) return Error{};
    string.strings.push_back(token::getLiteral(std::get<token::Token>(result)));

    while (true) {
        // Parse expression
        auto expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return expression;
        string.expressions.push_back(std::get<ast::Node *>(expression));

        if (std::holds_alternative<token::StringMiddle>(current(parser).kind)) {
            auto result = parse_token(
                parser,
                getIndex<token::Kind, token::StringMiddle>()
            );
            if (std::holds_alternative<Error>(result)) return Error{};
            string.strings.push_back(
                token::getLiteral(std::get<token::Token>(result))
            );
        } else {
            break;
        }
    }

    // Parse string
    result = parse_token(parser, getIndex<token::Kind, token::StringRight>());
    if (std::holds_alternative<Error>(result)) return Error{};
    string.strings.push_back(token::getLiteral(std::get<token::Token>(result)));

    parser.ast.push_back(string);
    return parser.ast.last_element();
}

// array → "[" (expression ("," expression)*)* "]"
std::variant<ast::Node *, Error> parse::parse_array(Parser &parser) {
    // Create node
    auto array = ast::ArrayNode{current(parser).line, current(parser).column};

    // Parse left bracket
    assert(std::holds_alternative<token::LeftBracket>(current(parser).kind));
    advance(parser);

    // Parse elements
    while (!std::holds_alternative<token::RightBracket>(current(parser).kind)
           && !at_end(parser)) {
        auto expression = parse_expression(parser);
        if (std::holds_alternative<Error>(expression)) return Error{};
        array.elements.push_back(std::get<ast::Node *>(expression));

        if (std::holds_alternative<token::Comma>(current(parser).kind))
            advance(parser);
        else if (std::holds_alternative<token::NewLine>(current(parser).kind))
            advance_until_next_statement(parser);
        else
            break;
    }

    // Parse right bracket
    auto right_bracket
        = parse_token(parser, getIndex<token::Kind, token::RightBracket>());
    if (std::holds_alternative<Error>(right_bracket)) return Error{};

    parser.ast.push_back(array);
    return parser.ast.last_element();
}

// field_access → assignable "." IDENTFIER ("." IDENTIFIER)*
std::variant<ast::Node *, Error> parse::parse_field_access(
    Parser &parser, ast::Node *accessed
) {  // Create node
    auto field_access
        = ast::FieldAccessNode{current(parser).line, current(parser).column};

    // Parse identifier
    field_access.accessed = accessed;

    auto dot = parse_token(parser, getIndex<token::Kind, token::Dot>());
    if (std::holds_alternative<Error>(dot)) return Error{};

    while (std::holds_alternative<token::Identifier>(current(parser).kind)) {
        // Parse identifier
        auto identifier = parse_identifier(parser);
        if (std::holds_alternative<Error>(identifier)) return Error();
        field_access.fields_accessed.push_back(
            (ast::IdentifierNode *)std::get<ast::Node *>(identifier)
        );

        if (std::holds_alternative<token::Dot>(current(parser).kind))
            advance(parser);
        else
            break;
    }

    parser.ast.push_back(field_access);
    return parser.ast.last_element();
}

// index_access → assignable "[" expression "]"
std::variant<ast::Node *, Error> parse::parse_index_access(
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
    auto index = parse_expression(parser);
    if (std::holds_alternative<Error>(index)) return Error{};
    arg2.expression = std::get<ast::Node *>(index);
    parser.ast.push_back(arg2);
    index_access.args.push_back(
        (ast::CallArgumentNode *)parser.ast.last_element()
    );

    // Parse right bracket
    auto right_bracket
        = parse_token(parser, getIndex<token::Kind, token::RightBracket>());
    if (std::holds_alternative<Error>(right_bracket)) return Error{};

    parser.ast.push_back(index_access);
    return parser.ast.last_element();
}

std::variant<token::Token, Error> parse::parse_token(
    Parser &parser, size_t token
) {
    if (current(parser).kind.index() != token) {
        parser.errors.push_back(errors::unexpected_character(location(parser)));
        return Error{};
    }
    auto curr = current(parser);
    advance(parser);
    return curr;
}