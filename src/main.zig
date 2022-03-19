const intern = @import("./intern.zig");
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
    var reader = std.io.bufferedReader(file.reader()).reader();
    var text = std.ArrayList(u8).init(allocator);
    defer text.deinit();
    var lexer = lex.Lexer(@TypeOf(reader)).init(allocator, reader, &text);
    defer lexer.deinit();
    var n = @as(u32, 0);
    // TODO Arena for intern buffer.
    var pool = intern.Pool.init(allocator);
    defer pool.deinit();
    while (true) {
        // TODO Intern ids, keys, ops, whitespace, and small strings & numbers
        // TODO Into array list (indices) or into arena (pointers)?
        text.clearRetainingCapacity();
        const token = (try lexer.next()) orelse break;
        std.debug.print("{} {s} {}\n", .{ token, text.items, text.items.len });
        _ = try pool.intern(text.items);
        n += 1;
    }
    std.debug.print("Read: {} {} {}, {} {}\n", .{ n, lexer.index, pool.keys.items.len, lexer.line, lexer.col });
    const arena_len = pool.arena.state.buffer_list.len();
    const node_size = pool.arena.state.buffer_list.first.?.data.len;
    std.debug.print("Intern arena: {} {} {}\n", .{ arena_len, node_size, pool.arena.state.end_index });
}

test "basic test" {
    try std.testing.expectEqual(10, 3 + 7);
}
