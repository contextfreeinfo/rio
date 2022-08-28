const extract = @import("./extract.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const norm = @import("./norm.zig");
const parse = @import("./parse.zig");
const resolve = @import("./resolve.zig");
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
    // TODO Who owns the path?
    path: []const u8,

    const Self = @This();

    pub fn deinit(self: *Self) void {
        self.normed.deinit(self.allocator);
        self.parsed.deinit(self.allocator);
    }
};

pub fn Worker(comptime Reader: type) type {
    return struct {
        allocator: Allocator,
        // TODO Makes queues of jobs with stage per work job item. Allows pipelining.
        // TODO For each stage, one processor per core. Memory waste?
        // TODO Fine-grained lock on interns during io might be ok?
        // TODO Or do full io then single lexer that owns interns most of the time?
        extractor: extract.Extractor,
        normer: norm.Normer,
        parser: parse.Parser(Reader),
        pool: *intern.Pool,
        resolver: resolve.Resolver,

        const Self = @This();

        pub fn init(allocator: Allocator, pool: *intern.Pool) !Self {
            return Self{
                .allocator = allocator,
                .extractor = try extract.Extractor.init(allocator),
                .normer = try norm.Normer.init(allocator),
                .parser = try parse.Parser(Reader).init(allocator, pool),
                .resolver = try resolve.Resolver.init(allocator),
                .pool = pool,
            };
        }

        pub fn deinit(self: *Self) void {
            self.extractor.deinit();
            self.normer.deinit();
            self.parser.deinit();
            self.resolver.deinit();
        }

        pub fn process(self: *Self, reader: Reader) !Script {
            // TODO Write lock interns during parse or at finer grain?
            const parsed = try self.parser.parse(reader);
            // Most stages shouldn't ever need even read lock on interns.
            const normed = try self.normer.build(parsed);
            _ = try self.extractor.extract(normed);
            _ = try self.resolver.resolve(normed);
            return Script{ .allocator = self.allocator, .normed = normed, .parsed = parsed, .path = "" };
        }
    };
}

// TODO Virtual file system.
// TODO Function-pointer-based buffer filler has got to be fast enough, right?
pub const FileBufferedReader = std.io.BufferedReader(4096, std.fs.File.Reader);
pub const FileReader = std.io.Reader(*FileBufferedReader, std.fs.File.Reader.Error, FileBufferedReader.read);
pub const FileWorker = Worker(FileReader);

pub fn dumpTree(worker: *FileWorker, name: String, parsed_out: anytype, normed_out: anytype) !void {
    const old_pool_storage = worker.pool.text.items.len;
    const file = try std.fs.cwd().openFile(name, .{});
    defer file.close();
    var bufReader = std.io.bufferedReader(file.reader());
    var reader = bufReader.reader();
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
