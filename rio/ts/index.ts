import * as rio from "rio-engine";

function main() {
    const engine = new rio.Engine();
    const module = engine.process("hi", rio.hi);
    console.log(rio.Node.stringifyTree(module.nodes, engine.interner));
}

main()
