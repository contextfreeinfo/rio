use std::sync::{Arc, Mutex};

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
    let _ = normed;
    let buffer = Arc::new(Mutex::new(String::new()));
    let stringer = rio_engine::TreeStringerBuilder {
        interner: interner.clone(),
        appender: {
            let buffer = buffer.clone();
            Arc::new(move |string: Arc<String>| buffer.lock().unwrap().push_str(string.as_str()))
        },
    }
    .build();
    normed
        .nodes()
        .read()
        .unwrap()
        .last()
        .unwrap()
        .stringify(stringer);
    print!("{}", &buffer.lock().unwrap());
}
