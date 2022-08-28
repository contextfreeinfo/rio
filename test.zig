const intern = @import("src/intern.zig");
const lex = @import("src/lex.zig");
const main = @import("src/main.zig");
const parse = @import("src/parse.zig");
const std = @import("std");

const Allocator = std.mem.Allocator;
const String = []const u8;

// TODO Remove this once we abstract more. Or keep it? Focus on slice reading?
const SliceStream = @TypeOf(std.io.fixedBufferStream(""));
const SliceReader = std.io.Reader(*SliceStream, SliceStream.ReadError, SliceStream.read);
const SliceWorker = main.Worker(SliceReader);

fn createFile(allocator: Allocator, comptime template: []const u8, name: []const u8) !std.fs.File {
    const out_name = try std.fmt.allocPrint(allocator, template, .{name});
    defer allocator.free(out_name);
    return try std.fs.cwd().createFile(out_name, .{});
}

test "dump trees" {
    const allocator = std.heap.page_allocator;
    var pool = try intern.Pool.init(allocator);
    defer pool.deinit();
    var worker = try main.FileWorker.init(allocator, &pool);
    defer worker.deinit();
    // TODO Walk up tree to project root?
    // TODO List dir?
    const names = [_][]const u8{ "boolerr", "fib", "hello", "persons", "wild" };
    for (names) |name| {
        // Parsed out.
        const parsed_file = try createFile(allocator, "tests/trees/{s}.parsed.txt", name);
        defer parsed_file.close();
        var parsed_buf = std.io.bufferedWriter(parsed_file.writer());
        // Normed ast out.
        const normed_file = try createFile(allocator, "tests/trees/{s}.normed.txt", name);
        defer normed_file.close();
        var normed_buf = std.io.bufferedWriter(normed_file.writer());
        // Process.
        const in_name = try std.fmt.allocPrint(allocator, "tests/{s}.rio", .{name});
        defer allocator.free(in_name);
        try main.dumpTree(&worker, in_name, parsed_buf.writer(), normed_buf.writer());
        try parsed_buf.flush();
        try normed_buf.flush();
        // TODO Assert no changes in git?
        // try std.testing.expectEqual(10, 3 + 7);
    }
}

test "parse speed" {
    const allocator = std.heap.page_allocator;
    const text = @embedFile("tests/boolerr.rio");
    var buffer = std.ArrayList(u8).init(allocator);
    defer buffer.deinit();
    var n = @as(usize, 0);
    const scale = try std.fmt.parseInt(usize, std.os.getenv("RIO_TEST_SCALE") orelse "1", 10);
    while (n < scale) : (n += 1) {
        try buffer.appendSlice(text);
    }
    // TODO Print stats to report file.
    // std.debug.print("Size: {}\n", .{buffer.items.len});
    var bufStream = std.io.fixedBufferStream(@as([]const u8, buffer.items));
    var reader: SliceReader = bufStream.reader();
    // Process it.
    // TODO Engine should include pool.
    var pool = try intern.Pool.init(allocator);
    defer pool.deinit();
    var worker = try SliceWorker.init(allocator, &pool);
    defer worker.deinit();
    var script = try worker.process(reader);
    defer script.deinit();
}
