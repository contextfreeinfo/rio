#pragma once

#include "common.h"

namespace rio {

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;

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
    String,
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
  bool published;
  Token::Kind kind;
  Scope scope;
  Node* params;
  Node* expr;
};

struct ParentNode {
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
    Block,
    Call,
    Cast,
    Const,
    Float,
    Fun,
    Int,
    Ref,
    String,
    Tuple,
    Use,
  };

  Kind kind;
  Type type;

  union {
    BlockNode Block;
    BinaryNode Cast;
    BinaryNode Const;
    CallNode Call;
    StringNode Float;
    FunNode Fun;
    StringNode Int;
    RefNode Ref;
    StringNode String;
    ParentNode Tuple;
    UseNode Use;
  };

};

}
