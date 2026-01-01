## Interning

### Interner

An abstraction for providing int ids for unique string values.

    export class Interner {
      private interns: MapBuilder<String, Int> = do {
        let interns = new MapBuilder<String, Int>();

Always initialize empty string to intern id 0.

        interns[""] = 0;
        interns
      };

      public get(string: String): Int {

We always keep empty string at intern 0, so we could specialize around that, but
we don't expect to check on it often, so don't bother. Using a negative default
ensures that "" as 0 also gets handled here.

Also, use `getOr` presuming that it's faster than handling bubbles on most
backends.

        let found = interns.getOr(string, -1);
        when (found) {
          -1 -> do {
            let intern = interns.length;
            interns[string] = intern;
            intern
          }
          else -> found
        }
      }
    }
