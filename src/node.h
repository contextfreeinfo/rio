#pragma once

#include "common.h"

namespace rio {

struct Pos {
  usize index;
  usize line;
  usize col;
};

// Token.

struct Token {
  enum struct Kind {
    // TODO Use cache files to remember ids of enums, so source order doesn't
    // TODO matter.
    None,
    Assign,
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
    Int,
    Junk,
    LineEnd,
    Minus,
    Plus,
    RoundL,
    RoundR,
    String,
    Update,
  };
  Kind kind;
  // TODO We don't need file name on each individual token.
  string file;
  Pos begin;
  usize len;
  union {
    string text;
  };
};

// Type.

struct Type {

  enum struct Kind {
    None,
    // TODO C Types.
    // Float.
    FloatLike,
    F32,
    F64,
    // Int.
    IntLike,
    I8,
    I16,
    I32,
    I64,
    ISize,
    // Unsigned.
    U8,
    U16,
    U32,
    U64,
    USize,
    // Pointers.
    Ref,
    MultiRef,
    // Aggregates.
    // String has special treatment because it needs interop.
    // Known semantics, auto conversion out, ...
    String,
    // Only applies at compile time.
    Type,
  };

  Kind kind;

};

// Node.

struct Node;

struct Def {
  string name;
  Node* node;
  Type type;
  Opt<Node> value;
};

struct Scope {
  Slice<Def*> defs;
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
  Scope scope;
  Node* expr;
};

struct ParentNode {
  Slice<Node*> items;
};

struct RefNode {
  string name;
};

struct StringNode {
  string text;
};

struct Node {

  enum struct Kind {
    None,
    Block,
    Const,
    Call,
    Float,
    Fun,
    Int,
    Ref,
    String,
    Tuple,
  };

  Kind kind;
  Type type;

  union {
    BlockNode Block;
    BinaryNode Const;
    CallNode Call;
    StringNode Float;
    FunNode Fun;
    StringNode Int;
    RefNode Ref;
    StringNode String;
    ParentNode Tuple;
  };

};

}
