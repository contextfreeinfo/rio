const idx = @import("./idx.zig");
const lex = @import("./lex.zig");
const parse = @import("./parse.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeId = idx.Idx(u32, Node);
pub const NodeSlice = idx.IdxSlice(NodeId);

pub const NodeKind = enum(@TypeOf(@enumToInt(parse.NodeKind.leaf))) {
    call = @enumToInt(parse.NodeKind.call),
    leaf = @enumToInt(parse.NodeKind.leaf),
};

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
    // TODO Lookup arrays for ast nodes to parse nodes?
};

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
        return Tree {.nodes = &.{}};
    }
};
