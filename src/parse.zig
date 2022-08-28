const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeKind = enum {
    add,
    as,
    assign,
    assign_to,
    be,
    block,
    bool_and,
    bool_or,
    call,
    colon,
    comment,
    compare,
    def,
    dot,
    end,
    escape,
    frac,
    fun,
    fun_for,
    fun_to,
    fun_with,
    leaf,
    list,
    mul,
    of,
    question,
    round,
    sign,
    sign_int,
    space,
    string,
};

pub const NodeId = idx.Idx(u32, Node);
pub const NodeSlice = idx.IdxSlice(NodeId);

pub const Node = struct {
    kind: NodeKind,
    data: union {
        // Both of these are pairs of u32 values.
        kids: NodeSlice,
        token: lex.Token,
    },

    const Self = @This();

    pub fn print(self: Self, writer: anytype, context: TreePrintContext) std.os.WriteError!u32 {
        var count = @as(u32, 0);
        try printIndent(writer, context.indent);
        const kind = @tagName(self.kind);
        // TODO Remove "NodeKind." in a future commit without other changes.
        try writer.print("NodeKind.{s}", .{kind});
        switch (self.kind) {
            .leaf => {
                count += 1;
                const text = lex.tokenTextSure(self.data.token, context.config.pool);
                // TODO Remove "TokenKind." in a future commit without other changes.
                try writer.print(": TokenKind.{s} {s}\n", .{ @tagName(self.data.token.kind), text });
            },
            else => {
                try writer.print("\n", .{});
                var nested = context;
                nested.indent += 2;
                for (self.data.kids.from(context.nodes)) |node| {
                    count += try node.print(writer, nested);
                }
                if (count > 1) {
                    try printIndent(writer, context.indent);
                    try writer.print("/NodeKind.{s}\n", .{kind});
                }
            },
        }
        return count;
    }

    fn printIndent(writer: anytype, indent: u16) !void {
        var i = @as(@TypeOf(indent), 0);
        while (i < indent) : (i += 1) {
            try writer.print(" ", .{});
        }
    }
};

pub fn adjustKids(nodes: []Node, new_begin: u32, offset: u32) void {
    for (nodes) |*node| {
        switch (node.kind) {
            .leaf => {},
            else => if (node.data.kids.idx.i >= new_begin + offset) {
                node.data.kids.idx.i -= offset;
            },
        }
    }
}

pub const TreePrintConfig = struct {
    pool: intern.Pool,
};

const TreePrintContext = struct {
    config: TreePrintConfig,
    indent: u16 = 0,
    nodes: []Node,
};

pub const Tree = struct {
    nodes: []Node,

    const Self = @This();

    pub fn deinit(self: Self, allocator: Allocator) void {
        allocator.free(self.nodes);
    }

    pub fn nameOf(self: Self, node: Node) ?intern.TextId {
        return switch (node.kind) {
            .dot => self.nameOf(lex.last(Node, node.data.kids.from(self.nodes)).?),
            .leaf => switch (lex.tokenKindCategory(node.data.token.kind)) {
                .id, .key => node.data.token.text,
                else => null,
            },
            .question => self.nameOf(node.data.kids.idx.from(self.nodes)),
            else => null,
        };
    }

    pub fn root(self: Self) Node {
        return lex.last(Node, self.nodes) orelse Node{ .kind = NodeKind.block, .data = .{ .kids = NodeId.of(0).slice(0) } };
    }

    pub fn print(self: Self, writer: anytype, config: TreePrintConfig) !void {
        _ = try self.root().print(writer, .{ .nodes = self.nodes, .config = config, .indent = 0 });
    }
};

pub const ParseError = error{OutOfMemory} || std.os.ReadError;

const Context = struct {
    fun: bool = false,
    line_begin: NodeId = NodeId.of(0),

    fn withFun(self: Context, fun: bool) Context {
        var result = self;
        result.fun = fun;
        return result;
    }

    fn withLineBegin(self: Context, line_begin: NodeId) Context {
        var result = self;
        result.line_begin = line_begin;
        return result;
    }
};

