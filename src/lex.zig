const std = @import("std");
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;
const String = []const u8;

// Limit size even on 64-bit systems.
const Index = u32;

// From `std.sort`.
// TODO Need this in tree?
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

pub const TokenKind = union(enum) {
    add,
    as,
    be,
    case,
    colon,
    comment, // has text
    div,
    do,
    dot,
    else_key,
    end,
    eol,
    eq,
    eq_eq,
    float, // has text
    for_key,
    ge,
    geq,
    hspace, // has text
    id, // has text
    int, // has text
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

pub const Token = struct {
    kind: TokenKind,
    begin: Index,
};

pub fn Lexer(comptime Reader: type) type {
    return struct {
        buffer: ArrayList(u8),
        col: Index,
        index: Index,
        line: Index,
        reader: Reader,
        unread: ?u8,

        const Self = @This();

        pub fn deinit(self: Self) void {
            self.buffer.deinit();
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

        pub fn peek(self: *Self) !u8 {
            if (self.unread == null) {
                self.unread = try self.reader.readByte();
            }
            return self.unread.?;
        }

        pub fn next(self: *Self) !Token {
            self.buffer.clearRetainingCapacity();
            switch (try self.peek()) {
                ' ', '\t' => {},
                else => {},
            }
            // TODO Lex.
            return Token{.kind = TokenKind.dot, .begin = 0};
        }
    };
}

pub fn lexer(allocator: Allocator, reader: anytype) Lexer(@TypeOf(reader)) {
    return .{
        .buffer = std.ArrayList(u8).init(allocator),
        .col = 0,
        .index = 0,
        .line = 0,
        .reader = reader,
        .unread = null,
    };
}
