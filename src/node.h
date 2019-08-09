#pragma once

#include "common.h"

namespace rio {

struct ModManager;

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;

struct Pos {
  usize index;
  usize line;
  usize col;
};

// Def.

struct Node;

struct Def {
  string name;
  Node* node;
  Opt<Node> value;
  // Only for globals. If present, references the mod root.
  Opt<ModManager> mod;
};

// Token.

struct Token {
  enum struct Kind {
    // TODO Use cache files to remember ids of enums, so source order doesn't
    // TODO matter.
    None,
    Assign,
    Colon,
    Comma,
    Comment,
    CurlyL,
    CurlyR,
    Do,
    Dot,
    End,
    FileEnd,
    Float,
    Fun,
    Id,
    Include,
    Int,
    Junk,
    LineEnd,
    Minus,
    Plus,
    Proc,
    Pub,
    RoundL,
    RoundR,
    SquareL,
    SquareR,
    String,
    Struct,
    Update,
    Use,
  };
  Kind kind;
  // TODO We don't need file name on each individual token.
  string file;
  Pos begin;
  usize len;
  union {
    opt_string text;
  };
};

// Type.

struct Type {

  enum struct Kind {
    None,
    Void,
    // TODO C Types.
    // Float.
    F32,
    F64,
    // Int.
    I8,
    I16,
    I32,
    I64,
    ISize,
    Int,
    // Unsigned.
    U8,
    U16,
    U32,
    U64,
    USize,
    // Pointers.
    Ref,
    MultiRef,
    NullTermRef,
    // Aggregates.
    Array,
    Proc,
    // String has special treatment because it needs interop.
    // Known semantics, auto conversion out, custom by back end?, ...
    String,
    // Custom.
    User,
    // Only applies at compile time.
    Type,
  };

  Kind kind;

  Opt<Type> arg;

  // TODO Push basic types through defs, too?
  Opt<Def> def;

  Opt<Node> node;

};

// Node.

struct Scope {

  Slice<Def*> defs;

  auto forget() -> void {
    defs.forget();
  }

};

struct BlockNode {
  Scope scope;
  Slice<Node*> items;
};

struct BinaryNode {
  Node* a;
  Node* b;
};

struct CallNode {
  Node* args;
  Node* callee;
};

struct FunNode {
  string name;
  bool published;
  // Scope and params both useless for structs.
  Scope scope;
  Opt<Node> params;
  Node* expr;
};

struct ParentNode {
  // TODO Scope for tuples, if we want to track named field defs?
  Slice<Node*> items;
};

struct RefNode {
  string name;
  Opt<Def> def;
};

struct StringNode {
  string text;
};

struct UseNode {
  string name;
  Token::Kind kind;
  Node* arg;
};

struct Node {

  enum struct Kind {
    None,
    Array,
    Block,
    Call,
    Cast,
    Const,
    Float,
    Fun,
    Int,
    Map,
    Member,
    Proc,
    Ref,
    String,
    Struct,
    Tuple,
    Use,
  };

  Kind kind;
  // TODO Maybe make type a pointer so we can reduce memory usage.
  Type type;

  union {
    ParentNode Array;
    BlockNode Block;
    BinaryNode Cast;
    BinaryNode Const;
    CallNode Call;
    StringNode Float;
    FunNode Fun;
    StringNode Int;
    ParentNode Map;
    BinaryNode Member;
    RefNode Ref;
    StringNode String;
    ParentNode Tuple;
    UseNode Use;
  };

};

}
