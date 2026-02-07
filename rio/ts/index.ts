import * as engine from "rio-engine";

function main() {
    const interner = new engine.Interner();

    // Lex
    const lexer = new engine.Lexer(interner);
    const tokens = lexer.lex(engine.hi);

    // Parse
    const parser = new engine.Parser();
    const parseTree = parser.parse(tokens);
    // console.log(engine.ParseNode.stringifyTree(parseTree, interner));

    // Norm
    const normer = new engine.Normer(interner);
    const normed = normer.norm(parseTree);
    console.log(engine.Node.stringifyTree(normed.nodes, interner));

    // Resolve
    const resolver = new engine.Resolver();
    resolver.resolve(normed);
}

main()
