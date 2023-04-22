let
  function* = uint8 0x60
  magic* = [uint8 0x00, 0x61, 0x73, 0x6d]
  version* = [uint8 0x01, 0x00, 0x00, 0x00]

type
  FunctionType* = object
    input: seq[ValType]
    output: seq[ValType]

  Import* = object
    module*: string
    name*: string

  SectionId* = enum
    typeSec = 0x01
    importSec = 0x02
    funcSec = 0x03
    tableSec = 0x04
    memSec = 0x05
    globalSec = 0x06
    exportSec = 0x07
    startSec = 0x08
    elemSec = 0x09
    codeSec = 0x0a
    dataSec = 0x0b
    datacountSec = 0x0c # Appears before codeSec if present.

  ValType* = enum
    ## These are negative leb128 values.
    emptyType = 0x40
    externrefType = 0x6f
    funcrefType = 0x70
    v128Type = 0x7b
    f64Type = 0x7c
    f32Type = 0x7d
    i64Type = 0x7e
    i32Type = 0x7f

func encode_uleb128*(buffer: var seq[uint8], n: uint32) =
  var m = n
  while true:
    var b = uint8(m and 0x7F)
    m = m shr 7
    if m != 0:
      b = b or 0x80
    buffer.add(b)
    if m == 0:
      break
