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
    node.file = tokens->file;
    node.pos = tokens->begin;
    return node;
  }

};

auto advance_end(ParseState* state) -> void;
auto advance_token(ParseState* state, bool skip_lines = false) -> void;
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
auto node_slice_copy(ParseState* state, rint buf_len_old) -> Slice<Node*>;
auto parse_add(ParseState* state) -> Node&;
auto parse_assign(ParseState* state) -> Node&;
auto parse_atom(ParseState* state) -> Node&;
auto parse_block(
  ParseState* state, Token::Kind end = Token::Kind::CurlyR
) -> Node&;
auto parse_block_or_expr(ParseState* state) -> Node&;
auto parse_call(ParseState* state) -> Node&;
auto parse_case(ParseState* state, Node::Kind kind = Node::Kind::Case) -> Node&;
auto parse_cast(ParseState* state) -> Node&;
auto parse_compare(ParseState* state) -> Node&;
auto parse_def(ParseState* state) -> Node&;
auto parse_else(ParseState* state) -> Node&;
auto parse_expr(ParseState* state) -> Node&;
auto parse_for(ParseState* state) -> Node&;
auto parse_fun(ParseState* state) -> Node&;
auto parse_if(ParseState* state) -> Node&;
auto parse_mul(ParseState* state) -> Node&;
auto parse_range(ParseState* state) -> Node&;
auto parse_tuple(ParseState* state) -> Node&;
auto parse_update(ParseState* state) -> Node&;
auto parse_use(ParseState* state) -> Node&;
auto skip_comments(ParseState* state, bool skip_lines = false) -> void;

auto advance_end(ParseState* state) -> void {
  advance_token(state);
  // See if we have any end tag.
  if (is_word(state->tokens->kind)) {
    // TODO Store this for later checking.
    advance_token(state);
  }
}

auto advance_token(ParseState* state, bool skip_lines) -> void {
  auto& tokens = state->tokens;
  if (tokens->kind != Token::Kind::FileEnd) {
    tokens += 1;
    skip_comments(state, skip_lines);
  }
}

auto is_token_add(const Token& token) -> Node::Kind {
  switch (token.kind) {
    case Token::Kind::Minus: return Node::Kind::Minus;
    case Token::Kind::Plus: return Node::Kind::Plus;
    default: return Node::Kind::None;
  }
}

auto is_token_mul(const Token& token) -> Node::Kind {
  switch (token.kind) {
    case Token::Kind::Div: return Node::Kind::Div;
    case Token::Kind::Mul: return Node::Kind::Mul;
    default: return Node::Kind::None;
  }
}

auto is_token_compare(const Token& token) -> Node::Kind {
  switch (token.kind) {
    case Token::Kind::Equal: return Node::Kind::Equal;
    case Token::Kind::Less: return Node::Kind::Less;
    case Token::Kind::LessOrEqual: return Node::Kind::LessOrEqual;
    case Token::Kind::More: return Node::Kind::More;
    case Token::Kind::MoreOrEqual: return Node::Kind::MoreOrEqual;
    case Token::Kind::NotEqual:  return Node::Kind::NotEqual;
    default: return Node::Kind::None;
  }
}

auto is_token_proc(const Token& token) -> bool {
  return
    token.kind == Token::Kind::Class ||
    token.kind == Token::Kind::Fun ||
    token.kind == Token::Kind::Proc ||
    token.kind == Token::Kind::Pub ||
    token.kind == Token::Kind::Struct;
}

auto more_tokens(ParseState* state, Token::Kind end) -> bool {
  return
    state->tokens->kind != end &&
    state->tokens->kind != Token::Kind::FileEnd;
}

