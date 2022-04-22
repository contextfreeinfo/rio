const idx = @import("./idx.zig");
const lex = @import("./lex.zig");
const parse = @import("./parse.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeId = idx.Idx(u32, Node);
pub const NodeSlice = idx.IdxSlice(NodeId);

// TODO Eventually replace with just the subset we use here.
pub const NodeKind = parse.NodeKind;

// pub const NodeKind = enum(@TypeOf(@enumToInt(parse.NodeKind.leaf))) {
//     call = @enumToInt(parse.NodeKind.call),
//     leaf = @enumToInt(parse.NodeKind.leaf),
// };

pub const Node = parse.Node;

// pub const Node = struct {
//     kind: NodeKind,
//     data: union {
//         // Both of these are pairs of u32 values.
//         kids: NodeSlice,
//         token: lex.Token,
//     },
// };

pub const Tree = parse.Tree;

// pub const Tree = struct {
//     nodes: []Node,
//     // TODO Lookup arrays for ast nodes to parse nodes?
// };

pub const NormError = error{OutOfMemory};

pub const Normer = struct {
    nodes: std.ArrayList(Node),
    parsed: parse.Tree,
    working: std.ArrayList(Node),

    const Self = @This();

    pub fn init(allocator: Allocator) !Self {
        return Self{
            .nodes = std.ArrayList(Node).init(allocator),
            .parsed = .{ .nodes = &.{} },
            .working = std.ArrayList(Node).init(allocator),
        };
    }

    pub fn deinit(self: *Self) void {
        self.nodes.deinit();
        self.working.deinit();
    }

    pub fn build(self: *Self, parsed: parse.Tree) !Tree {
        self.parsed = parsed;
        try self.any(parsed.root());
        // This one doesn't ever appear but should push down the block if any.
        try self.nest(.block, NodeId.of(0));
        return Tree{ .nodes = (try self.nodes.clone()).items };
    }

    fn any(self: *Self, node: parse.Node) NormError!void {
        switch (node.kind) {
            .add => try self.simple(node),
            .as => try self.simple(node),
            .assign => try self.simple(node),
            .assign_to => try self.simple(node), // TODO Convert to assign.
            .be => try self.simple(node),
            .block => try self.block(node),
            .bool_and => try self.simple(node),
            .bool_or => try self.simple(node),
            .call => try self.call(node),
            .colon => try self.simple(node),
            .comment, .end, .space => try self.ignore(node),
            .compare => try self.simple(node),
            .def => try self.simple(node),
            .dot => try self.simple(node), // TODO Turn dot into prefix calls. Nest multi-dots.
            .escape => try self.simple(node),
            .frac => try self.simple(node),
            .fun => try self.simple(node),
            .fun_for => try self.simple(node),
            .fun_to => try self.simple(node),
            .fun_with => try self.simple(node),
            .leaf => try self.leaf(node),
            .list => try self.simple(node),
            .mul => try self.simple(node),
            .of => try self.of(node),
            .question => try self.simple(node),
            .round => try self.round(node),
            .sign => try self.simple(node),
            .sign_int => try self.simple(node),
            .string => try self.simple(node),
        }
    }

    fn block(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.kids(node);
        if (self.here().i > begin.i + 1) {
            try self.nest(node.kind, begin);
        }
    }

    fn call(self: *Self, node: parse.Node) !void {
        // TODO If first is dot, invert some, where first is always numbered.
        try self.simple(node);
        // TODO If first is an one-child call, unwrap it.
        // TODO Add numbered colon labels to each guaranteed positional arg.
    }

    fn here(self: Self) NodeId {
        return NodeId.of(self.working.items.len);
    }

    fn ignore(self: *Self, node: parse.Node) !void {
        _ = self;
        _ = node;
    }

    fn kids(self: *Self, node: parse.Node) !void {
        for (node.data.kids.from(self.parsed.nodes)) |kid| {
            try self.any(kid);
        }
    }

    fn leaf(self: *Self, node: parse.Node) !void {
        switch (node.data.token.kind) {
            .escape_begin, .escape_end, .key_be, .key_of, .key_for, .key_to, .key_with, .round_begin, .round_end => return,
            else => try self.working.append(node),
        }
    }

    // TODO Work out how to combine with Parser.nest.
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

    fn of(self: *Self, node: parse.Node) !void {
        for (node.data.kids.from(self.parsed.nodes)) |kid| {
            switch (kid.kind) {
                .block => try self.kids(kid),
                else => try self.any(kid),
            }
        }
    }

    fn round(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.kids(node);
        const end = self.here();
        switch (end.i - begin.i) {
            0 => try self.nest(.call, begin), // Void.
            1 => if (self.parsed.nameOf(self.working.items[end.i - 1])) |_| {
                // This could be interpreted as a named arg, so wrap it.
                try self.nest(.call, begin);
            },
            else => {},
        }
    }

    fn simple(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.kids(node);
        try self.nest(node.kind, begin);
    }
};
