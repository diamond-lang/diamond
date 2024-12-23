use crate::error::Error;
use crate::token::{Token, TokenKind};

struct Lexer {
    source: Vec<u8>,
    tokens: Vec<Token>,
    errors: Vec<Error>,
    start: usize,
    current: usize,
    line: usize,
    column: usize,
}

pub fn lex(source: String) -> Result<Vec<Token>, Vec<Error>> {
    let mut lexer = Lexer {
        source: source.into_bytes(),
        tokens: Vec::new(),
        errors: Vec::new(),
        start: 0,
        current: 0,
        line: 1,
        column: 1,
    };

    scan_tokens(&mut lexer);
    if lexer.errors.len() == 0 {
        return Ok(lexer.tokens);
    } else {
        return Err(lexer.errors);
    }
}

fn scan_tokens(lexer: &mut Lexer) {
    while !at_end(lexer) {
        scan_token(lexer);
    }

    lexer.tokens.push(Token {
        line: lexer.line,
        column: lexer.column,
        kind: TokenKind::EndOfFile,
    });
}

fn scan_token(lexer: &mut Lexer) {
    lexer.start = lexer.current;
    let char = advance(lexer);
    match char {
        '(' => add_token(lexer, TokenKind::LeftParen),
        ')' => add_token(lexer, TokenKind::RightParen),
        '[' => add_token(lexer, TokenKind::LeftBracket),
        ']' => add_token(lexer, TokenKind::RightBracket),
        '{' => add_token(lexer, TokenKind::LeftCurly),
        '}' => add_token(lexer, TokenKind::RightCurly),
        '+' => add_token(lexer, TokenKind::Plus),
        '*' => add_token(lexer, TokenKind::Star),
        '/' => add_token(lexer, TokenKind::Slash),
        '%' => add_token(lexer, TokenKind::Modulo),
        ',' => add_token(lexer, TokenKind::Comma),
        '&' => add_token(lexer, TokenKind::Ampersand),
        ':' => match matches(lexer, "=") {
            true => add_token(lexer, TokenKind::ColonEqual),
            false => add_token(lexer, TokenKind::Colon),
        },
        '!' => match matches(lexer, "=") {
            true => add_token(lexer, TokenKind::NotEqual),
            _ => {
                lexer.errors.push(Error {
                    message: "Error: Unrecognized character".to_owned(),
                    line: lexer.line,
                    column: lexer.column,
                });
            }
        },
        '=' => match matches(lexer, "=") {
            true => add_token(lexer, TokenKind::EqualEqual),
            false => add_token(lexer, TokenKind::Equal),
        },
        '>' => match matches(lexer, "=") {
            true => add_token(lexer, TokenKind::GreaterEqual),
            false => add_token(lexer, TokenKind::Greater),
        },
        '<' => match matches(lexer, "=") {
            true => add_token(lexer, TokenKind::LessEqual),
            false => add_token(lexer, TokenKind::Less),
        },
        '.' => match peek(lexer).is_digit(10) {
            true => scan_number(lexer),
            false => add_token(lexer, TokenKind::Dot),
        },
        '-' => match matches(lexer, "--") {
            true => {
                while !(at_end(lexer) || matches(lexer, "---")) {
                    advance(lexer);
                }
                if at_end(lexer) {
                    lexer.errors.push(Error {
                        message: "Error: Unclosed block comment".to_owned(),
                        line: lexer.line,
                        column: lexer.column,
                    });
                }
            }
            false => match matches(lexer, "-") {
                true => {
                    advance_until_newline(lexer);
                    advance(lexer);
                }
                false => add_token(lexer, TokenKind::Minus),
            },
        },
        '_' => scan_identifier_or_keyword(lexer),
        ' ' | '\t' => {}
        '\n' => add_token(lexer, TokenKind::NewLine),
        '\r' => {
            if matches(lexer, "\n") {
                add_token(lexer, TokenKind::NewLine)
            } else {
                panic!()
            }
        }
        '\"' => scan_string(lexer),
        _ => {
            if char.is_digit(10) {
                scan_number(lexer);
            } else if char.is_alphanumeric() {
                return scan_identifier_or_keyword(lexer);
            } else {
                lexer.errors.push(Error {
                    message: "Error: Unrecognized character".to_owned(),
                    line: lexer.line,
                    column: lexer.column,
                });
            }
        }
    }
}

fn advance(lexer: &mut Lexer) -> char {
    if peek(lexer) == '\n' || (peek(lexer) == '\r' && peek_next(lexer) == '\n') {
        lexer.line += 1;
        lexer.column = 1;
    } else {
        lexer.column += 1;
    }
    lexer.current += 1;
    return char::from(lexer.source[lexer.current - 1]);
}

