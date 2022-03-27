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
    // TODO Need to support restart with new reader.
    var parser = try parse.Parser(@TypeOf(reader)).init(allocator, &pool);
    defer parser.deinit();
    const tree = try parser.parse(reader);
    defer tree.deinit(allocator);
    try tree.print(std.io.getStdErr().writer());
    // var lexer = &parser.lexer;
    // lexer.start(reader);
    // var byte_count = @as(usize, 0);
    // var line_count = @as(usize, 0);
    // var token_count = @as(usize, 0);
    // while (true) {
    //     const token = (try lexer.next()) orelse break;
    //     _ = token;
    //     // std.debug.print("{} {s} {}\n", .{ token, lexer.text.items, lexer.text.items.len });
    //     byte_count += lexer.text.items.len;
    //     if (token.kind == lex.TokenKind.vspace) {
    //         line_count += 1;
    //     }
    //     token_count += 1;
    // }
    // std.debug.print("Read: {} {} {} {}\n", .{ token_count, byte_count, pool.begins.items.len, line_count });
    std.debug.print("Intern storage: {} {}\n", .{ pool.text.capacity, pool.text.items.len });
    std.debug.print("Token size: {} node size: {} {}\n", .{ @sizeOf(lex.Token), @sizeOf(parse.Node), @sizeOf(parse.NodeKind) });
}

test "basic test" {
    try std.testing.expectEqual(10, 3 + 7);
}
