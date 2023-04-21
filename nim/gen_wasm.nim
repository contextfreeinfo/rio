import parse
import run
import wasm

proc gen_wasm*(grower: Grower, module: Module): seq[uint8] =
  result.add(magic)
  result.add(version)
