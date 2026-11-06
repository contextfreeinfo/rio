"""rio lsp, the language server, through the protocol as an editor speaks it: python3 tests/lsp.py path/to/rio

The project in tests/lsp/ has what an editor meets: a main file including a part, an imported module
(shapes), and a host module auto-imported through rio.json (gfx, as rinc's pc), each with comments for
hover. Then diagnostics, completion, hover, definition, the outline and formatting, a stress run of edits
and requests the server must survive, and the VS Code extension's JSON files, which must parse."""
import json, os, pathlib, subprocess, sys

rio = os.path.abspath(sys.argv[1])
here = pathlib.Path(__file__).resolve().parent
project = here / "lsp"
fails = []

server = subprocess.Popen([rio, "lsp"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
ids = [0]

def send(m):
    b = json.dumps(m).encode()
    server.stdin.write(b"Content-Length: %d\r\n\r\n" % len(b) + b)
    server.stdin.flush()

def recv():
    n = -1
    while True:
        line = server.stdout.readline()
        if not line:
            sys.exit("FAIL the server stopped (exit %r)" % server.wait())
        if line.startswith(b"Content-Length:"):
            n = int(line[15:])
        if line in (b"\r\n", b"\n"):
            break
    return json.loads(server.stdout.read(n))

def request(method, params):
    ids[0] += 1
    send({"jsonrpc": "2.0", "id": ids[0], "method": method, "params": params})
    reply = recv()
    if reply.get("id") != ids[0]:
        fails.append("%s: a reply to %r" % (method, reply.get("id")))
    return reply.get("result")

def uri(path):
    return pathlib.Path(path).resolve().as_uri()

def open_doc(path, text=None):
    text = text if text is not None else pathlib.Path(path).read_text(encoding="utf-8")
    send({"jsonrpc": "2.0", "method": "textDocument/didOpen",
          "params": {"textDocument": {"uri": uri(path), "languageId": "rio", "version": 1, "text": text}}})
    return text, recv()["params"]["diagnostics"]

def at(text, needle, into=0):
    """the position of needle in text (into: so many characters into it)"""
    i = text.index(needle) + into
    line = text.count("\n", 0, i)
    return {"line": line, "character": i - (text.rfind("\n", 0, i) + 1)}

def ask(method, path, pos):
    return request(method, {"textDocument": {"uri": uri(path)}, "position": pos})

def check(what, ok, got=None):
    if not ok:
        fails.append("%s: got %r" % (what, got))

# the server's start: what it can do
caps = request("initialize", {"processId": None, "capabilities": {}})["capabilities"]
for c in ("completionProvider", "hoverProvider", "definitionProvider", "documentSymbolProvider", "documentFormattingProvider"):
    check("can " + c, c in caps, caps)
send({"jsonrpc": "2.0", "method": "initialized", "params": {}})

main, part, shapes, gfx = project / "main.rio", project / "part.rio", project / "shapes.rio", project / "gfx.rio"

# diagnostics: none in the project; an error's range exactly; one in a part, found through its includer
text, diag = open_doc(main)
check("no errors in main.rio", diag == [], diag)
_, diag = open_doc(project / "bad.rio", "x := 1\ny := nope + x\n")
check("an error's range", len(diag) == 1 and diag[0]["range"] == {"start": {"line": 1, "character": 5}, "end": {"line": 1, "character": 9}}
      and diag[0]["message"].startswith("undefined name"), diag)
part_text, diag = open_doc(part)
check("a part, checked through main.rio (its P known)", diag == [], diag)
_, diag = open_doc(part, part_text.replace("q.across * q.across", "q.across * q.nope"))
check("a part's own error, in the part", len(diag) == 1 and diag[0]["range"]["start"]["line"] == 4, diag)
open_doc(part, part_text)

# completion: members, a module's names, a host module's, words by place, a part's
def labels(path, pos):
    return {i["label"]: i.get("detail") for i in ask("textDocument/completion", path, pos)}
got = labels(main, at(text, "p.across", 2))
check("p.: its fields, with their types", got.get("across") == "Int" and got.get("down") == "Int", got)
got = labels(main, at(text, "shapes.area", 7))
check("shapes.: its exported proc, with its signature", got.get("area") == "proc(Int, Int) -> Int", got)
got = labels(main, at(text, "gfx.cls", 4))
check("gfx.: the host module's (rio.json)", got.get("cls") == "proc(Int)", got)
got = labels(main, at(text, "log(p", 0))
check("a statement's place: names and words", "log" in got and "if" in got and "P" in got and "and" not in got, got)
got = labels(part, at(part_text, "q.across", 2))
check("in a part: P's fields, from main.rio", "across" in got and "down" in got, got)

# hover: types and signatures, with the comments above (or after) a declaration
def hover(path, pos):
    h = ask("textDocument/hover", path, pos)
    return h["contents"]["value"] if h else ""
got = hover(main, at(text, "p.across", 3))
check("hover: a field, its comment after it", "across: Int" in got and "from the left" in got, got)
got = hover(main, at(text, "far(p)", 1))
check("hover: a proc from a part, its comment above", "far :: proc(P) -> Int" in got and "its distance, squared" in got, got)
got = hover(main, at(text, "shapes.area", 8))
check("hover: an imported module's proc", "area :: proc(Int, Int) -> Int" in got and "the area of a w by h rectangle" in got, got)
got = hover(main, at(text, "gfx.cls", 5))
check("hover: a host proc (rio.json)", "cls :: proc(Int)" in got and "fill the screen with color c" in got, got)
got = hover(main, at(text, "P :: struct", 0))
check("hover: a name on its own declaration's line", "P :: struct" in got and "a point on the screen" in got, got)
got = hover(main, at(text, "log(", 0))
check("hover: a builtin", "log" in got, got)

# go to definition: into a part, a module, a host module, os, a field's line
def definition(path, pos):
    d = ask("textDocument/definition", path, pos)
    return d and (pathlib.Path(d["uri"].replace("file:///", "/").replace("file://", "")).name, d["range"]["start"]["line"])
check("definition: a field", definition(main, at(text, "p.across", 3)) == ("main.rio", 3), definition(main, at(text, "p.across", 3)))
check("definition: into a part", definition(main, at(text, "far(p)", 1)) == ("part.rio", 3), definition(main, at(text, "far(p)", 1)))
check("definition: into an imported module", definition(main, at(text, "shapes.area", 8)) == ("shapes.rio", 3),
      definition(main, at(text, "shapes.area", 8)))
check("definition: into a host module (rio.json)", definition(main, at(text, "gfx.cls", 5)) == ("gfx.rio", 3),
      definition(main, at(text, "gfx.cls", 5)))
check("definition: a module, its own file", definition(main, at(text, "shapes.area", 1)) == ("shapes.rio", 0),
      definition(main, at(text, "shapes.area", 1)))
os_text, _ = open_doc(project / "useos.rio", "import os\nos.writeErr(\"x\")\n")
got = definition(project / "useos.rio", at(os_text, "writeErr", 2))
check("definition: into os, built into rio", got is not None and got[0] == "rio-lsp-os.rio" and got[1] > 0, got)

# the outline: this file's own declarations (a part's in the part)
names = [s["name"] for s in request("textDocument/documentSymbol", {"textDocument": {"uri": uri(main)}})]
check("outline of main.rio", "P" in names and "p" in names and "far" not in names, names)
names = [s["name"] for s in request("textDocument/documentSymbol", {"textDocument": {"uri": uri(part)}})]
check("outline of a part", names == ["far"], names)

# formatting: rio fmt's layout
open_doc(project / "messy.rio", "x:=f( 1,2 )\n")
edits = request("textDocument/formatting", {"textDocument": {"uri": uri(project / "messy.rio")}, "options": {}})
check("formatting", edits and edits[0]["newText"] == "x := f(1, 2)\n", edits)

# a stress run: rio's own big test file, each line's prefix as edits come, completion all over: it lives
big = (here / "test.rio").read_text(encoding="utf-8")
big_path = here / "test.rio"
open_doc(big_path, big)
lines = big.split("\n")
for n in range(0, len(lines), 23):
    for ch in (0, len(lines[n]) // 2, len(lines[n])):
        ask("textDocument/completion", big_path, {"line": n, "character": ch})
        ask("textDocument/hover", big_path, {"line": n, "character": ch})
for k in range(0, len(big), max(1, len(big) // 60)):
    send({"jsonrpc": "2.0", "method": "textDocument/didChange",
          "params": {"textDocument": {"uri": uri(big_path), "version": 2}, "contentChanges": [{"text": big[:k]}]}})
    recv()

request("shutdown", None)
send({"jsonrpc": "2.0", "method": "exit"})
check("exits when asked", server.wait(timeout=10) == 0, server.returncode)

# the extension: its JSON files parse (a grammar that doesn't, and VS Code shows no colors at all)
ext = here.parent / "editors" / "vscode"
for f in ("package.json", "language-configuration.json", "syntaxes/rio.tmLanguage.json"):
    try:
        json.loads((ext / f).read_text(encoding="utf-8"))
    except Exception as e:
        fails.append("extension's %s: %s" % (f, e))

for f in fails:
    print("FAIL " + f)
print("all lsp tests passed" if not fails else "%d failed" % len(fails))
sys.exit(1 if fails else 0)
