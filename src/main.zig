const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

const max_file_size = 10 << 20;

const Script = struct {
    arena: std.heap.ArenaAllocator,
    source: []u8,

    fn init(allocator: Allocator) Script {
        return Script{
            .arena = std.heap.ArenaAllocator.init(allocator),
        };
    }
};

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();
    const args = try std.process.argsAlloc(allocator);
    if (args.len < 3) {
        try stdout.print("Usage: rio run [script.rio]", .{});
        // std.process.exit(1);
        return;
    }
    const name = args[2];
    try stdout.print("Run {s}", .{name});
    const file = try std.fs.cwd().openFile(name, .{});
    defer file.close();
    const source = try file.reader().readAllAlloc(allocator, max_file_size);
    _ = source;
    _ = try lex.lex(allocator, source);
}

test "basic test" {
    try std.testing.expectEqual(10, 3 + 7);
}
