const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeKind = enum {
    block,
    call,
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

    const Self = @This();

    pub fn print(self: Self, writer: anytype, context: TreePrintContext) std.os.WriteError!void {
        _ = self;
        var i = @as(@TypeOf(context.indent), 0);
        while (i < context.indent) : (i += 1) {
            try writer.print(" ", .{});
        }
        try writer.print("{}", .{self.kind});
        switch (self.kind) {
            .leaf => try writer.print(": {}\n", .{self.data.token.kind}),
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

const TreePrintContext = struct {
    tree: Tree,
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

    pub fn print(self: Self, writer: anytype) !void {
        // try writer.print("nodes: {}\n", .{self.nodes.len});
        if (self.root()) |r| {
            try r.print(writer, .{ .tree = self, .indent = 0 });
        } else {
            try writer.print("()\n", .{});
        }
    }
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

        fn block(self: *Self) !void {
            const begin = self.working.items.len;
            while (true) {
                try self.space();
                _ = (try self.peek()) orelse break;
                try self.blockItem();
                _ = (try self.peek()) orelse break;
            }
            if (self.working.items.len > begin) {
                try self.nest(.block, NodeId.of(begin));
            }
        }

        fn blockItem(self: *Self) !void {
            const begin = self.working.items.len;
            while (true) {
                _ = (try self.advance()).?;
                // TODO Is EndOfStream easier here in parsing?
                try self.hspace();
                switch (((try self.peek()) orelse break).kind) {
                    .vspace => break,
                    else => {},
                }
            }
            try self.nest(.call, NodeId.of(begin));
        }

        fn hspace(self: *Self) !void {
            const begin = self.working.items.len;
            while (true) {
                switch (((try self.peek()) orelse break).kind) {
                    .comment, .hspace => _ = try self.advance(),
                    else => break,
                }
            }
            const end = self.working.items.len;
            if (end > begin) {
                try self.nest(.space, NodeId.of(begin));
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
                try self.nest(.space, NodeId.of(begin));
            }
        }
    };
}
