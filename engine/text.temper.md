## Text utilities

### Interner

An abstraction for providing int ids for unique string values.

    export class Interner {

We need two-way mapping if we want strings back out.

      private strings: MapBuilder<Int, String> = new MapBuilder();
      private ids: MapBuilder<String, Int> = do {
        let ids = new MapBuilder<String, Int>();

Always initialize empty string to intern id 0.

        ids[""] = 0;
        strings[0] = "";
        ids
      };

      public get(string: String): Int {

We always keep empty string at id 0, so we could specialize around that, but
we don't expect to check on it often, so don't bother. Using a negative default
ensures that "" as 0 also gets handled here.

Also, use `getOr` presuming that it's faster than handling bubbles on most
backends.

        let found = ids.getOr(string, -1);
        when (found) {
          -1 -> do {
            let id = ids.length;
            ids[string] = id;
            strings[id] = string;
            id
          }
          else -> found
        }
      }

TODO Should this panic, since we expect to use it only in controlled cases?

      public string(id: Int): String? {
        when (id) {
          0 -> "";
          else -> do {
            let s = strings.getOr(id, "");
            when (s) {
              "" -> null;
              else -> s;
            }
          }
        }
      }
    }

### Character classes

We're starting out with just ASCII support. Maybe Temper should get fuller
Unicode support sometime, because replicating here won't be fun. How consistent
is different target std lib support for Unicode character sets?

#### isDigit

    let isDigit(c: Int): Boolean {
      c >= char"0" && c <= char"9"
    }

#### isLetter

This one especially might should be Unicode.

    let isLetter(c: Int): Boolean {
      c >= char"A" && c <= char"Z" ||
      c >= char"a" && c <= char"z"
    }
