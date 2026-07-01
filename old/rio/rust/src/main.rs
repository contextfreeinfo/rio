fn main() {
    let _ = rio_engine::init(None);
    let engine = rio_engine::Engine::new();
    let module = engine.process("hi", rio_engine::hi());
    print!(
        "{}",
        rio_engine::Node::stringify_tree(module.nodes(), engine.interner().clone())
    );
}
