const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const parse = @import("./parse.zig");
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
    var reader = std.io.bufferedReader(file.reader()).reader();
    var pool = try intern.Pool.init(allocator);
    defer pool.deinit();
    var parser = try parse.Parser(@TypeOf(reader)).init();
    defer parser.deinit();
    // TODO Then parse into tree, passing lexer into parser. Still param'd on (just?) Reader?
    var lexer = try lex.Lexer(@TypeOf(reader)).init(allocator, &pool, reader);
    defer lexer.deinit();
    var n = @as(u32, 0);
    while (true) {
        const token = (try lexer.next()) orelse break;
        _ = token;
        // std.debug.print("{} {s} {}\n", .{ token, lexer.text.items, lexer.text.items.len });
        n += 1;
    }
    std.debug.print("Read: {} {} {}, {} {}\n", .{ n, lexer.index, pool.begins.items.len, lexer.line, lexer.col });
    // const arena_len = pool.arena.state.buffer_list.len();
    // const node_size = pool.arena.state.buffer_list.first.?.data.len;
    std.debug.print("Intern storage: {} {}\n", .{ pool.text.capacity, pool.text.items.len });
    std.debug.print("Token size: {}\n", .{@sizeOf(lex.Token)});
}

test "basic test" {
    try std.testing.expectEqual(10, 3 + 7);
}
