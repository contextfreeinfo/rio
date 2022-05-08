const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const norm = @import("./norm.zig");
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
    // Get going.
    var pool = try intern.Pool.init(allocator);
    defer pool.deinit();
    var worker = try FileWorker.init(allocator, &pool);
    defer worker.deinit();
    try dumpTree(&worker, name, out, out);
}

pub const Script = struct {
    allocator: Allocator,
    normed: norm.Tree,
    parsed: parse.Tree,

    const Self = @This();

    pub fn deinit(self: *Self) void {
        self.normed.deinit(self.allocator);
        self.parsed.deinit(self.allocator);
    }
};

pub fn Worker(comptime Reader: type) type {
    return struct {
        allocator: Allocator,
        normer: norm.Normer,
        parser: parse.Parser(Reader),
        pool: *intern.Pool,

        const Self = @This();

        pub fn init(allocator: Allocator, pool: *intern.Pool) !Self {
            return Self{
                .allocator = allocator,
                .normer = try norm.Normer.init(allocator),
                .parser = try parse.Parser(Reader).init(allocator, pool),
                .pool = pool,
            };
        }

        pub fn deinit(self: *Self) void {
            self.normer.deinit();
            self.parser.deinit();
        }

        pub fn process(self: *Self, reader: Reader) !Script {
            const parsed = try self.parser.parse(reader);
            const normed = try self.normer.build(parsed);
            return Script{ .allocator = self.allocator, .normed = normed, .parsed = parsed };
        }
    };
}

const FileBufferedReader = std.io.BufferedReader(4096, std.fs.File.Reader);
const FileReader = std.io.Reader(*FileBufferedReader, std.fs.File.Reader.Error, FileBufferedReader.read);
const FileWorker = Worker(FileReader);

fn dumpTree(worker: *FileWorker, name: String, parsed_out: anytype, normed_out: anytype) !void {
    const old_pool_storage = worker.pool.text.items.len;
    const file = try std.fs.cwd().openFile(name, .{});
    defer file.close();
    var reader = std.io.bufferedReader(file.reader()).reader();
    var script = try worker.process(reader);
    defer script.deinit();
    // Parsed.
    try script.parsed.print(parsed_out, .{ .pool = worker.pool.* });
    const pool_increase = worker.pool.text.items.len - old_pool_storage;
    try parsed_out.print("Pool storage: {} {} ({} -> {})\n", .{ worker.pool.text.capacity, pool_increase, old_pool_storage, worker.pool.text.items.len });
    try parsed_out.print("Parsed size: {}\n", .{script.parsed.nodes.len});
    try parsed_out.print("Token size: {} node size: {} {}\n", .{ @sizeOf(lex.Token), @sizeOf(parse.Node), @sizeOf(parse.NodeKind) });
    // Normed.
    try script.normed.print(normed_out, .{ .pool = worker.pool.* });
    try normed_out.print("Normed size: {}\n", .{script.normed.nodes.len});
}

fn createFile(allocator: Allocator, comptime template: []const u8, name: []const u8) !std.fs.File {
    const out_name = try std.fmt.allocPrint(allocator, template, .{name});
    defer allocator.free(out_name);
    return try std.fs.cwd().createFile(out_name, .{});
}

test "dump trees" {
    const allocator = std.heap.page_allocator;
    var pool = try intern.Pool.init(allocator);
    defer pool.deinit();
    var worker = try FileWorker.init(allocator, &pool);
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
        try dumpTree(&worker, in_name, parsed_buf.writer(), normed_buf.writer());
        try parsed_buf.flush();
        try normed_buf.flush();
        // TODO Assert no changes in git?
        // try std.testing.expectEqual(10, 3 + 7);
    }
}
