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
            .assign_to => try self.simple(node),
            .be => try self.simple(node),
            .be_call => try self.simple(node),
            .block => try self.simple(node),
            .bool_and => try self.simple(node),
            .bool_or => try self.simple(node),
            .call => try self.simple(node),
            .colon => try self.simple(node),
            .comment, .space => try self.ignore(node),
            .compare => try self.simple(node),
            .def => try self.simple(node),
            .dot => try self.simple(node),
            .end => try self.simple(node),
            .escape => try self.simple(node),
            .frac => try self.simple(node),
            .fun => try self.simple(node),
            .fun_for => try self.simple(node),
            .fun_to => try self.simple(node),
            .fun_with => try self.simple(node),
            .leaf => try self.leaf(node),
            .list => try self.simple(node),
            .mul => try self.simple(node),
            .of => try self.simple(node),
            .question => try self.simple(node),
            .round => try self.round(node),
            .sign => try self.simple(node),
            .sign_int => try self.simple(node),
            .string => try self.simple(node),
        }
    }

    fn here(self: Self) NodeId {
        return NodeId.of(self.working.items.len);
    }

    fn ignore(self: *Self, node: parse.Node) !void {
        _ = self;
        _ = node;
    }

    fn leaf(self: *Self, node: parse.Node) !void {
        switch (node.data.token.kind) {
            .round_begin, .round_end => return,
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

    fn round(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        for (node.data.kids.from(self.parsed.nodes)) |kid| {
            try self.any(kid);
        }
        if (self.here().i == begin.i) {
            // Nest only if empty, for unit value.
            try self.nest(node.kind, begin);
        }
    }

    fn simple(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        for (node.data.kids.from(self.parsed.nodes)) |kid| {
            try self.any(kid);
        }
        try self.nest(node.kind, begin);
    }
};
