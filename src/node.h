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
    Comment,
    CurlyL,
    CurlyR,
    FileEnd,
    Fun,
    Id,
    Junk,
    LineEnd,
    RoundL,
    RoundR,
    String,
  };
  Kind kind;
  const char* file;
  Pos begin;
  usize len;
  union {
    const char* text;
  };
};

struct Node;

struct BlockNode {
  Slice<Node*> items;
};

struct CallNode {
  Node* callee;
};

struct FunNode {
  const char* name;
  Node* expr;
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
    Call,
    Fun,
    Ref,
    String,
  };

  Kind kind;

  union {
    BlockNode Block;
    CallNode Call;
    FunNode Fun;
    RefNode Ref;
    StringNode String;
  };

};

}
