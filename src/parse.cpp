#include "main.h"

namespace rio {

struct ParseState {

  Engine* engine;
  Array<Node*> node_buf;
  const Token* tokens;

  Node& alloc(Node::Kind kind) {
    Node& node = engine->arena.alloc<Node>();
    node.kind = kind;
    return node;
  }

};

void advance_token(ParseState* state, bool skip_lines = false);
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
auto parse_atom(ParseState* state) -> Node&;
auto parse_block(ParseState* state) -> Node&;
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

auto more_tokens(ParseState* state, Token::Kind end) -> bool {
  return
    state->tokens->kind != end &&
    state->tokens->kind != Token::Kind::FileEnd;
}

void parse(Engine* engine, const Token* tokens) {
  ParseState state = [&]() {
    ParseState state;
    state.engine = engine;
    state.tokens = tokens;
    return state;
  }();
  skip_comments(&state, true);
  parse_expr(&state);
}

auto parse_atom(ParseState* state) -> Node& {
  // TODO Call parse_call, and have this in parse_atom
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::CurlyL: {
      return parse_block(state);
    }
    case Token::Kind::Id: {
      printf("ref: %s\n", tokens->text);
      Node& node = state->alloc(Node::Kind::Ref);
      node.Ref.name = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::Key: {
      switch (tokens->key) {
        case Key::Fun: {
          return parse_fun(state);
        }
        default: return state->alloc(Node::Kind::None);
      } // switch
    }
    case Token::Kind::String: {
      printf("string: %s\n", token_text(*tokens));
      Node& node = state->alloc(Node::Kind::String);
      node.String.text = tokens->text;
      advance_token(state);
      return node;
    }
    default: {
      printf("junk: %s\n", token_name(*tokens));
      advance_token(state);
      return state->alloc(Node::Kind::None);
    }
  }
}

auto parse_block(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::Block);
  printf("begin block\n");
  advance_token(state, true);
  auto& buf = state->node_buf;
  auto old_size = buf.size();
  for (; more_tokens(state, Token::Kind::CurlyR); skip_comments(state, true)) {
    Node* kid = &parse_expr(state);
    buf.push_back(kid);
  }
  usize new_size = buf.size();
  usize len = new_size - old_size;
  usize nbytes = len * sizeof(Node*);
  void* items = state->engine->arena.alloc(nbytes);
  std::memcpy(items, buf.data() + old_size, nbytes);
  node.Block.items = {static_cast<Node**>(items), len};
  buf.resize(old_size);
  advance_token(state);
  printf("item 0: %d, %d, %s\n", static_cast<int>(node.Block.items[0]->kind), static_cast<int>(node.Block.items[0]->Call.callee->kind), node.Block.items[0]->Call.callee->Ref.name);
  printf("end block\n");
  return node;
}

auto parse_call(ParseState* state) -> Node& {
  Node& callee = parse_atom(state);
  if (state->tokens->kind == Token::Kind::RoundL) {
    Node& node = state->alloc(Node::Kind::Call);
    node.Call.callee = &callee;
    printf("begin call\n");
    parse_tuple(state);
    printf("end call\n");
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
  printf("begin fun\n");
  advance_token(state);
  if (state->tokens->kind == Token::Kind::Id) {
    node.Fun.name = state->tokens->text;
    printf("name %s\n", node.Fun.name);
    advance_token(state);
  } else {
    node.Fun.name = "";
  }
  if (state->tokens->kind == Token::Kind::RoundL) {
    parse_tuple(state);
    printf("args\n");
  }
  parse_expr(state);
  printf("end fun\n");
  return node;
}

auto parse_tuple(ParseState* state) -> Node& {
  // TODO Parameterize end token since usually square maybe?
  printf("begin tuple\n");
  advance_token(state);
  for (; more_tokens(state, Token::Kind::RoundR); skip_comments(state, true)) {
    parse_expr(state);
    // TODO Accept comma, expect except after last.
  }
  advance_token(state);
  printf("end tuple\n");
  return state->alloc(Node::Kind::None);
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
