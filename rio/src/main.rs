fn main() {
    let _ = rio_engine::init(None);
    let interner = rio_engine::Interner::new();
    let lexer = rio_engine::Lexer::new(Some(interner.clone()));
    let tokens = lexer.lex(rio_engine::hi());
    println!("Tokens:");
    for token in tokens.iter() {
        println!("{}", token.stringify(interner.clone()));
    }
    // TODO Print parse tree
}
