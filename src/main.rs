use std::{env, fs, process};

mod error;
mod lexer;
mod token;

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 || 2 < args.len() {
        println!("Usage: jlox [script]");
        process::exit(64);
    } else if args.len() == 2 {
        run_file(&args[1]);
    } else {
        //runPrompt();
    }
}

fn run_file(path: &String) {
    let content = fs::read_to_string(path).expect("Expected to be able to read the file");
    run(content);
}

fn run(source: String) {
    let result = lexer::lex(source);

    match result {
        Ok(tokens) => {
            for token in tokens {
                println!("{:?}", token);
            }
        }
        Err(errors) => {
            for error in errors {
                println!("{:?}", error);
            }
        }
    }
}
