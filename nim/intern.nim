import std/tables

# TODO Make something efficient like: https://forum.nim-lang.org/t/7509#47727

type
  Pool*[Id] = ref object
    ids: Table[string, Id]
    size: Id
    texts: Table[Id, string]

const
  emptyId* = 1
  blankId* = 2
  pubId* = 3
  addId* = 4
  subId* = 5
  eqId* = 6
  geId* = 7
  gtId* = 8
  leId* = 9
  ltId* = 10
  neId* = 11

proc newPool*[Id](): Pool[Id] =
  var pool = Pool[Id](
    ids: initTable[string, Id](),
    size: 0,
    texts: initTable[Id, string](),
  )
  assert pool.intern("") == emptyId
  assert pool.intern("_") == blankId
  assert pool.intern("pub") == pubId
  assert pool.intern("add") == addId
  assert pool.intern("sub") == subId
  assert pool.intern("eq") == eqId
  assert pool.intern("ge") == geId
  assert pool.intern("gt") == gtId
  assert pool.intern("le") == leId
  assert pool.intern("lt") == ltId
  assert pool.intern("ne") == neId
  pool

func `[]`*[Id](pool: Pool[Id], id: Id): string = pool.texts[id]

proc intern*[Id](pool: var Pool[Id], text: string): Id =
  let id = pool.ids.getOrDefault(text, 0)
  if id > 0:
    return id
  let next = pool.size + 1
  pool.ids[text] = next
  pool.size = next
  pool.texts[next] = text
  next

func size*[Id](pool: Pool[Id]): Id = pool.size