// TODO Unify with def_slice_copy?
auto node_slice_copy(ParseState* state, rint buf_len_old) -> Slice<Node*> {
  // Prep space.
  auto& buf = state->node_buf;
  rint len = buf.len - buf_len_old;
  buf.len = buf_len_old;
  rint nbytes = len * sizeof(*buf.items);
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

auto parse_add(ParseState* state) -> Node& {
  Node* node = &parse_mul(state);
  while (true) {
    auto node_kind = is_token_add(*state->tokens);
    if (node_kind == Node::Kind::None) {
      break;
    }
    advance_token(state, true);
    Node* pair = &state->alloc(node_kind);
    pair->Binary.a = node;
    pair->Binary.b = &parse_mul(state);
    node = pair;
  }
  return *node;
}

auto parse_assign(ParseState* state) -> Node& {
  // TODO Assign to a call declares a macro???
  Node& a = parse_update(state);
  if (state->tokens->kind == Token::Kind::Assign) {
    advance_token(state, true);
    Node& node = state->alloc(Node::Kind::Const);
    node.Const.a = &a;
    if (verbose) fprintf(stderr, "begin assign\n");
    node.Const.b = &parse_assign(state);
    if (verbose) fprintf(stderr, "end assign\n");
    return node;
  } else {
    return a;
  }
}

auto parse_atom(ParseState* state) -> Node& {
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::And: {
      advance_token(state);
      auto multi = state->tokens->kind == Token::Kind::Mul;
      Node& node = state->alloc(
        multi ? Node::Kind::AddressMul : Node::Kind::Address
      );
      if (multi) advance_token(state);
      node.Address.expr = &parse_call(state);
      return node;
    }
    case Token::Kind::CurlyL:
    case Token::Kind::SquareL: {
      return parse_tuple(state);
    }
    case Token::Kind::Do: {
      advance_token(state);
      Node* params = nullptr;
      if (state->tokens->kind == Token::Kind::RoundL) {
        params = &parse_tuple(state);
      }
      skip_comments(state, true);
      auto& node = parse_block(state, Token::Kind::End);
      node.Block.params = params;
      return node;
    }
    case Token::Kind::Float: {
      Node& node = state->alloc(Node::Kind::Float);
      node.Float.text = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::For: {
      return parse_for(state);
    }
    case Token::Kind::Class:
    case Token::Kind::Fun:
    case Token::Kind::Proc:
    case Token::Kind::Struct: {
      return parse_fun(state);
    }
    case Token::Kind::Id: {
      if (verbose) fprintf(stderr, "ref: %s\n", tokens->text);
      Node& node = state->alloc(Node::Kind::Ref);
      node.Ref.name = tokens->text;
      advance_token(state);
      if (state->tokens->kind == Token::Kind::Var) {
        // TODO Handle published somewheres.
        advance_token(state);
        Node& var = state->alloc(Node::Kind::Var);
        var.Var.expr = &node;
        return var;
      }
      return node;
    }
    case Token::Kind::If: {
      return parse_if(state);
    }
    case Token::Kind::Int: {
      Node& node = state->alloc(Node::Kind::Int);
      node.Int.text = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::SizeOf: {
      Node& node = state->alloc(Node::Kind::SizeOf);
      advance_token(state);
      node.SizeOf.expr = &parse_call(state);
      return node;
    }
    case Token::Kind::String: {
      if (verbose) fprintf(stderr, "string: %s\n", token_text(*tokens));
      Node& node = state->alloc(Node::Kind::String);
      node.String.text = tokens->text;
      advance_token(state);
      return node;
    }
    case Token::Kind::Unsafe: {
      advance_token(state);
      Node& node = state->alloc(Node::Kind::Unsafe);
      node.Unsafe.expr = &parse_block_or_expr(state);
      return node;
    }
    default: {
      if (verbose) fprintf(stderr, "junk: %s\n", token_name(*tokens));
      advance_token(state);
      return state->alloc(Node::Kind::None);
    }
  }
}

auto parse_block(ParseState* state, Token::Kind end) -> Node& {
  Node& node = state->alloc(Node::Kind::Block);
  if (verbose) fprintf(stderr, "begin block\n");
  // Parse kids.
  auto buf_len_old = state->node_buf.len;
  for (; more_tokens(state, end); skip_comments(state, true)) {
    Node* kid = &parse_expr(state);
    state->node_buf.push(kid);
  }
  // TODO Put these in some destructor if we can throw above.
  node.Block.items = node_slice_copy(state, buf_len_old);
  // Move on.
  advance_end(state);
  // if (verbose) fprintf(stderr, "item 0: %d, %d, %s\n", static_cast<int>(node.Block.items[0]->kind), static_cast<int>(node.Block.items[0]->Call.callee->kind), node.Block.items[0]->Call.callee->Ref.name);
  if (verbose) fprintf(stderr, "end block\n");
  return node;
}

auto parse_block_or_expr(ParseState* state) -> Node& {
  if (state->tokens->kind == Token::Kind::LineEnd) {
    skip_comments(state, true);
    return parse_block(state, Token::Kind::End);
  } else {
    return parse_expr(state);
  }
}

auto parse_call(ParseState* state) -> Node& {
  Node* node = &parse_atom(state);
  bool done = false;
  while (!done) {
    switch (state->tokens->kind) {
      case Token::Kind::Dot: {
        Node* member = &state->alloc(Node::Kind::Member);
        member->Member.a = node;
        advance_token(state, true);
        member->Member.b = &parse_atom(state);
        node = member;
        break;
      }
      case Token::Kind::RoundL:
      case Token::Kind::SquareL: {
        Node* call = &state->alloc(Node::Kind::Call);
        call->Call.callee = node;
        call->Call.args = &parse_tuple(state);
        node = call;
        break;
      }
      default: {
        done = true;
        break;
      }
    }
  }
  return *node;
}

auto parse_case(ParseState* state, Node::Kind kind) -> Node& {
  // This presumes any keyword has already been advanced past.
  Node& node = state->alloc(kind);
  if (state->tokens->kind == Token::Kind::Case) {
    advance_token(state);
  }
  node.Case.arg = &parse_expr(state);
  node.Case.expr = &parse_block_or_expr(state);
  return node;
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

auto parse_compare(ParseState* state) -> Node& {
  // TODO Should compare be outside or inside range?
  // TODO Compare ranges or range on bools??? Neither seems likely.
  Node* node = &parse_range(state);
  while (true) {
    auto node_kind = is_token_compare(*state->tokens);
    if (node_kind == Node::Kind::None) {
      break;
    }
    advance_token(state, true);
    Node* pair = &state->alloc(node_kind);
    pair->Binary.a = node;
    pair->Binary.b = &parse_range(state);
    node = pair;
  }
  return *node;
}

auto parse_def(ParseState* state) -> Node& {
  Node& a = parse_cast(state);
  if (a.kind == Node::Kind::Ref && is_token_proc(*state->tokens)) {
    // TODO Should pub go before proc/fun/struct/...?
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
    // Nothing got nested, so go with what we already had.
    return a;
  }
}

auto parse_else(ParseState* state) -> Node& {
  advance_token(state);
  Node& node = state->alloc(Node::Kind::Else);
  node.Else.expr = &parse_block_or_expr(state);
  return node;
}

auto parse_expr(ParseState* state) -> Node& {
  return parse_assign(state);
}

auto parse_for(ParseState* state) -> Node& {
  Node& node = state->alloc(Node::Kind::For);
  // TODO Support `for item at index in items ...` ?
  advance_token(state);
  auto sub = &parse_expr(state);
  if (state->tokens->kind == Token::Kind::In) {
    // for item in items
    node.For.param = sub;
    advance_token(state, true);
    // TODO for item at index in items
    node.For.arg = &parse_expr(state);
  } else {
    // for items
    node.For.arg = sub;
  }
  node.For.expr = &parse_block_or_expr(state);
  return node;
}

auto parse_fun(ParseState* state) -> Node& {
  if (verbose) fprintf(stderr, "begin fun\n");
  Node::Kind kind;
  switch (state->tokens->kind) {
    case Token::Kind::Fun: {
      kind = Node::Kind::Fun;
      break;
    }
    case Token::Kind::Class:
    case Token::Kind::Struct: {
      kind = Node::Kind::Struct;
      break;
    }
    default: {
      kind = Node::Kind::Proc;
      break;
    }
  }
  Node& node = state->alloc(kind);
  advance_token(state);
  node.Fun.name = "";
  if (state->tokens->kind == Token::Kind::RoundL) {
    node.Fun.params = &parse_tuple(state);
    if (verbose) fprintf(stderr, "args\n");
  }
  if (state->tokens->kind == Token::Kind::Colon) {
    advance_token(state, true);
    node.Fun.ret_type = &parse_call(state);
  } else {
    node.Fun.ret_type = &state->alloc(Node::Kind::Void);
  }
  node.Fun.expr = &parse_block_or_expr(state);
  if (verbose) fprintf(stderr, "end fun\n");
  return node;
}

auto parse_if(ParseState* state) -> Node& {
  advance_token(state);
  if (state->tokens->kind == Token::Kind::LineEnd) {
    // If/else switch.
    Node& node = state->alloc(Node::Kind::Switch);
    skip_comments(state, true);
    auto buf_len_old = state->node_buf.len;
    for (; more_tokens(state, Token::Kind::End); skip_comments(state, true)) {
      //~ fprintf(stderr, "parse case\n");
      Node* kid = state->tokens->kind == Token::Kind::Else ?
        &parse_else(state) : &parse_case(state);
      state->node_buf.push(kid);
    }
    // TODO Put these in some destructor.
    node.Switch.items = node_slice_copy(state, buf_len_old);
    // Move on.
    advance_end(state);
    return node;
  } else {
    // Just a single if case.
    return parse_case(state);
  }
}

auto parse_mul(ParseState* state) -> Node& {
  Node* node = &parse_call(state);
  while (true) {
    // Combine functions for add and other binaries?
    auto node_kind = is_token_mul(*state->tokens);
    if (node_kind == Node::Kind::None) {
      break;
    }
    advance_token(state, true);
    Node* pair = &state->alloc(node_kind);
    pair->Binary.a = node;
    pair->Binary.b = &parse_call(state);
    node = pair;
  }
  return *node;
}

auto parse_range(ParseState* state) -> Node& {
  // TODO Also have a deeper handler for prefix `to ...`.
  Node* node = &parse_add(state);
  switch (state->tokens->kind) {
    case Token::Kind::Through:
    case Token::Kind::To: {
      auto inclusive = state->tokens->kind == Token::Kind::Through;
      // No going cross lines here, since we can have dangling `to` sometimes.
      advance_token(state);
      // TODO Extract here down to a separate function also for prefix `to`.
      Node* range = &state->alloc(Node::Kind::Range);
      range->Range.from = node;
      // TODO For dangling `... to`, skip this if any endish tokens are here.
      range->Range.to = &parse_add(state);
      // TODO Check for `by`!
      range->Range.inclusive = inclusive;
      node = range;
      break;
    }
    default: break;
  }
  return *node;
}

auto parse_tuple(ParseState* state) -> Node& {
  Token::Kind end;
  Node::Kind kind;
  switch (state->tokens->kind) {
    case Token::Kind::CurlyL: {
      end = Token::Kind::CurlyR;
      kind = Node::Kind::Map;
      break;
    }
    case Token::Kind::RoundL: {
      end = Token::Kind::RoundR;
      kind = Node::Kind::Tuple;
      break;
    }
    case Token::Kind::SquareL: {
      end = Token::Kind::SquareR;
      kind = Node::Kind::Array;
      break;
    }
    default: {
      // Getting here is a code bug.
      assert(false);
    }
  }
  Node& node = state->alloc(kind);
  if (verbose) fprintf(stderr, "begin tuple\n");
  advance_token(state, true);
  auto buf_len_old = state->node_buf.len;
  bool past_first = false;
  for (; more_tokens(state, end); skip_comments(state, true)) {
    if (past_first) {
      // TODO Expect, not just accept!
      if (state->tokens->kind == Token::Kind::Comma) {
        advance_token(state, true);
      }
      if (!more_tokens(state, end)) {
        // That was a trailing comma.
        break;
      }
    }
    auto kid = &parse_expr(state);
    state->node_buf.push(kid);
    past_first = true;
  }
  node.Tuple.items = node_slice_copy(state, buf_len_old);
  advance_token(state);
  if (verbose) fprintf(stderr, "end tuple\n");
  return node;
}

auto parse_update(ParseState* state) -> Node& {
  Node& a = parse_def(state);
  if (state->tokens->kind == Token::Kind::Update) {
    advance_token(state, true);
    Node& node = state->alloc(Node::Kind::Update);
    node.Const.a = &a;
    node.Const.b = &parse_update(state);
    return node;
  } else {
    return a;
  }
}

auto parse_use(ParseState* state) -> Node& {
  switch (state->tokens->kind) {
    case Token::Kind::Include:
    case Token::Kind::Use: {
      Node& node = state->alloc(Node::Kind::Use);
      node.Use.kind = state->tokens->kind;
      advance_token(state, true);
      // TODO Do binary operators go in here?
      Node& arg = parse_compare(state);
      if (arg.kind == Node::Kind::String) {
        node.Use.name = arg.String.text;
      } else {
        node.Use.name = "";
      }
      node.Use.arg = &arg;
      return node;
    }
    default: {
      return parse_compare(state);
    }
  }
}

auto skip_comments(ParseState* state, bool skip_lines) -> void {
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
