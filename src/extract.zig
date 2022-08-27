const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const norm = @import("./norm.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

// TODO Figure out where to define this.
pub const FileId = i32;

pub const Def = struct {
    // Could make a function to look up text from node, but this should make
    // scanning defs faster.
    text: intern.TextId,
    // This allows defs to stand alone for lookup.
    file: FileId,
    node: norm.NodeId,
    // Could just make a linked tree of defs and skip envs.
    // But envs allow easier linear looping through larger lists of defs.
    // parent: DefId,
};
pub const DefId = idx.Idx(u32, Def);
pub const DefSlice = idx.IdxSlice(DefId);

pub const Env = struct {
    // Presume we have enough tree structure that linear scan is good enough.
    // If someone defines 100s or 1000s or more in a single scope, this could
    // be slow.
    // TODO Allow a hash option for module level or otherwise?
    defs: DefSlice,
    parent: EnvId,
};
pub const EnvId = idx.Idx(u32, Def);

pub const Extraction = struct {
    defs: []Def,
    envs: []Env,
};

pub const Extractor = struct {
    defs: std.ArrayList(Def),
    envs: std.ArrayList(Env),
    normed: norm.Tree,

    const Self = @This();

    pub fn init(allocator: Allocator) !Self {
        return Self{
            .defs = std.ArrayList(Def).init(allocator),
            .envs = std.ArrayList(Env).init(allocator),
            .normed = .{ .nodes = &.{} },
        };
    }

    pub fn deinit(self: *Self) void {
        self.defs.deinit();
        self.envs.deinit();
    }

    pub fn extract(self: *Self, normed: norm.Tree) !void {
        _ = self;
        _ = normed;
    }
};
