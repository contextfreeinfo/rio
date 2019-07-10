#include "main.h"

namespace rio {

struct ParseState {

  Engine* engine;
  List<Node*> node_buf;
  const Token* tokens;

  Node& alloc(Node::Kind kind) {
    Node& node = engine->arena.alloc<Node>();
    node.kind = kind;
    return node;
  }

};

void advance_token(ParseState* state, bool skip_lines = false);
auto node_slice_copy(ParseState* state, usize buf_len_old) -> Slice<Node*>;
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
auto parse_atom(ParseState* state) -> Node&;
auto parse_block(
  ParseState* state, Token::Kind end = Token::Kind::CurlyR
) -> Node&;
auto parse_call(ParseState* state) -> Node&;
auto parse_expr(ParseState* state) -> Node&;
auto parse_fun(ParseState* state) -> Node&;
auto parse_tuple(ParseState* state) -> Node&;
void skip_comments(ParseState* state, bool skip_lines = false);

void advance_token(ParseState* state, bool skip_lines) {
  auto& tokens = state->tokens;
  if (tokens->kind != Token::Kind::FileEnd) {
    tokens += 1;
    skip_comments(state, skip_lines);
  }
}

auto node_slice_copy(ParseState* state, usize buf_len_old) -> Slice<Node*> {
  // Prep space.
  auto& buf = state->node_buf;
  usize len = buf.len - buf_len_old;
  buf.len = buf_len_old;
  usize nbytes = len * sizeof(Node*);
  void* items = state->engine->arena.alloc_bytes(nbytes);
  // Copy it in.
  memcpy(items, buf.items + buf_len_old, nbytes);
  return {static_cast<Node**>(items), len};
}

auto more_tokens(ParseState* state, Token::Kind end) -> bool {
  return
    state->tokens->kind != end &&
    state->tokens->kind != Token::Kind::FileEnd;
}

auto parse(Engine* engine, const Token* tokens) -> Node& {
  ParseState state = [&]() {
    ParseState state;
    state.engine = engine;
    state.tokens = tokens;
    return state;
  }();
  skip_comments(&state, true);
  return parse_block(&state, Token::Kind::FileEnd);
}

auto parse_atom(ParseState* state) -> Node& {
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::CurlyL: {
      return parse_block(state);
    }
    case Token::Kind::Do: {
      return parse_block(state, Token::Kind::End);
    }
    case Token::Kind::Id: {
      if (verbose) printf("ref: %s\n", tokens->text);
      Node& node = state->alloc(Node::Kind::Ref);
      node.Ref.name = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::Fun: {
      return parse_fun(state);
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
  if (end != Token::Kind::FileEnd) {
    advance_token(state, true);
  }
  // Parse kids.
  auto buf_len_old = state->node_buf.len;
  for (; more_tokens(state, end); skip_comments(state, true)) {
    Node* kid = &parse_expr(state);
    state->node_buf.push(kid);
  }
  node.Block.items = node_slice_copy(state, buf_len_old);
  // Move on.
  advance_token(state);
  // if (verbose) printf("item 0: %d, %d, %s\n", static_cast<int>(node.Block.items[0]->kind), static_cast<int>(node.Block.items[0]->Call.callee->kind), node.Block.items[0]->Call.callee->Ref.name);
  if (verbose) printf("end block\n");
  return node;
}

auto parse_call(ParseState* state) -> Node& {
  Node& callee = parse_atom(state);
  if (state->tokens->kind == Token::Kind::RoundL) {
    Node& node = state->alloc(Node::Kind::Call);
    node.Call.callee = &callee;
    if (verbose) printf("begin call\n");
    node.Call.args = &parse_tuple(state);
    if (verbose) printf("end call\n");
    return node;
  } else {
    return callee;
  }
}

auto parse_expr(ParseState* state) -> Node& {
  return parse_call(state);
}

auto parse_fun(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::Fun);
  if (verbose) printf("begin fun\n");
  advance_token(state);
  if (state->tokens->kind == Token::Kind::Id) {
    node.Fun.name = state->tokens->text;
    if (verbose) printf("name %s\n", node.Fun.name);
    advance_token(state);
  } else {
    node.Fun.name = "";
  }
  if (state->tokens->kind == Token::Kind::RoundL) {
    parse_tuple(state);
    if (verbose) printf("args\n");
  }
  node.Fun.expr = &parse_expr(state);
  if (verbose) printf("end fun\n");
  return node;
}

auto parse_tuple(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::Tuple);
  // TODO Parameterize end token since usually square maybe?
  if (verbose) printf("begin tuple\n");
  advance_token(state);
  auto buf_len_old = state->node_buf.len;
  for (; more_tokens(state, Token::Kind::RoundR); skip_comments(state, true)) {
    auto kid = &parse_expr(state);
    state->node_buf.push(kid);
    // TODO Accept comma, expect except after last.
  }
  node.Tuple.items = node_slice_copy(state, buf_len_old);
  advance_token(state);
  if (verbose) printf("end tuple\n");
  return node;
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
