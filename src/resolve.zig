const norm = @import("./norm.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const Tree = norm.Tree;

pub const Resolver = struct {
    const Self = @This();

    pub fn init(allocator: Allocator) !Self {
        _ = allocator;
        return Self{};
    }

    pub fn deinit(self: *Self) void {
        _ = self;
    }

    pub fn resolve(self: *Self, tree: Tree) !void {
        _ = self;
        _ = tree;
        // TODO Build a side array with matched indexing to tree?
    }
};
