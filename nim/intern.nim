import std/tables

# TODO Make something efficient like: https://forum.nim-lang.org/t/7509#47727

type
  Pool*[Id] = ref object
    ids: Table[string, Id]
    size: Id
    texts: Table[Id, string]

proc newPool*[Id](): Pool[Id] =
  var pool = Pool[Id](
    ids: initTable[string, Id](), size: 0, texts: initTable[Id, string]()
  )
  discard pool.intern("")
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