fn at_end(lexer: &Lexer) -> bool {
    lexer.current >= lexer.source.len()
}

fn add_token(lexer: &mut Lexer, token: TokenKind) {
    lexer.tokens.push(Token {
        kind: token,
        line: lexer.line,
        column: lexer.column - (lexer.current - lexer.start),
    });
}

fn matches(lexer: &mut Lexer, string: &str) -> bool {
    let mut it_matches = true;
    let mut i = 0;
    while !at_end(lexer) && i < string.len() {
        if peek(lexer) != string.as_bytes()[i] as char {
            it_matches = false;
            lexer.current -= i;
            break;
        }
        advance(lexer);
        i += 1;
    }
    if at_end(lexer) && i != string.len() {
        it_matches = false;
        lexer.current -= i;
    }

    return it_matches;
}

fn peek(lexer: &Lexer) -> char {
    if at_end(lexer) {
        return '\0';
    }
    return lexer.source[lexer.current] as char;
}

fn previous(lexer: &Lexer) -> char {
    if lexer.current > 0 {
        return lexer.source[lexer.current - 1] as char;
    } else {
        return lexer.source[lexer.current] as char;
    }
}

fn peek_next(lexer: &Lexer) -> char {
    if lexer.current + 1 >= lexer.source.len() {
        return '\0';
    }
    return lexer.source[lexer.current + 1] as char;
}

fn scan_string(lexer: &mut Lexer) {
    while peek(lexer) != '"' && !at_end(lexer) {
        if peek(lexer) == '\n' {
            lexer.line += 1
        }
        advance(lexer);
    }

    if at_end(lexer) {
        lexer.errors.push(Error {
            message: String::from("Unterminated string"),
            line: lexer.line,
            column: lexer.column,
        });
        return;
    }

    advance(lexer);

    let value = lexer.source[lexer.start + 1..lexer.current - 1].to_vec();
    add_token(
        lexer,
        TokenKind::String {
            literal: String::from_utf8(value.clone()).unwrap(),
        },
    );
}

fn scan_number(lexer: &mut Lexer) {
    let mut is_float = false;

    // eg: .8
    if previous(lexer) == '.' && peek(lexer).is_digit(10) {
        // consume digits
        while peek(lexer).is_digit(10) {
            advance(lexer);
        }

        is_float = true;
    }
    // eg: 3.5
    else {
        // consume digits
        while peek(lexer).is_digit(10) {
            advance(lexer);
        }

        if matches(lexer, ".") {
            // consume digits
            while peek(lexer).is_digit(10) {
                advance(lexer);
            }

            is_float = true;
        }
    }

    let literal = lexer.source[lexer.start..lexer.current].to_vec();
    let literal = String::from_utf8(literal.clone()).unwrap();
    add_token(
        lexer,
        if is_float {
            TokenKind::Float { literal }
        } else {
            TokenKind::Integer { literal }
        },
    );
}

fn scan_identifier_or_keyword(lexer: &mut Lexer) {
    while peek(lexer).is_alphanumeric() {
        advance(lexer);
    }

    let literal = lexer.source[lexer.start..lexer.current].to_vec();
    let literal = String::from_utf8(literal.clone()).unwrap();

    match &literal[..] {
        "if" => add_token(lexer, TokenKind::If),
        "else" => add_token(lexer, TokenKind::Else),
        "while" => add_token(lexer, TokenKind::While),
        "function" => add_token(lexer, TokenKind::Function),
        "interface" => add_token(lexer, TokenKind::Interface),
        "builtin" => add_token(lexer, TokenKind::Builtin),
        "type" => add_token(lexer, TokenKind::Type),
        "case" => add_token(lexer, TokenKind::Case),
        "be" => add_token(lexer, TokenKind::Be),
        "true" => add_token(lexer, TokenKind::True),
        "false" => add_token(lexer, TokenKind::False),
        "and" => add_token(lexer, TokenKind::And),
        "or" => add_token(lexer, TokenKind::Or),
        "use" => add_token(lexer, TokenKind::Use),
        "include" => add_token(lexer, TokenKind::Include),
        "break" => add_token(lexer, TokenKind::Break),
        "continue" => add_token(lexer, TokenKind::Continue),
        "return" => add_token(lexer, TokenKind::Return),
        "mut" => add_token(lexer, TokenKind::Mut),
        "new" => add_token(lexer, TokenKind::New),
        "not" => add_token(lexer, TokenKind::Not),
        "extern" => add_token(lexer, TokenKind::Extern),
        "link_with" => add_token(lexer, TokenKind::LinkWith),
        _ => add_token(lexer, TokenKind::Identifier { literal }),
    }
}

fn advance_until_newline(lexer: &mut Lexer) {
    while peek(lexer) != '\n' {
        advance(lexer);
    }
}
