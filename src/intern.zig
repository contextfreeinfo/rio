const std = @import("std");
const Allocator = std.mem.Allocator;
const Index = u32;

pub const Pool = struct {
    arena: std.heap.ArenaAllocator,
    keys: std.ArrayList([]const u8),
    indices: std.StringHashMap(Index),

    pub fn init(allocator: Allocator) !Pool {
        var arena = std.heap.ArenaAllocator.init(allocator);
        var pool = Pool{
            .arena = arena,
            .keys = std.ArrayList([]const u8).init(allocator),
            .indices = std.StringHashMap(Index).init(allocator),
        };
        // Empty string is always index 0, just for convenience.
        _ = try pool.intern("");
        return pool;
    }

    pub fn deinit(self: *Pool) void {
        self.arena.deinit();
        self.keys.deinit();
        self.indices.deinit();
    }

    pub fn intern(self: *Pool, text: []const u8) !Index {
        // TODO Use central arena. Mutex on this function?
        var result = try self.indices.getOrPut(text);
        if (!result.found_existing) {
            // std.debug.print("New!\n", .{});
            const index = self.keys.items.len;
            const key = try self.arena.allocator().dupe(u8, text);
            try self.keys.append(key);
            result.key_ptr.* = key;
            result.value_ptr.* = @intCast(Index, index);
        }
        return result.value_ptr.*;
    }
};
