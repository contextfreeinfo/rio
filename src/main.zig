const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const parse = @import("./parse.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

const String = []const u8;

pub fn main() !void {
    const allocator = std.heap.page_allocator;
    const out = std.io.getStdOut().writer();
    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len < 3) {
        try out.print("Usage: rio run [script.rio]", .{});
        // std.process.exit(1);
        return;
    }
    const name = args[2];
    try out.print("Run {s}\n", .{name});
    try dumpTree(allocator, out, name);
}

fn dumpTree(allocator: Allocator, out: anytype, name: String) !void {
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
    try tree.print(out, .{ .pool = pool });
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
    try out.print("Intern storage: {} {}\n", .{ pool.text.capacity, pool.text.items.len });
    try out.print("Tree size: {}\n", .{tree.nodes.len});
    try out.print("Token size: {} node size: {} {}\n", .{ @sizeOf(lex.Token), @sizeOf(parse.Node), @sizeOf(parse.NodeKind) });
}

test "dump trees" {
    const allocator = std.heap.page_allocator;
    // TODO Walk up tree to project root?
    // TODO List dir?
    const names = [_][]const u8{ "boolerr", "fib", "hello", "persons", "wild" };
    for (names) |name| {
        // Prep write.
        const out_name = try std.fmt.allocPrint(allocator, "tests/trees/{s}.tree.txt", .{name});
        defer allocator.free(out_name);
        const file = try std.fs.cwd().createFile(out_name, .{});
        defer file.close();
        var buffer = std.io.bufferedWriter(file.writer());
        // Read.
        const in_name = try std.fmt.allocPrint(allocator, "tests/{s}.rio", .{name});
        defer allocator.free(in_name);
        // std.debug.print("Processing: {s}\n", .{in_name});
        try dumpTree(allocator, buffer.writer(), in_name);
        try buffer.flush();
        // TODO Assert no changes in git?
        // try std.testing.expectEqual(10, 3 + 7);
    }
}
