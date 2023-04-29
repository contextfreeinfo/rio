import { promises as fs } from "fs";

async function load(path: string) {
  return await WebAssembly.compile(await fs.readFile(path));
}

async function instantiateGreet(module: WebAssembly.Module) {
  const env = {
    print: (address: number) => {
      // Structural access.
      const data = new DataView(memory.buffer);
      const size = data.getUint32(address, true);
      // Array access.
      const bytes = new Uint8Array(memory.buffer, address + 4, size);
      const text = decoder.decode(bytes);
      console.log(text);
    },
  };
  // Instantiate.
  const instance = await WebAssembly.instantiate(module, { env });
  // Extract things for use in env above and for caller.
  const exports = instance.exports as WasminiApp;
  const { memory } = exports;
  const decoder = new TextDecoder();
  return exports;
}

type WasminiApp = {
  main(): void;
  memory: WebAssembly.Memory;
};

const module = await load(Bun.argv[3]);
const exports = await instantiateGreet(module);
exports.main();
