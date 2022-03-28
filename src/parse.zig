const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeKind = enum {
    assign,
    assign_to,
    block,
    call,
    colon,
    comment,
    def,
    dot,
    fun,
    frac,
    leaf,
    question,
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

    pub fn print(self: Self, writer: anytype, context: TreePrintContext) std.os.WriteError!void {
        _ = self;
        var i = @as(@TypeOf(context.indent), 0);
        while (i < context.indent) : (i += 1) {
            try writer.print(" ", .{});
        }
        try writer.print("{}", .{self.kind});
        switch (self.kind) {
            .leaf => try writer.print(": {} {s}\n", .{ self.data.token.kind, context.config.pool.get(self.data.token.text) }),
            else => {
                try writer.print("\n", .{});
                var nested = context;
                nested.indent += 2;
                for (self.data.kids.from(context.tree.nodes)) |node| {
                    _ = try node.print(writer, nested);
                }
            },
        }
    }
};

pub const TreePrintConfig = struct {
    pool: intern.Pool,
};

const TreePrintContext = struct {
    tree: Tree,
    config: TreePrintConfig,
    indent: u16,
};

pub const Tree = struct {
    nodes: []Node,

    const Self = @This();

    pub fn deinit(self: Self, allocator: Allocator) void {
        allocator.free(self.nodes);
    }

    pub fn root(self: Self) ?Node {
        return if (self.nodes.len > 0) self.nodes[self.nodes.len - 1] else null;
    }

    pub fn print(self: Self, writer: anytype, config: TreePrintConfig) !void {
        if (self.root()) |r| {
            try r.print(writer, .{ .tree = self, .config = config, .indent = 0 });
        } else {
            try writer.print("()\n", .{});
        }
    }
};

pub const ParseError = error{OutOfMemory} || std.os.ReadError;

pub fn Parser(comptime Reader: type) type {
    return struct {
        lexer: lex.Lexer(Reader),
        nodes: std.ArrayList(Node),
        unread: ?lex.Token,
        working: std.ArrayList(Node),

        const Self = @This();

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
            self.working.clearRetainingCapacity();
            try self.block();
            // This one doesn't ever appear but should push down the block if any.
            try self.nest(.block, NodeId.of(0));
            // Retain the buffer on the idea we might need that much again.
            return Tree{ .nodes = (try self.nodes.clone()).items };
        }

        fn advance(self: *Self) !?lex.Token {
            const token = (try self.readToken()) orelse return null;
            try self.working.append(.{ .kind = .leaf, .data = .{ .token = token } });
            return try self.peek();
        }

        fn assign(self: *Self) !void {
            try self.infix(.assign, .op_eq, call);
        }

        fn assign_to(self: *Self) !void {
            try self.infix(.assign_to, .op_eqto, assign);
        }

        fn atom(self: *Self) !void {
            switch (((try self.peek()) orelse return).kind) {
                .string_begin_single, .string_begin_double => try self.string(),
                else => _ = try self.advance(),
            }
        }

        fn dot(self: *Self) !void {
            try self.infix(.dot, .op_dot, atom);
        }

        fn block(self: *Self) !void {
            const begin = self.here();
            while (true) {
                try self.space();
                _ = (try self.peek()) orelse break;
                try self.assign_to();
                _ = (try self.peek()) orelse break;
            }
            try self.nestMaybe(.block, begin);
        }

        fn call(self: *Self) !void {
            const begin = self.here();
            var count = @as(u32, 0);
            while (true) : (count += 1) {
                try self.colon(count);
                // TODO Is EndOfStream easier here in parsing?
                try self.hspace();
                switch (((try self.peek()) orelse break).kind) {
                    .op_eq, .op_eqto, .vspace => break,
                    else => {},
                }
            }
            if (count > 0) {
                try self.nest(.call, begin);
            }
        }

        fn colon(self: *Self, count: u32) ParseError!void {
            const begin = self.here();
            try self.question();
            try self.infixFinish(.colon, .op_colon, if (count == 0) call else question, begin);
        }

        fn here(self: Self) NodeId {
            return NodeId.of(self.working.items.len);
        }

        fn hspace(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch (((try self.peek()) orelse break).kind) {
                    .comment, .hspace => _ = try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        // TODO This nests left, but normal assign (if nesting is allowed) should nest right. (And assign_to should nest left.)
        fn infix(self: *Self, kind: NodeKind, op: lex.TokenKind, kid: fn(self: *Self) ParseError!void) !void {
            const begin = self.here();
            try kid(self);
            try self.infixFinish(kind, op, kid, begin);
        }

        // TODO This nests left, but normal assign (if nesting is allowed) should nest right.
        fn infixFinish(self: *Self, kind: NodeKind, op: lex.TokenKind, kid: fn(self: *Self) ParseError!void, begin: NodeId) !void {
            try self.hspace();
            while (true) {
                if (((try self.peek()) orelse return).kind != op) return;
                _ = (try self.advance()).?;
                try self.space();
                if ((try self.peek()) != null) { // or any end TODO Use eof token???
                    try kid(self);
                }
                try self.nest(kind, begin);
            }
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

        fn peek(self: *Self) !?lex.Token {
            if (self.unread == null) {
                self.unread = try self.lexer.next();
            }
            return self.unread;
        }

        fn question(self: *Self) !void { // suffix -- any others?
            const begin = self.here();
            try self.dot();
            try self.hspace();
            while (true) {
                if (((try self.peek()) orelse return).kind != .op_question) return;
                _ = (try self.advance()).?;
                // The goal here is to keep questions outside of dots.
                try self.nest(.question, begin);
                // But to let dots proceed still.
                try self.infixFinish(.dot, .op_dot, atom, begin);
            }
        }

        fn readToken(self: *Self) !?lex.Token {
            if (self.unread) |unread| {
                self.unread = null;
                return unread;
            }
            return try self.lexer.next();
        }

        fn space(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch (((try self.peek()) orelse break).kind) {
                    .comment, .hspace, .vspace => _ = try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        fn string(self: *Self) !void {
            const begin = self.here();
            _ = (try self.advance()).?;
            while (true) {
                switch (((try self.peek()) orelse break).kind) {
                    .string_end => break,
                    // TODO Escapes.
                    else => _ = try self.advance(),
                }
            }
            _ = try self.advance();
            try self.nest(.string, begin);
        }
    };
}
