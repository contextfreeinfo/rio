#pragma once

#include "common.h"

namespace rio {

struct Pos {
  usize index;
  usize line;
  usize col;
};

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
  const char* file;
  Pos begin;
  usize len;
  union {
    const char* text;
  };
};

struct Node;

struct BinaryNode {
  Node* a;
  Node* b;
};

struct CallNode {
  Node* args;
  Node* callee;
};

struct FunNode {
  const char* name;
  Node* expr;
};

struct ParentNode {
  Slice<Node*> items;
};

struct RefNode {
  const char* name;
};

struct StringNode {
  const char* text;
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

  union {
    ParentNode Block;
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
