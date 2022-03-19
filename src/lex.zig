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

pub const TokenKind = enum {
    comment,
    escape,
    escape_begin,
    escape_end,
    float,
    hspace,
    id,
    int,
    key_as,
    key_be,
    key_case,
    key_do,
    key_else,
    key_end,
    key_for,
    key_include,
    key_struct,
    key_to,
    key_use,
    key_when,
    op_add,
    op_colon,
    op_div,
    op_dot,
    op_eq,
    op_eqeq,
    op_ge,
    op_geq,
    op_le,
    op_leq,
    op_mul,
    op_sub,
    other,
    round_begin,
    round_end,
    string_begin_single,
    string_begin_double,
    string_end,
    string_text,
    vspace,
};

pub const TokenCategory = enum {
    content,
    id,
    key,
    op,
    other,
    space,
};

pub fn ofId(byte: u8) bool {
    return switch (byte) {
        '.', ',', ';', ' ', '\t', '\r', '\n', '(', ')', '[', ']', '{', '}', '#' => false,
        else => true,
    };
}

pub fn tokenKindCategory(kind: TokenKind) TokenCategory {
    return switch (kind) {
        .comment, .escape, .int, .string_text => .content,
        .id => .id,
        .key_as, .key_be, .key_case, .key_do, .key_else, .key_end, .key_for, .key_include, .key_struct, .key_to, .key_use, .key_when => .key,
        .escape_begin, .escape_end, .op_add, .op_colon, .op_div, .op_dot, .op_eq, .op_eqeq, .op_ge, .op_geq, .op_le, .op_leq, .op_mul, .op_sub, .round_begin, .round_end, .string_begin, .string_end => .op,
        .other => .other,
        .hspace, .vspace => .space,
    };
}

// pub fn shouldInternText(kind: TokenKind, text: []const u8) bool {
//     return switch (tokenKindCategory(kind)) {
//         .content, .space => text.len < 16,
//         .id, .key, .op => true,
//         .other => false,
//     };
// }

// pub const Token = struct {
//     kind: TokenKind,
//     begin: Index,
// };

pub fn last(comptime Item: type, items: []const Item) ?Item {
    return if (items.len > 0) items[items.len - 1] else null;
}

