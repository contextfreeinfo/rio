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
    const allocator = std.heap.page_allocator;
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len < 3) {
        try stdout.print("Usage: rio run [script.rio]", .{});
        // std.process.exit(1);
        return;
    }
    const name = args[2];
    try stdout.print("Run {s}\n", .{name});
    const file = try std.fs.cwd().openFile(name, .{});
    defer file.close();
    var buffering = std.io.bufferedReader(file.reader());
    // TODO Change to streaming bytes from buffered reader.
    // const source = try file.reader().readAllAlloc(allocator, max_file_size);
    // _ = source;
    // try lex.lex(allocator, buffering.reader());
    var lexer = lex.lexer(allocator, buffering.reader());
    defer lexer.deinit();
    _ = try lexer.next();
    try lexer.lex();
}

test "basic test" {
    try std.testing.expectEqual(10, 3 + 7);
}
