const std = @import("std");
const Allocator = std.mem.Allocator;
const Index = u32;

pub const Interner = struct {
    arena: std.heap.ArenaAllocator,
    keys: std.ArrayList([]const u8),
    indices: std.StringHashMap(Index),

    pub fn init(allocator: Allocator) Interner {
        var arena = std.heap.ArenaAllocator.init(allocator);
        return .{
            .arena = arena,
            .keys = std.ArrayList([]const u8).init(allocator),
            .indices = std.StringHashMap(Index).init(allocator),
        };
    }

    pub fn deinit(self: *Interner) void {
        self.arena.deinit();
        self.keys.deinit();
        self.indices.deinit();
    }

    pub fn intern(self: *Interner, text: []const u8) !Index {
        var result = try self.indices.getOrPut(text);
        if (!result.found_existing) {
            std.debug.print("New!\n", .{});
            const index = self.keys.items.len;
            const key = try self.arena.allocator().dupe(u8, text);
            try self.keys.append(key);
            result.key_ptr.* = key;
            result.value_ptr.* = @intCast(Index, index);
        }
        return result.value_ptr.*;
    }
};
