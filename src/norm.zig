const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
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
            .dot => try self.dot(node),
            .escape => try self.simple(node),
            .frac => try self.simple(node),
            .fun => try self.simple(node),
            .fun_for => try self.funFor(node),
            .fun_to => try self.simple(node), // TODO What here?
            .fun_with => try self.simple(node),
            .leaf => try self.leaf(node),
            .list => try self.list(node),
            .mul => try self.simple(node),
            .of => try self.of(node),
            .question => try self.simple(node),
            .round => try self.round(node, false),
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
        const begin = self.here();
        // TODO Don't hand fun_for or fun_to here! First aren't callees there!
        const node_kids = self.kidsFrom(node);
        // We create empty calls from rounds, but shouldn't receive empty calls.
        std.debug.assert(node_kids.len > 0);
        const first = node_kids[0];
        switch (first.kind) {
            .dot => try self.dotSwap(first), // TODO If it was a chain, then what???
            .round => try self.round(first, false),
            else => try self.any(first),
        }
        for (node_kids[1..]) |kid| {
            // TODO Also need this logic for `of`.
            // TODO Add blank labels to each uncertain position vs named arg.
            switch (kid.kind) {
                .round => try self.round(kid, true),
                else => try self.any(kid),
            }
        }
        try self.nest(node.kind, begin);
    }

    fn dot(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.dotSwap(node);
        try self.nest(.call, begin);
    }

    fn dotChain(self: *Self, target: ?parse.Node, chain: parse.Node) !void {
        try self.any(chain);
        if (target) |target_sure| {
            try self.any(target_sure);
        }
    }

    fn dotSwap(self: *Self, node: parse.Node) !void {
        // Invert order of first two.
        var dot_kids = self.kidsFrom(node);
        // Skip past dot.
        var dot_index = dot_kids.len;
        for (dot_kids) |kid, index| {
            if (isLeafOf(kid, .op_dot)) {
                dot_index = index;
                break;
            }
        }
        // See if we have something.
        if (dot_index + 1 < dot_kids.len) {
            for (dot_kids[dot_index + 1 ..]) |kid| {
                switch (kid.kind) {
                    .comment, .end, .space => {},
                    .of => {
                        try self.dotChain(if (dot_index > 0) dot_kids[0] else null, kid);
                        return;
                    },
                    else => {
                        try self.any(kid);
                        break;
                    },
                }
            }
        }
        // And append the first if we had any.
        if (dot_index > 0) {
            try self.any(dot_kids[0]);
        }
    }

    fn funFor(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        const node_kids = self.kidsFrom(node);
        if (node_kids.len > 0) {
            // Expand block or call if present.
            for (node_kids) |kid| {
                switch (kid.kind) {
                    .block, .call => {
                        // TODO Special params handling of kids.
                        try self.kids(kid);
                        break;
                    },
                    else => {},
                }
            } else {
                // We have one kid at most. Handle it.
                // TODO Special params handling of kids.
                try self.kids(node);
            }
        }
        try self.nest(node.kind, begin);
    }

    fn here(self: Self) NodeId {
        return NodeId.of(self.working.items.len);
    }

    fn ignore(self: *Self, node: parse.Node) !void {
        _ = self;
        _ = node;
    }

    fn kids(self: *Self, node: parse.Node) !void {
        for (self.kidsFrom(node)) |kid| {
            try self.any(kid);
        }
    }

    fn kidsFrom(self: Self, node: parse.Node) []const parse.Node {
        return node.data.kids.from(self.parsed.nodes);
    }

    fn leaf(self: *Self, node: parse.Node) !void {
        switch (node.data.token.kind) {
            .escape_begin, .escape_end, .key_be, .key_of, .key_for, .key_to, .key_with, .op_comma, .op_question, .round_begin, .round_end => return,
            else => try self.working.append(node),
        }
    }

    fn list(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.working.append(leafOf(.key_blank));
        try self.kids(node);
        try self.nest(.call, begin);
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
        for (self.kidsFrom(node)) |kid| {
            if (kid.kind == .block) {
                // Always expand the block.
                try self.kids(kid);
                break;
            }
        }
    }

    fn round(self: *Self, node: parse.Node, wrap_all: bool) !void {
        const begin = self.here();
        try self.kids(node);
        const last = &self.working.items[self.working.items.len - 1];
        if (last.kind == .block) {
            last.kind = .call;
        }
        // TODO If block back make it into a big call???
        // Equals means void literal.
        if (wrap_all or self.here().i == begin.i) {
            try self.nest(node.kind, begin);
        }
    }

    fn simple(self: *Self, node: parse.Node) !void {
        const begin = self.here();
        try self.kids(node);
        try self.nest(node.kind, begin);
    }
};

fn isLeafOf(node: parse.Node, kind: lex.TokenKind) bool {
    return node.kind == .leaf and node.data.token.kind == kind;
}

fn leafOf(kind: lex.TokenKind) Node {
    return .{ .kind = .leaf, .data = .{ .token = .{ .kind = kind, .text = intern.TextId.of(0) } } };
}
