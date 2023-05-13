import parse
import run
import wasm

type
  Genner = object
    grower: Grower
    tree: Tree

  Genning = ptr Genner

proc gen_header(gen: Genning) =
  gen.grower.bytes.add(magic)
  gen.grower.bytes.add(version)

proc gen_typesec(gen: Genning) =
  discard

proc gen_wasm*(grower: var Grower, module: Module): seq[uint8] =
  var
    genner = Genner(grower: grower, tree: module.resolved)
    gen = addr genner
  grower.bytes.setLen(0)
  gen.gen_header
  gen.gen_typesec
  # TODO typesec (separate pass through tree?)
  # TODO importsec
  # TODO funcsec
  # TODO memsec
  # TODO exportsec
  # TODO codesec
  # TODO datasec
  grower.bytes
