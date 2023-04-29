import parse
import run
import wasm

proc gen_header(grower: var Grower) =
  grower.bytes.add(magic)
  grower.bytes.add(version)

proc gen_wasm*(grower: var Grower, module: Module): seq[uint8] =
  grower.bytes.setLen(0)
  grower.gen_header
  # TODO typesec (separate pass through tree?)
  # TODO importsec
  # TODO funcsec
  # TODO memsec
  # TODO exportsec
  # TODO codesec
  # TODO datasec
  grower.bytes