pub fn Parser(comptime Reader: type) type {
    return struct {
        lexer: lex.Lexer(Reader),
        nodes: std.ArrayList(Node),
        unread: ?lex.Token,
        working: std.ArrayList(Node),

        const Self = @This();
        const CheckEnd = *const fn (self: *Self, kind: lex.TokenKind) ParseError!bool;
        const ParseKid = *const fn (self: *Self, context: Context) ParseError!void;

        pub fn init(allocator: Allocator, pool: ?*intern.Pool) !Self {
            return Self{
                .lexer = try lex.Lexer(Reader).init(allocator, pool),
                .nodes = std.ArrayList(Node).init(allocator),
                .unread = null,
                .working = std.ArrayList(Node).init(allocator),
            };
        }

        pub fn deinit(self: *Self) void {
            self.lexer.deinit();
            self.nodes.deinit();
            self.working.deinit();
        }

        pub fn parse(self: *Self, reader: Reader) !Tree {
            self.lexer.start(reader);
            self.nodes.clearRetainingCapacity();
            self.unread = null;
            self.working.clearRetainingCapacity();
            var bonus = false;
            while (true) {
                try self.block();
                switch ((try self.peek()).kind) {
                    .eof => break,
                    else => {
                        // Some extra `end` involved.
                        bonus = true;
                        try self.advance();
                    },
                }
            }
            if (bonus) {
                // Gather multiple top levels into a bonus block.
                try self.nest(.block, NodeId.of(0));
            }
            // This one doesn't ever appear but should push down the block if any.
            try self.nest(.block, NodeId.of(0));
            // Retain the buffer on the idea we might need that much again.
            return Tree{ .nodes = (try self.nodes.clone()).items };
        }

        fn add(self: *Self, context: Context) !void {
            try self.infix(context, .add, opAdd, mul);
        }

        fn advance(self: *Self) !void {
            const token = try self.readToken();
            if (token.kind != .eof) {
                try self.working.append(.{ .kind = .leaf, .data = .{ .token = token } });
            }
        }

        fn argVal(self: *Self, context: Context) !void {
            try self.boolOr(context);
        }

        fn as(self: *Self, context: Context) !void {
            try self.infix(context, .as, &opAs, &call);
        }

        fn assign(self: *Self, context: Context) ParseError!void {
            // Not normal infix because right associative.
            const begin = self.here();
            try self.subline(context);
            try self.hspace();
            if ((try self.peek()).kind == .op_eq) {
                try self.advance();
                try self.space();
                try self.assign(context);
                try self.nest(.assign, begin);
            }
        }

        fn assignTo(self: *Self, context: Context) !void {
            try self.infix(context, .assign_to, &opEqto, &assign);
        }

        fn atom(self: *Self, context: Context) !void {
            switch ((try self.peek()).kind) {
                .eof, .escape_end, .key_end, .op_comma, .round_end => {},
                .key_be, .key_for, .key_to, .key_with => if (!context.fun) {
                    try self.fun(context);
                },
                .key_of => try self.blocker(context, context.withFun(false), .of, checkBlockEnd),
                .op_sub => try self.sign(context),
                .round_begin => try self.round(context),
                .quote_single, .quote_double => try self.string(context),
                else => try self.advance(),
            }
        }

        fn block(self: *Self) !void {
            try self.blockUntil(.{}, &checkBlockEnd);
        }

        // TODO? fn blockUntil(self: *Self, context: Context, comptime check_end: CheckEnd) !void {
        fn blockUntil(self: *Self, context: Context, check_end: CheckEnd) !void {
            const begin = self.here();
            while (true) {
                try self.space();
                const check = (try self.peek()).kind;
                switch (check) {
                    .eof => break,
                    else => if (try check_end(self, check)) {
                        break;
                    } else if (check == .round_end) {
                        // Just eat this unexpected paren. Other blocks are stronger.
                        try self.advance();
                        try self.space();
                    },
                }
                try self.line(context);
            }
            try self.nestMaybe(.block, begin);
        }

        // TODO? fn blocker(self: *Self, context: Context, block_context: Context, kind: NodeKind, comptime check_end: CheckEnd) !void {
        fn blocker(self: *Self, context: Context, block_context: Context, kind: NodeKind, check_end: CheckEnd) !void {
            const begin = self.here();
            try self.advance();
            try self.hspace();
            switch ((try self.peek()).kind) {
                .vspace => {
                    try self.blockUntil(block_context, check_end);
                    if ((try self.peek()).kind == .key_end) {
                        try self.end();
                    }
                },
                else => {
                    const sub = self.here();
                    try self.subline(context);
                    if (kind != .fun_for) {
                        try self.nest(.block, sub);
                    }
                },
            }
            try self.nest(kind, begin);
        }

        // TODO? fn blockerCommon(self: *Self, context: Context, kind: NodeKind, comptime check_end: CheckEnd) !void {
        fn blockerCommon(self: *Self, context: Context, kind: NodeKind, check_end: CheckEnd) !void {
            try self.blocker(context, context, kind, check_end);
        }

        fn boolOr(self: *Self, context: Context) !void {
            try self.infix(context, .bool_or, &opOr, &boolAnd);
        }

        fn boolAnd(self: *Self, context: Context) !void {
            try self.infix(context, .bool_and, &opAnd, &compare);
        }

        fn call(self: *Self, context: Context) !void {
            const begin = self.here();
            var count = @as(u32, 0);
            while (true) : (count += 1) {
                switch ((try self.peek()).kind) {
                    .eof, .escape_end, .key_as, .key_end, .op_comma, .op_eq, .op_eqto, .round_end, .vspace => break,
                    .key_be, .key_for, .key_to, .key_with => if (context.fun) {
                        break;
                    },
                    else => {},
                }
                try self.colon(context);
                try self.hspace();
            }
            if (count > 1) {
                const first = begin.from(self.working.items);
                const wrap_question = first.kind == .question;
                if (wrap_question) {
                    // Bring out the wrapped kids to working space and adjust.
                    // Overwrite the question with the question's kid.
                    self.working.items[begin.i] = first.data.kids.idx.from(self.nodes.items);
                    const kid_begin = first.data.kids.idx.i;
                    const kid_count = first.data.kids.len;
                    if (kid_count > 1) {
                        // If more than one kid, insert the rest after.
                        const kids = NodeId.of(kid_begin + 1).slice(kid_count - 1);
                        try self.working.insertSlice(begin.i + 1, kids.from(self.nodes.items));
                    }
                    // Remove these kids from the tree.
                    std.mem.copy(
                        Node,
                        self.nodes.items[kid_begin .. self.nodes.items.len - kid_count],
                        self.nodes.items[kid_begin + kid_count ..],
                    );
                    self.nodes.items.len -= kid_count;
                    // Move the pointing back further in the tree, too.
                    adjustKids(self.nodes.items[kid_begin..], kid_begin, kid_count);
                    adjustKids(self.working.items[begin.i..], kid_begin, kid_count);
                }
                try self.nest(.call, begin);
                if (wrap_question) {
                    // Wrap the call in a question.
                    try self.nest(.question, begin);
                }
            }
        }

        fn checkBlockEnd(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // TODO Get fancier?
            _ = self;
            return kind == .key_end;
        }

        fn checkEscapeEnd(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // Nothing much fancier we can do here with nested lexing.
            _ = self;
            return kind == .escape_end;
        }

        fn checkFunPartEnd(self: *Self, kind: lex.TokenKind) ParseError!bool {
            _ = self;
            return switch (kind) {
                .escape_end, .key_be, .key_end, .key_for, .key_to, .key_with, .round_end => true,
                else => false,
            };
        }

        fn checkRoundEnd(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // TODO Get fancier?
            _ = self;
            return kind == .round_end;
        }

        fn colon(self: *Self, context: Context) ParseError!void {
            const begin = self.here();
            try self.argVal(context);
            // Line won't repeat colons, since they'll be consumed internally, but eh.
            try self.infixFinish(context, .colon, opColon, if (begin.is(context.line_begin)) lineVal else question, begin);
        }

        fn compare(self: *Self, context: Context) !void {
            try self.infix(context, .compare, opCompare, add);
        }

        fn dot(self: *Self, context: Context) !void {
            try self.infix(context, .dot, opDot, atom);
            var last = lex.lastPtr(Node, self.working.items);
            if (last != null and last.?.kind == .dot) {
                self.dotConvert(last.?);
            }
        }

        fn dotConvert(self: *Self, node: *Node) void {
            var kids = node.data.kids;
            var first = &self.nodes.items[kids.idx.i];
            if (first.kind == .dot) {
                self.dotConvert(first);
                return;
            }
            // At the innermost dot.
            for (kids.from(self.nodes.items)) |kid| {
                switch (kid.kind) {
                    .leaf => {},
                    .sign_int => continue,
                    else => return,
                }
                switch (kid.data.token.kind) {
                    .int, .op_dot => {},
                    else => return,
                }
            }
            node.kind = .frac;
        }

        fn end(self: *Self) !void {
            const begin = self.here();
            try self.advance();
            try self.hspace();
            switch (lex.tokenKindCategory((try self.peek()).kind)) {
                // TODO Use for multipop???
                .id, .key => try self.advance(),
                else => {},
            }
            try self.nest(.end, begin);
        }

        fn escape(self: *Self, context: Context) !void {
            const begin = self.here();
            try self.advance();
            try self.blockUntil(context.withFun(false), checkEscapeEnd);
            if ((try self.peek()).kind == .escape_end) {
                try self.advance();
            }
            try self.nest(.escape, begin);
        }

        fn fun(self: *Self, context: Context) !void {
            const begin = self.here();
            var count = @as(u32, 0);
            while (true) : (count += 1) {
                switch ((try self.peek()).kind) {
                    .key_be => {
                        try self.blockerCommon(context.withFun(false), .be, checkBlockEnd);
                        if (count == 0) {
                            return;
                        } else {
                            break;
                        }
                    },
                    .key_for => try self.blockerCommon(context.withFun(true), .fun_for, checkFunPartEnd),
                    // We expect only one thing under `to`, but be flexible in parsing to match the rest.
                    .key_to => try self.blockerCommon(context.withFun(true), .fun_to, checkFunPartEnd),
                    .key_with => try self.blockerCommon(context.withFun(true), .fun_with, checkFunPartEnd),
                    else => break,
                }
            }
            try self.nest(.fun, begin);
        }

        fn here(self: Self) NodeId {
            return NodeId.of(self.working.items.len);
        }

        fn hspace(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch ((try self.peek()).kind) {
                    .comment, .hspace => try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        // TODO? fn infix(self: *Self, context: Context, kind: NodeKind, comptime op: CheckOp, comptime kid: ParseKid) !void {
        fn infix(self: *Self, context: Context, kind: NodeKind, op: CheckOp, kid: ParseKid) !void {
            const begin = self.here();
            try kid(self, context);
            try self.infixFinish(context, kind, op, kid, begin);
        }

        fn infixFinish(self: *Self, context: Context, kind: NodeKind, op: CheckOp, kid: ParseKid, begin: NodeId) !void {
            try self.hspace();
            while (true) {
                if (!op((try self.peek()).kind)) break;
                try self.advance();
                try self.space();
                switch ((try self.peek()).kind) { // or any end
                    .eof, .escape_end, .key_end, .round_end => {},
                    else => try kid(self, context),
                }
                try self.nest(kind, begin);
            }
        }

        fn line(self: *Self, context: Context) !void {
            try self.list(context);
        }

        fn lineVal(self: *Self, context: Context) !void {
            try self.assignTo(context.withLineBegin(self.here()));
        }

        fn list(self: *Self, context: Context) ParseError!void {
            const begin = self.here();
            try self.lineVal(context);
            try self.hspace();
            var count = @as(u32, 0);
            while (true) : (count += 1) {
                if ((try self.peek()).kind != .op_comma) break;
                try self.advance();
                try self.space();
                try self.lineVal(context);
            }
            if (count > 0) {
                try self.nest(.list, begin);
            }
        }

        fn mul(self: *Self, context: Context) !void {
            try self.infix(context, .mul, opMul, question);
        }

        fn nest(self: *Self, kind: NodeKind, begin: NodeId) !void {
            // Move the working nodes into permanent.
            const nodes_begin = NodeId.of(self.nodes.items.len);
            try self.nodes.appendSlice(begin.til(NodeId.of(self.working.items.len)).from(self.working.items));
            self.working.items.len = begin.i;
            // Make a new working node.
            const parent = Node{
                .kind = kind,
                .data = .{ .kids = nodes_begin.til(NodeId.of(self.nodes.items.len)) },
            };
            try self.working.append(parent);
        }

        fn nestMaybe(self: *Self, kind: NodeKind, begin: NodeId) !void {
            if (self.here().i > begin.i) {
                try self.nest(kind, begin);
            }
        }

        fn peek(self: *Self) !lex.Token {
            if (self.unread == null) {
                self.unread = try self.lexer.next();
            }
            return self.unread.?;
        }

        fn question(self: *Self, context: Context) !void { // suffix -- any others?
            const begin = self.here();
            try self.dot(context);
            try self.hspace();
            while (true) {
                if ((try self.peek()).kind != .op_question) break;
                try self.advance();
                // The goal here is to keep questions outside of dots.
                try self.nest(.question, begin);
                // But to let dots proceed still.
                try self.infixFinish(context.withFun(false), .dot, opDot, atom, begin);
            }
        }

        fn readToken(self: *Self) !lex.Token {
            if (self.unread) |unread| {
                self.unread = null;
                return unread;
            }
            return try self.lexer.next();
        }

        fn round(self: *Self, context: Context) !void {
            const begin = self.here();
            try self.advance();
            try self.blockUntil(context.withFun(false), checkRoundEnd);
            if ((try self.peek()).kind == .round_end) {
                try self.advance();
            }
            try self.nest(.round, begin);
        }

        fn sign(self: *Self, context: Context) ParseError!void {
            const begin = self.here();
            try self.advance();
            switch ((try self.peek()).kind) {
                .int => {
                    try self.advance();
                    try self.nest(.sign_int, begin);
                },
                else => {
                    try self.question(context);
                    try self.nest(.sign, begin);
                },
            }
        }

        fn space(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch ((try self.peek()).kind) {
                    .comment, .hspace, .vspace => try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        fn subline(self: *Self, context: Context) !void {
            try self.as(context);
        }

        fn string(self: *Self, context: Context) !void {
            const begin = self.here();
            try self.advance();
            var done = false;
            while (true) {
                switch ((try self.peek()).kind) {
                    .eof, .quote_double, .quote_single => break,
                    .escape_begin => try self.escape(context),
                    .vspace => {
                        done = true;
                        break;
                    },
                    else => try self.advance(),
                }
            }
            if (!done) {
                try self.advance();
            }
            try self.nest(.string, begin);
        }
    };
}

const CheckOp = *const fn (lex.TokenKind) bool;

fn opAdd(kind: lex.TokenKind) bool {
    return switch (kind) {
        .op_add, .op_sub => true,
        else => false,
    };
}

fn opAnd(kind: lex.TokenKind) bool {
    return kind == .key_and;
}

fn opAs(kind: lex.TokenKind) bool {
    return kind == .key_as;
}

fn opColon(kind: lex.TokenKind) bool {
    return kind == .op_colon;
}

fn opComma(kind: lex.TokenKind) bool {
    return kind == .op_comma;
}

fn opCompare(kind: lex.TokenKind) bool {
    return switch (kind) {
        .op_eqeq, .op_ge, .op_gt, .op_le, .op_lt => true,
        else => false,
    };
}

fn opDot(kind: lex.TokenKind) bool {
    return kind == .op_dot;
}

fn opEqto(kind: lex.TokenKind) bool {
    return kind == .op_eqto;
}

fn opMul(kind: lex.TokenKind) bool {
    return switch (kind) {
        .op_div, .op_mul => true,
        else => false,
    };
}

fn opOr(kind: lex.TokenKind) bool {
    return kind == .key_or;
}
