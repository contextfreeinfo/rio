async function run() {
  const response = await fetch("fib.wasm");
  const typedArray = await response.arrayBuffer();
  const instantiation = await WebAssembly.instantiate(
    typedArray,
    {
      wasi_snapshot_preview1: {
        fd_write: () => { console.log("Whatever"); }
      }
    },
  );
  const printI32 = instantiation.instance.exports["print-i32"];
  printI32(-456);
}

run();
