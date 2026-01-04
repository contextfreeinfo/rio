fn main() {
    let _ = rio_engine::init(None);
    let interner = rio_engine::Interner::new();
    // Lex
    let lexer = rio_engine::Lexer::new(Some(interner.clone()));
    let tokens = lexer.lex(rio_engine::hi());
    // Parse
    let parser = rio_engine::Parser::new();
    let parse_tree = parser.parse(tokens);
    print!(
        "{}",
        rio_engine::ParseNode::stringify_tree(parse_tree.clone(), interner.clone())
    );
}