pub fn Lexer(comptime Reader: type) type {
    return struct {
        col: Index,
        index: Index,
        line: Index,
        reader: Reader,
        stack: ArrayList(TokenKind),
        text: ?*ArrayList(u8),
        unread: ?u8,

        const Self = @This();

        pub fn init(allocator: Allocator, reader: anytype, text: ?*std.ArrayList(u8)) Self {
            return .{
                .col = 0,
                .index = 0,
                .line = 0,
                .reader = reader,
                .stack = ArrayList(TokenKind).init(allocator),
                .text = text,
                .unread = null,
            };
        }

        pub fn deinit(self: Self) void {
            self.stack.deinit();
        }

        pub fn next(self: *Self) !?TokenKind {
            const mode = last(TokenKind, self.stack.items) orelse .round_begin;
            return switch (mode) {
                .string_begin_double => self.modeNextString('"'),
                .string_begin_single => self.modeNextString('\''),
                else => self.modeNextCode(),
            } catch |err| switch (err) {
                error.EndOfStream => null,
                else => |e| return e,
            };
        }

        fn advance(self: *Self) !?u8 {
            const byte = try self.readByte();
            if (self.text) |text| {
                try text.append(byte);
            }
            self.index += 1;
            if (byte == '\n') {
                self.col = 0;
                self.line += 1;
            }
            return try self.peekByteOrNull();
        }

        fn modeNextCode(self: *Self) !TokenKind {
            return switch (try self.peekByte()) {
                // TODO Unicode ids.
                'A'...'Z', 'a'...'z', '_' => self.nextId(),
                '0'...'9' => self.nextNumber(),
                '(' => self.nextRoundBegin(),
                ')' => self.nextRoundEnd(),
                ':' => self.nextSingle(.op_colon),
                '.' => self.nextSingle(.op_dot),
                '=' => self.nextSingle(.op_eq),
                '#' => self.nextComment(),
                ' ', '\t' => self.nextHspace(),
                '\r', '\n' => self.nextVspace(),
                '"' => self.nextStringBegin(.string_begin_double),
                '\'' => self.nextStringBegin(.string_begin_single),
                else => self.nextSingle(.other),
            };
        }

        fn modeNextString(self: *Self, comptime end: u8) !TokenKind {
            // TODO Handle newlines
            return switch (try self.peekByte()) {
                '\\' => self.nextEscape(),
                end => self.nextStringEnd(),
                else => self.nextStringText(end),
            };
        }

        fn nextComment(self: *Self) !TokenKind {
            while (true) {
                switch ((try self.advance()) orelse break) {
                    '\r', '\n' => break,
                    else => {},
                }
            }
            return .comment;
        }

        fn nextEscape(self: *Self) !TokenKind {
            switch ((try self.advance()) orelse return TokenKind.escape) {
                '(' => {
                    _ = try self.advance();
                    try self.stack.append(TokenKind.round_begin);
                    return .escape_begin;
                },
                // TODO Unicode escapes.
                // 'u' => ... expect parens with only int inside.
                else => {
                    _ = try self.advance();
                    return .escape;
                },
            }
        }

        fn nextHspace(self: *Self) !TokenKind {
            while (true) {
                switch ((try self.advance()) orelse break) {
                    ' ', '\t' => {},
                    else => break,
                }
            }
            return .hspace;
        }

        fn nextId(self: *Self) !TokenKind {
            while (true) {
                const byte = try self.advance();
                if (byte == null or !ofId(byte.?)) {
                    break;
                }
            }
            return .id;
        }

        fn nextNumber(self: *Self) !TokenKind {
            while (true) {
                switch ((try self.advance()) orelse break) {
                    '0'...'9' => {},
                    // 'e' => self.nextNumberExp(),
                    // 'x' => self.nextNumberHex(),
                    else => break,
                }
            }
            return .int;
        }

        fn nextRoundBegin(self: *Self) !TokenKind {
            _ = try self.advance();
            if (self.stack.items.len > 0) {
                try self.stack.append(TokenKind.round_begin);
            }
            return .round_begin;
        }

        fn nextRoundEnd(self: *Self) !TokenKind {
            _ = try self.advance();
            if (last(TokenKind, self.stack.items)) |mode| {
                if (mode == TokenKind.round_begin) {
                    self.stack.items.len -= 1;
                    if (last(TokenKind, self.stack.items)) |old| {
                        switch (old) {
                            .string_begin_double, .string_begin_single => return .escape_end,
                            else => {},
                        }
                    }
                }
            }
            return .round_end;
        }

        fn nextSingle(self: *Self, kind: TokenKind) !TokenKind {
            _ = try self.advance();
            return kind;
        }

        fn nextStringBegin(self: *Self, mode: TokenKind) !TokenKind {
            _ = try self.advance();
            try self.stack.append(mode);
            return mode;
        }

        fn nextStringEnd(self: *Self) !TokenKind {
            _ = try self.advance();
            self.stack.items.len -= 1;
            return .string_end;
        }

        fn nextStringText(self: *Self, comptime end: u8) !TokenKind {
            while (true) {
                switch ((try self.advance()) orelse break) {
                    end, '\\', '\r', '\n' => break,
                    else => {},
                }
            }
            return .string_text;
        }

        fn nextVspace(self: *Self) !TokenKind {
            // Make vspace single for easier counting of lines later.
            switch (try self.peekByte()) {
                '\r' => {
                    const byte = try self.advance();
                    if (byte == null or byte.? != '\n') {
                        return .vspace;
                    }
                },
                else => {},
            }
            _ = try self.advance();
            return .vspace;
        }

        fn peekByte(self: *Self) !u8 {
            if (self.unread == null) {
                self.unread = try self.reader.readByte();
            }
            return self.unread.?;
        }

        fn peekByteOrNull(self: *Self) !?u8 {
            return self.peekByte() catch |err| switch (err) {
                error.EndOfStream => null,
                else => |e| return e,
            };
        }

        fn readByte(self: *Self) !u8 {
            if (self.unread) |unread| {
                self.unread = null;
                return unread;
            }
            return try self.reader.readByte();
        }
    };
}
