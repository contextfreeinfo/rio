const std = @import("std");
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;
const String = []const u8;

// Limit size even on 64-bit systems.
const Index = u32;

// From `std.sort`.
const Range = struct {
    start: Index,
    end: Index,

    fn init(start: Index, end: Index) Range {
        return Range{
            .start = start,
            .end = end,
        };
    }

    fn len(self: Range) Index {
        return self.end - self.start;
    }
};

pub const Token = union(enum) {
    add,
    as,
    be,
    case,
    colon,
    comment: Range,
    div,
    do,
    dot,
    else_key,
    end,
    eq,
    eq_eq,
    float: Range,
    for_key,
    ge,
    geq,
    id: Range,
    int: Range,
    le,
    leq,
    mul,
    round_begin,
    round_end,
    string,
    struct_key,
    sub,
    to,
    use,
    when_key,
};

pub fn Lexer(comptime Alloc: type, comptime Reader: type) type {
    return struct {
        allocator: Alloc,
        reader: Reader,

        const Self = @This();

        pub fn deinit(self: Self) void {
            _ = self;
        }

        pub fn lex(self: Self) !void {
            var reader = self.reader;
            var n = @as(u32, 0);
            while (true) {
                const byte = reader.readByte() catch |err| switch (err) {
                    error.EndOfStream => break,
                    else => |e| return e,
                };
                _ = byte;
                n += 1;
            }
            std.debug.print("Read: {} {}\n", .{ n, @sizeOf(Token) });
            // _ = Token.float;
            // var tokens = ArrayList(Token).init(allocator);
            // try tokens.append(Token.dot);
            // // TODO Allocate string data in one buffer and tokens in another.
            // // TODO Use u32 indices into text buffer.
            // try tokens.append(Token{ .id = "hi" });
            // return tokens;
        }

        pub fn next(self: Self) !Token {
            _ = self;
            return Token.dot;
        }
    };
}

pub fn lexer(allocator: Allocator, reader: anytype) Lexer(@TypeOf(allocator), @TypeOf(reader)) {
    return .{
        .allocator = allocator,
        .reader = reader,
    };
}
