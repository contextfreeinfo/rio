#pragma once

#include "common.h"

namespace rio {

enum struct Key {
  None,
  Fun,
};

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
    Id,
    Junk,
    Key,
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
    Key key;
    const char* text;
  };
};

struct Node;

struct BlockNode {
  Slice<Node> items;
};

struct FunNode {
  const char* name;
  Node* expr;
};

struct Node {
  enum struct Kind {
    Block,
    Call,
    Fun,
    Ref,
    String,
  };
  union {
    FunNode fun;
  };
};

}
