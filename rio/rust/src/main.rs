fn main() {
    let _ = rio_engine::init(None);
    let interner = rio_engine::Interner::new();

    // Lex
    let lexer = rio_engine::Lexer::new(Some(interner.clone()));
    let tokens = lexer.lex(rio_engine::hi());

    // Parse
    let parser = rio_engine::Parser::new();
    let parse_tree = parser.parse(tokens);
    // print!(
    //     "{}",
    //     rio_engine::ParseNode::stringify_tree(parse_tree.clone(), interner.clone())
    // );

    // Norm
    let normer = rio_engine::Normer::new(interner.clone());
    let normed = normer.norm(parse_tree);

    // Resolve
    let resolver = rio_engine::Resolver::new();
    resolver.resolve(normed.clone());
    print!(
        "{}",
        rio_engine::Node::stringify_tree(normed.nodes(), interner.clone())
    );
}
