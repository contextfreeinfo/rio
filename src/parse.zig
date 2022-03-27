const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeKind = enum {
    block,
    comment,
    def,
    fun,
    frac,
    leaf,
    space,
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
};

pub const Tree = struct {
    nodes: []Node,
};

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
            // Retain the buffer on the idea we might need that much again.
            return Tree{ .nodes = (try self.nodes.clone()).items };
        }

        fn advance(self: *Self) !?lex.Token {
            const token = (try self.readToken()) orelse return null;
            try self.working.append(.{ .kind = .leaf, .data = .{ .token = token } });
            return try self.peek();
        }

        fn block(self: *Self) !void {
            try self.space();
        }

        fn nest(self: *Self, kind: NodeKind, begin: usize) !void {
            // Move the working nodes into permanent.
            const nodes_begin = NodeId.of(self.nodes.items.len);
            try self.nodes.appendSlice(self.working.items[begin..self.working.items.len]);
            self.working.items.len = begin;
            // Make a new working node.
            const parent = Node{
                .kind = kind,
                .data = .{ .kids = NodeSlice.of(nodes_begin, self.nodes.items.len - nodes_begin.i) },
            };
            try self.working.append(parent);
        }

        fn peek(self: *Self) !?lex.Token {
            if (self.unread == null) {
                self.unread = try self.lexer.next();
            }
            return self.unread;
        }

        fn readToken(self: *Self) !?lex.Token {
            if (self.unread) |unread| {
                self.unread = null;
                return unread;
            }
            return try self.lexer.next();
        }

        fn space(self: *Self) !void {
            const begin = self.working.items.len;
            while (true) {
                switch (((try self.peek()) orelse break).kind) {
                    .comment, .hspace, .vspace => _ = try self.advance(),
                    else => break,
                }
            }
            const end = self.working.items.len;
            if (end > begin) {
                try self.nest(.space, begin);
            }
        }
    };
}
