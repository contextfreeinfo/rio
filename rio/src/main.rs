fn main() {
    let _ = rio_engine::init(None);
    let interner = rio_engine::Interner::new();
    let lexer = rio_engine::Lexer::new(Some(interner.clone()));
    let tokens = lexer.lex(SOURCE);
    println!("Tokens: {}", tokens.len());
    for token in tokens.iter() {
        println!("Tokens: {}", token.stringify(interner.clone()));
    }
}

const SOURCE: &str = r#"
pub fun main(sys)
    # Prove we can call twice.
    greet()
    greet()
end

fun greet()
    # Had some parsing bugs, so try string variety.
    log("I say, \"Hi!\"")
    log("Now bye.")
    log("")
end
"#;
