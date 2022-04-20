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
        try self.any(parsed.root());
        return Tree{ .nodes = &.{} };
    }

    fn any(self: *Self, node: parse.Node) !void {
        switch (node.kind) {
            .add => try self.ignore(node),
            .as => try self.ignore(node),
            .assign => try self.ignore(node),
            .assign_to => try self.ignore(node),
            .be => try self.ignore(node),
            .be_call => try self.ignore(node),
            .block => try self.block(node),
            .bool_and => try self.ignore(node),
            .bool_or => try self.ignore(node),
            .call => try self.ignore(node),
            .colon => try self.ignore(node),
            .comment => try self.ignore(node),
            .compare => try self.ignore(node),
            .def => try self.ignore(node),
            .dot => try self.ignore(node),
            .end => try self.ignore(node),
            .escape => try self.ignore(node),
            .frac => try self.ignore(node),
            .fun => try self.ignore(node),
            .fun_for => try self.ignore(node),
            .fun_to => try self.ignore(node),
            .fun_with => try self.ignore(node),
            .leaf => try self.ignore(node),
            .list => try self.ignore(node),
            .mul => try self.ignore(node),
            .of => try self.ignore(node),
            .question => try self.ignore(node),
            .round => try self.ignore(node),
            .sign => try self.ignore(node),
            .sign_int => try self.ignore(node),
            .space => try self.ignore(node),
            .string => try self.ignore(node),
        }
    }

    fn block(self: *Self, node: parse.Node) !void {
        _ = self;
        _ = node;
    }

    fn ignore(self: *Self, node: parse.Node) !void {
        _ = self;
        _ = node;
    }
};
