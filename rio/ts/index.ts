import * as engine from "rio-engine";

function main() {
    const eng = new engine.Engine();
    const tree = eng.process("hi", engine.hi);
    console.log(engine.Node.stringifyTree(tree.nodes, eng.interner));
}

main()
