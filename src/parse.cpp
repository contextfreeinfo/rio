#include "main.h"

namespace rio {

struct ParseState {

  ModManager* mod;
  // TODO Share common space for all bufs at different stages to avoid allocs?
  List<Node*> node_buf;
  const Token* tokens;

  Node& alloc(Node::Kind kind) {
    Node& node = mod->arena.alloc<Node>();
    node.kind = kind;
    return node;
  }

};

void advance_token(ParseState* state, bool skip_lines = false);
auto node_slice_copy(ParseState* state, usize buf_len_old) -> Slice<Node*>;
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
auto parse_assign(ParseState* state) -> Node&;
auto parse_atom(ParseState* state) -> Node&;
auto parse_block(
  ParseState* state, Token::Kind end = Token::Kind::CurlyR
) -> Node&;
auto parse_call(ParseState* state) -> Node&;
auto parse_cast(ParseState* state) -> Node&;
auto parse_def(ParseState* state) -> Node&;
auto parse_expr(ParseState* state) -> Node&;
auto parse_fun(ParseState* state) -> Node&;
auto parse_tuple(ParseState* state) -> Node&;
auto parse_use(ParseState* state) -> Node&;
void skip_comments(ParseState* state, bool skip_lines = false);

void advance_token(ParseState* state, bool skip_lines) {
  auto& tokens = state->tokens;
  if (tokens->kind != Token::Kind::FileEnd) {
    tokens += 1;
    skip_comments(state, skip_lines);
  }
}

auto is_token_proc(const Token& token) -> bool {
  return
    token.kind == Token::Kind::Fun ||
    token.kind == Token::Kind::Proc ||
    token.kind == Token::Kind::Pub;
}

auto more_tokens(ParseState* state, Token::Kind end) -> bool {
  return
    state->tokens->kind != end &&
    state->tokens->kind != Token::Kind::FileEnd;
}

// TODO Unify with def_slice_copy?
auto node_slice_copy(ParseState* state, usize buf_len_old) -> Slice<Node*> {
  // Prep space.
  auto& buf = state->node_buf;
  usize len = buf.len - buf_len_old;
  buf.len = buf_len_old;
  usize nbytes = len * sizeof(*buf.items);
  void* items = state->mod->arena.alloc_bytes(nbytes);
  // Copy it in.
  memcpy(items, buf.items + buf_len_old, nbytes);
  return {static_cast<Node**>(items), len};
}

auto parse(ModManager* mod, const Token* tokens) -> Node& {
  ParseState state = [&]() {
    ParseState state;
    state.mod = mod;
    state.tokens = tokens;
    return state;
  }();
  skip_comments(&state, true);
  return parse_block(&state, Token::Kind::FileEnd);
}

auto parse_assign(ParseState* state) -> Node& {
  // TODO Assign to a call declares a macro???
  Node& a = parse_def(state);
  if (state->tokens->kind == Token::Kind::Assign) {
    advance_token(state, true);
    Node& node = state->alloc(Node::Kind::Const);
    node.Const.a = &a;
    if (verbose) printf("begin assign\n");
    node.Const.b = &parse_expr(state);
    if (verbose) printf("end assign\n");
    return node;
  } else {
    return a;
  }
}

auto parse_atom(ParseState* state) -> Node& {
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::CurlyL: {
      // TODO These should be only for data now? Unless do {...}?
      advance_token(state, true);
      return parse_block(state);
    }
    case Token::Kind::Do: {
      advance_token(state, true);
      return parse_block(state, Token::Kind::End);
    }
    case Token::Kind::Float: {
      Node& node = state->alloc(Node::Kind::Float);
      node.Float.text = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::Fun:
    case Token::Kind::Proc: {
      return parse_fun(state);
    }
    case Token::Kind::Id: {
      if (verbose) printf("ref: %s\n", tokens->text);
      Node& node = state->alloc(Node::Kind::Ref);
      node.Ref.name = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::Int: {
      Node& node = state->alloc(Node::Kind::Int);
      node.Int.text = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::String: {
      if (verbose) printf("string: %s\n", token_text(*tokens));
      Node& node = state->alloc(Node::Kind::String);
      node.String.text = tokens->text;
      advance_token(state);
      return node;
    }
    default: {
      if (verbose) printf("junk: %s\n", token_name(*tokens));
      advance_token(state);
      return state->alloc(Node::Kind::None);
    }
  }
}

auto parse_block(ParseState* state, Token::Kind end) -> Node& {
  Node& node = state->alloc(Node::Kind::Block);
  if (verbose) printf("begin block\n");
  // Parse kids.
  auto buf_len_old = state->node_buf.len;
  for (; more_tokens(state, end); skip_comments(state, true)) {
    Node* kid = &parse_expr(state);
    state->node_buf.push(kid);
  }
  // TODO Put these in some destructor if we can throw above.
  node.Block.items = node_slice_copy(state, buf_len_old);
  // Move on.
  advance_token(state);
  // if (verbose) printf("item 0: %d, %d, %s\n", static_cast<int>(node.Block.items[0]->kind), static_cast<int>(node.Block.items[0]->Call.callee->kind), node.Block.items[0]->Call.callee->Ref.name);
  if (verbose) printf("end block\n");
  return node;
}

auto parse_call(ParseState* state) -> Node& {
  Node* node = &parse_atom(state);
  while (state->tokens->kind == Token::Kind::RoundL) {
    Node* call = &state->alloc(Node::Kind::Call);
    call->Call.callee = node;
    if (verbose) printf("begin call\n");
    call->Call.args = &parse_tuple(state);
    if (verbose) printf("end call\n");
    node = call;
  }
  return *node;
}

auto parse_cast(ParseState* state) -> Node& {
  Node* node = &parse_use(state);
  while (state->tokens->kind == Token::Kind::Colon) {
    Node* cast = &state->alloc(Node::Kind::Cast);
    cast->Cast.a = node;
    advance_token(state, true);
    cast->Cast.b = &parse_call(state);
    node = cast;
  }
  return *node;
}

auto parse_def(ParseState* state) -> Node& {
  Node& a = parse_cast(state);
  if (a.kind == Node::Kind::Ref && is_token_proc(*state->tokens)) {
    bool published = false;
    if (state->tokens->kind == Token::Kind::Pub) {
      published = true;
      advance_token(state);
    }
    Node& node = parse_fun(state);
    node.Fun.name = a.Ref.name;
    node.Fun.published = published;
    return node;
  } else {
    return a;
  }
}

auto parse_expr(ParseState* state) -> Node& {
  return parse_assign(state);
}

auto parse_fun(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::Fun);
  if (verbose) printf("begin fun\n");
  node.Fun.kind = state->tokens->kind;
  advance_token(state);
  node.Fun.name = "";
  if (state->tokens->kind == Token::Kind::RoundL) {
    node.Fun.params = &parse_tuple(state);
    if (verbose) printf("args\n");
  }
  if (state->tokens->kind == Token::Kind::LineEnd) {
    skip_comments(state, true);
    node.Fun.expr = &parse_block(state, Token::Kind::End);
  } else {
    node.Fun.expr = &parse_expr(state);
  }
  if (verbose) printf("end fun\n");
  return node;
}

auto parse_tuple(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::Tuple);
  // TODO Parameterize end token since usually square maybe?
  if (verbose) printf("begin tuple\n");
  advance_token(state);
  auto buf_len_old = state->node_buf.len;
  bool past_first = false;
  for (; more_tokens(state, Token::Kind::RoundR); skip_comments(state, true)) {
    if (past_first) {
      // TODO Expect, not just accept!
      if (state->tokens->kind == Token::Kind::Comma) {
        advance_token(state, true);
      }
    }
    auto kid = &parse_expr(state);
    state->node_buf.push(kid);
    past_first = true;
  }
  if (state->tokens->kind == Token::Kind::Comma) {
    advance_token(state, true);
  }
  node.Tuple.items = node_slice_copy(state, buf_len_old);
  advance_token(state);
  if (verbose) printf("end tuple\n");
  return node;
}

auto parse_use(ParseState* state) -> Node& {
  switch (state->tokens->kind) {
    case Token::Kind::Include:
    case Token::Kind::Use: {
      Node& node = state->alloc(Node::Kind::Use);
      node.Use.kind = state->tokens->kind;
      advance_token(state, true);
      Node& arg = parse_call(state);
      if (arg.kind == Node::Kind::String) {
        node.Use.name = arg.String.text;
      } else {
        node.Use.name = "";
      }
      node.Use.arg = &arg;
      return node;
    }
    default: {
      return parse_call(state);
    }
  }
}

void skip_comments(ParseState* state, bool skip_lines) {
  // TODO Store them somewhere automatically?
  auto& tokens = state->tokens;
  if (skip_lines) {
    for (;
      tokens->kind == Token::Kind::Comment ||
        tokens->kind == Token::Kind::LineEnd;
      tokens += 1
    ) {}
  } else {
    for (; tokens->kind == Token::Kind::Comment; tokens += 1) {}
  }
}

}
