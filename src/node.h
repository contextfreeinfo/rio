#pragma once

#include "common.h"

namespace rio {

struct Engine;
struct ModManager;
struct Type;

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;
auto name_type(Engine* engine, StrBuf* buf, const Type& type) -> string;

struct Pos {
  rint index;
  rint line;
  rint col;
};

// Def.

struct Node;

struct Def {
  string name;
  Opt<Node> node;
  Opt<Node> top;
  // Custom instantiations without a node?
  Opt<Type> type;
  // Make resolution easier to order.
  bool resolve_started;
  bool resolved;
  // And cache for faster generation.
  // Should refer only to cases in the current mod, since mods are acyclic.
  Slice<Def*> uses;
  // Only matters for global struct types?
  rint depth;
  // Only for globals. If present, references the mod root.
  Opt<ModManager> mod;
};

// Token.

struct Token {
  enum struct Kind {
    // TODO Use cache files to remember ids of enums, so source order doesn't
    // TODO matter.
    None,
    And,
    Assign,
    Case,
    Class,
    Colon,
    Comma,
    Comment,
    CurlyL,
    CurlyR,
    Div,
    Do,
    Dot,
    Else,
    End,
    Equal,
    FileEnd,
    Float,
    For,
    Fun,
    Id,
    If,
    In,
    Include,
    Int,
    Junk,
    Less,
    LessOrEqual,
    LineEnd,
    Minus,
    More,
    MoreOrEqual,
    Mul,
    NotEqual,
    Plus,
    Proc,
    Pub,
    Role,
    RoundL,
    RoundR,
    SizeOf,
    SquareL,
    SquareR,
    String,
    Struct,
    Switch,
    Through,
    To,
    Unsafe,
    Update,
    Use,
    Var,
  };
  Kind kind;
  // TODO We don't need file name on each individual token.
  string file;
  Pos begin;
  rint len;
  union {
    opt_string text;
  };
};

auto is_word(Token::Kind kind) -> bool;

// Type.

// TODO Replace types with defs like anything else.
struct Type {

  enum struct Kind {
    // Voidish types.
    None,
    Ignored,
    Never,
    Void,
    // TODO C Types.
    // Float.
    F32,
    F64,
    Float,
    // Int.
    Bool,
    I8,
    I16,
    I32,
    I64,
    Int,
    // Unsigned.
    U8,
    U16,
    U32,
    U64,
    UInt,
    // Pointers.
    Address,
    AddressMul,
    //~ AddressNullTerm,
    // Aggregates.
    Array,
    Proc,
    Range,
    // String has special treatment because it needs interop.
    // Known semantics, auto conversion out, custom by back end?, ...
    String,
    Tuple,
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

auto is_voidish(Type::Kind kind) -> bool;

// Node.

struct Scope {

  Slice<Def*> defs;

  auto find(string name) const -> Opt<Def> {
    for (auto def: defs) {
      // All should be interned.
      if (name == def->name) {
        return def;
      }
    }
    return nullptr;
  }

  auto forget() -> void {
    defs.forget();
  }

};

struct BlockNode {
  Scope scope;
  Opt<Node> params;
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

struct CaseNode {
  Node* arg;
  Node* expr;
};

struct ForNode {
  Scope scope;
  Opt<Node> param;
  Node* arg;
  Node* expr;
};

// TODO Rename to ProcNode since proc more general than fun?
struct FunNode {
  string name;
  bool published;
  // Scope and params both useless for structs.
  Scope scope;
  Opt<Node> params;
  Opt<Node> ret_type;
  Node* expr;
};

struct ParentNode {
  // TODO Scope for tuples, if we want to track named field defs?
  Slice<Node*> items;
};

struct RangeNode {
  Node* from;
  Node* to;
  Node* by;
  bool inclusive;
};

struct RefNode {
  string name;
  Opt<Def> def;
};

struct StringNode {
  string text;
};

struct SwitchNode {
  Opt<Node> arg;
  Slice<Node*> items;
};

struct UnaryNode {
  Node* expr;
};

struct UseNode {
  string name;
  Token::Kind kind;
  Node* arg;
};

struct VoidNode {};

struct Node {

  enum struct Kind {
    None,
    Address,
    AddressMul,
    Array,
    Block,
    Call,
    Case,
    Cast,
    Const,
    Div,
    Else,
    Equal,
    Float,
    For,
    Fun,
    Int,
    Less,
    LessOrEqual,
    Map,
    Member,
    Minus,
    More,
    MoreOrEqual,
    Mul,
    NotEqual,
    Plus,
    Proc,
    Range,
    // A var/name ref, not a memory address reference.
    Ref,
    Return,
    SizeOf,
    String,
    Struct,
    Switch,
    Tuple,
    Unsafe,
    Update,
    Use,
    Var,
    Void,
  };

  Kind kind;
  // TODO Maybe make type a pointer so we can reduce memory usage.
  Type type;

  union {
    UnaryNode Address;
    UnaryNode AddressMul;
    ParentNode Array;
    BinaryNode Binary;
    BlockNode Block;
    BinaryNode Cast;
    BinaryNode Const;
    CallNode Call;
    CaseNode Case;
    UnaryNode Else;
    StringNode Float;
    ForNode For;
    FunNode Fun;
    StringNode Int;
    ParentNode Map;
    BinaryNode Member;
    ParentNode Parent;
    RangeNode Range;
    RefNode Ref;
    UnaryNode Return;
    UnaryNode SizeOf;
    StringNode String;
    SwitchNode Switch;
    ParentNode Tuple;
    UnaryNode Unary;
    UnaryNode Unsafe;
    BinaryNode Update;
    UnaryNode Var;
    UseNode Use;
    VoidNode Void;
  };

};

}
