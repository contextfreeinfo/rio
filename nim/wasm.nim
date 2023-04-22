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

  NumType* = enum
    f64 = 0x7c
    f32 = 0x7d
    i64 = 0x7e
    i32 = 0x7f

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

  ValTypeKind* = enum
    numTypeKind

  ValType* = object
    case kind*: ValTypeKind
    of numTypeKind:
      numType: NumType

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
