const std = @import("std");
const Allocator = std.mem.Allocator;
const Index = u32;
const Size = u16;

pub const Pool = struct {
    begins: std.ArrayList(Index),
    indices: PoolHashMap,
    sizes: std.ArrayList(Size),
    text: std.ArrayList(u8),
    query: []const u8 = "",

    pub fn init(allocator: Allocator) !Pool {
        var pool = Pool{
            .begins = std.ArrayList(Index).init(allocator),
            // The context pool is updated on each use.
            .indices = PoolHashMap.initContext(allocator, PoolContext{ .pool = null }),
            .sizes = std.ArrayList(Size).init(allocator),
            .text = std.ArrayList(u8).init(allocator),
        };
        // Empty string is always index 0, just for convenience.
        _ = try pool.intern("");
        return pool;
    }

    pub fn deinit(self: *Pool) void {
        self.begins.deinit();
        self.indices.deinit();
        self.sizes.deinit();
        self.text.deinit();
    }

    fn get(self: Pool, i: Index) []const u8 {
        // TODO Use central pool. Mutex all access?
        // Special case to avoid access into empty text at start.
        if (i == 0) return "";
        // std.debug.print("sliceAt {} for size {}\n", .{i, self.begins.items.len});
        const begin = self.begins.items[i];
        return self.text.items[begin .. begin + self.sizes.items[i]];
    }

    pub fn intern(self: *Pool, text: []const u8) !Index {
        // TODO Use central pool. Mutex all access?
        // We may throw this away, but it should be a wash in the end.
        // Other text will take its place.
        // Only if the very last inquiry is duplicate and trips growth will this matter at all.
        // std.debug.print("Try begin at {} index at {}: {s} {}\n", .{begin, index, text, text.len});
        // Reset the context before using, since our original pointer is now bogus.
        self.indices.ctx.pool = self;
        self.query = text;
        var result = try self.indices.getOrPut(@intCast(Index, self.begins.items.len));
        if (!result.found_existing) {
            // std.debug.print("New\n", .{});
            // New, so store the new index.
            const index = @intCast(Index, self.begins.items.len);
            const begin = @intCast(Index, self.text.items.len);
            try self.text.appendSlice(text);
            try self.begins.append(begin);
            try self.sizes.append(@intCast(Size, text.len));
            result.key_ptr.* = index;
        }
        self.query = "";
        return result.key_ptr.*;
    }
};

const PoolHashMap = std.HashMap(Index, void, PoolContext, std.hash_map.default_max_load_percentage);

// Based on `StringContext` in `hash_map.zig`.
pub const PoolContext = struct {
    // I can't just make this a pointer to pool because Zig whines about circular.
    pool: ?*anyopaque,

    pub fn hash(self: @This(), i: Index) u64 {
        _ = self;
        return std.hash_map.hashString(self.get(i));
    }

    pub fn eql(self: @This(), a: Index, b: Index) bool {
        _ = self;
        return std.hash_map.eqlString(self.get(a), self.get(b));
    }

    fn get(self: @This(), i: Index) []const u8 {
        const pool = @ptrCast(*const Pool, @alignCast(8, self.pool));
        return if (i < pool.begins.items.len) pool.get(i) else pool.query;
    }
};
