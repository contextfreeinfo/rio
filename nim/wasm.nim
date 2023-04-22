let
  function* = uint8 0x60
  magic* = [uint8 0x00, 0x61, 0x73, 0x6D]
  version* = [uint8 0x01, 0x00, 0x00, 0x00]

type
  NumType* = enum
    f64 = 0x7C
    f32 = 0x7D
    i64 = 0x7E
    i32 = 0x7F

  Section* = enum
    funcSec = 0x01
    importSec = 0x02

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
