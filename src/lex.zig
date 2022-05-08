const intern = @import("./intern.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;
const String = []const u8;

// Limit size even on 64-bit systems.
const Index = intern.Index;
const TextId = intern.TextId;

pub const TokenKind = enum {
    comment,
    eof,
    escape,
    escape_begin,
    escape_end,
    frac,
    hspace,
    id,
    int,
    // Some keys, like `add`, are for convenient recognition and handling,
    key_add,
    key_and,
    key_as,
    key_be,
    key_blank,
    key_case,
    key_div,
    key_do,
    key_else,
    key_end,
    key_eq,
    key_for,
    key_ge,
    key_gt,
    key_include,
    key_le,
    key_lt,
    key_mul,
    key_ne,
    key_of,
    key_or,
    key_struct,
    key_sub,
    key_to,
    key_try,
    key_use,
    key_with,
    op_add,
    op_colon,
    op_comma,
    op_div,
    op_dot,
    op_eq,
    op_eqeq,
    op_eqto,
    op_ge,
    op_gt,
    op_le,
    op_lt,
    op_mul,
    op_ne,
    op_question,
    op_spread,
    op_sub,
    other,
    round_begin,
    round_end,
    string_begin_double,
    string_begin_single,
    string_end,
    string_text,
    vspace,
};

pub const token_key_first = TokenKind.key_and;
pub const token_key_last = TokenKind.key_with;

pub const TokenCategory = enum {
    content,
    eof,
    id,
    key,
    op,
    other,
    space,
};

pub fn ofId(byte: u8) bool {
    return switch (byte) {
        '.', ',', ';', ':', '?', ' ', '\t', '\r', '\n', '(', ')', '[', ']', '{', '}', '#' => false,
        else => true,
    };
}

pub fn tokenKindCategory(kind: TokenKind) TokenCategory {
    return switch (kind) {
        .comment, .escape, .frac, .int, .string_text => .content,
        .eof => .eof,
        .id => .id,
        .key_add, .key_and, .key_as, .key_be, .key_blank, .key_case, .key_div, .key_do, .key_else, .key_end, .key_eq, .key_for, .key_ge, .key_gt, .key_include, .key_le, .key_lt, .key_mul, .key_ne, .key_of, .key_or, .key_struct, .key_sub, .key_to, .key_try, .key_use, .key_with => .key,
        .escape_begin, .escape_end, .op_add, .op_colon, .op_comma, .op_div, .op_dot, .op_eq, .op_eqeq, .op_eqto, .op_ge, .op_gt, .op_le, .op_lt, .op_mul, .op_ne, .op_question, .op_spread, .op_sub, .round_begin, .round_end, .string_begin_double, .string_begin_single, .string_end => .op,
        .other => .other,
        .hspace, .vspace => .space,
    };
}

// For keywords and operators, this gives actual string values.
// For others, it might just be an approximation.
pub fn tokenText(kind: TokenKind) []const u8 {
    return switch (kind) {
        .comment => "#",
        .eof => "eof",
        .escape => "\\_",
        .escape_begin => "\\(",
        .escape_end => ")",
        .frac => "._",
        .hspace => " ",
        .id => "_",
        .int => "_",
        .key_add => "add",
        .key_and => "and",
        .key_as => "as",
        .key_be => "be",
        .key_blank => "_",
        .key_case => "case",
        .key_div => "div",
        .key_do => "do",
        .key_else => "else",
        .key_end => "end",
        .key_eq => "eq",
        .key_for => "for",
        .key_ge => "ge",
        .key_gt => "gt",
        .key_include => "include",
        .key_le => "le",
        .key_lt => "lt",
        .key_mul => "mul",
        .key_ne => "ne",
        .key_of => "of",
        .key_or => "or",
        .key_struct => "struct",
        .key_sub => "sub",
        .key_to => "to",
        .key_try => "try",
        .key_use => "use",
        .key_with => "with",
        .op_add => "+",
        .op_colon => ":",
        .op_comma => ",",
        .op_div => "/",
        .op_dot => ".",
        .op_eq => "=",
        .op_eqeq => "==",
        .op_eqto => "=>",
        .op_ge => ">=",
        .op_gt => ">",
        .op_le => "<=",
        .op_lt => "<",
        .op_mul => "*",
        .op_ne => "!=",
        .op_question => "?",
        .op_spread => "..",
        .op_sub => "-",
        .other => "_",
        .round_begin => "(",
        .round_end => ")",
        .string_begin_double => "\"",
        .string_begin_single => "'",
        .string_end => "[\"']",
        .string_text => "...",
        .vspace => "\n",
    };
}

pub fn tokenTextSure(token: Token, pool: intern.Pool) []const u8 {
    return if (token.text.i == 0) tokenText(token.kind) else pool.get(token.text);
}

// TODO Replace this with a trie or even a generated or hardcoded switch tree.
// TODO This is just here now for convenience.
fn initKeys(allocator: Allocator) !std.StringHashMap(TokenKind) {
    var keys = std.StringHashMap(TokenKind).init(allocator);
    // "specifically, it's a slice of https://github.com/ziglang/zig/blob/master/lib/std/builtin.zig#L318 these"
    // "you can use std.meta.fields(Enum) to get a slice that holds all of the fields in the enum" - random internet person on Zig Discord
    var int = @enumToInt(token_key_first);
    while (int <= @enumToInt(token_key_last)) : (int += 1) {
        const kind = @intToEnum(TokenKind, int);
        try keys.put(tokenText(kind), kind);
    }
    return keys;
}

pub const Token = struct {
    kind: TokenKind,
    text: TextId,
};

pub fn last(comptime Item: type, items: []const Item) ?Item {
    return if (items.len > 0) items[items.len - 1] else null;
}

pub fn lastPtr(comptime Item: type, items: []Item) ?*Item {
    return if (items.len > 0) &items[items.len - 1] else null;
}

pub fn Lexer(comptime Reader: type) type {
    return struct {
        keys: std.StringHashMap(TokenKind),
        pool: ?*intern.Pool,
        reader: ?Reader,
        stack: std.ArrayList(TokenKind),
        text: std.ArrayList(u8),
        unread: ?u8,

        const Self = @This();

        pub fn init(allocator: Allocator, pool: ?*intern.Pool) !Self {
            return Self{
                .keys = try initKeys(allocator),
                .pool = pool,
                .reader = null,
                .stack = std.ArrayList(TokenKind).init(allocator),
                .text = std.ArrayList(u8).init(allocator),
                .unread = null,
            };
        }

        pub fn deinit(self: *Self) void {
            self.keys.deinit();
            self.stack.deinit();
            self.text.deinit();
        }

        pub fn start(self: *Self, reader: Reader) void {
            self.reader = reader;
            self.stack.clearRetainingCapacity();
            self.text.clearRetainingCapacity();
            self.unread = null;
        }

        pub fn next(self: *Self) !Token {
            self.text.clearRetainingCapacity();
            const mode = last(TokenKind, self.stack.items) orelse .round_begin;
            const kind = switch (mode) {
                .string_begin_double => self.modeNextString('"'),
                .string_begin_single => self.modeNextString('\''),
                else => self.modeNextCode(),
            } catch |err| switch (err) {
                error.EndOfStream => return Token{ .kind = .eof, .text = TextId.of(0) },
                else => |e| return e,
            };
            const text = if (self.pool) |pool| try pool.intern(self.text.items) else TextId.of(self.text.items.len);
            return Token{ .kind = kind, .text = text };
        }

        fn advance(self: *Self) !?u8 {
            const byte = try self.readByte();
            try self.text.append(byte);
            return try self.peekByteOrNull();
        }

        fn modeNextCode(self: *Self) !TokenKind {
            return switch (try self.peekByte()) {
                'A'...'Z', 'a'...'z', '_' => self.nextId(),
                '0'...'9' => self.nextNumber(),
                '(' => self.nextRoundBegin(),
                ')' => self.nextRoundEnd(),
                ':' => self.nextSingle(.op_colon),
                '.' => self.nextDot(),
                ',' => self.nextSingle(.op_comma),
                '?' => self.nextSingle(.op_question),
                '!' => self.nextBang(),
                '=' => self.nextEq(),
                '<' => self.nextLt(),
                '>' => self.nextGt(),
                '+' => self.nextSingle(.op_add),
                '-' => self.nextSingle(.op_sub),
                '*' => self.nextSingle(.op_mul),
                '/' => self.nextSingle(.op_div),
                '#' => self.nextComment(),
                ' ', '\t' => self.nextHspace(),
                '\r', '\n' => self.nextVspace(),
                '"' => self.nextStringBegin(.string_begin_double),
                '\'' => self.nextStringBegin(.string_begin_single),
                else => self.nextSingle(.other),
            };
        }

        fn modeNextString(self: *Self, comptime end: u8) !TokenKind {
            return switch (try self.peekByte()) {
                '\\' => self.nextEscape(),
                '\r', '\n' => self.nextStringEndEmpty(),
                end => self.nextStringEnd(),
                else => self.nextStringText(end),
            };
        }

        fn nextBang(self: *Self) !TokenKind {
            const result: TokenKind = switch ((try self.advance()) orelse return .other) {
                '=' => .op_ne,
                else => return .other,
            };
            _ = try self.advance();
            return result;
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

        fn nextDot(self: *Self) !TokenKind {
            const result: TokenKind = switch ((try self.advance()) orelse return .op_dot) {
                '.' => .op_spread,
                else => return .op_dot,
            };
            _ = try self.advance();
            return result;
        }

        fn nextEq(self: *Self) !TokenKind {
            const result: TokenKind = switch ((try self.advance()) orelse return .op_eq) {
                '=' => .op_eqeq,
                '>' => .op_eqto,
                else => return .op_eq,
            };
            _ = try self.advance();
            return result;
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

        fn nextGt(self: *Self) !TokenKind {
            const result: TokenKind = switch ((try self.advance()) orelse return .op_gt) {
                '=' => .op_ge,
                else => return .op_gt,
            };
            _ = try self.advance();
            return result;
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
            return self.keys.get(self.text.items) orelse .id;
        }

        fn nextLt(self: *Self) !TokenKind {
            const result: TokenKind = switch ((try self.advance()) orelse return .op_lt) {
                '=' => .op_le,
                else => return .op_lt,
            };
            _ = try self.advance();
            return result;
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

        fn nextStringEndEmpty(self: *Self) !TokenKind {
            self.stack.items.len -= 1;
            return self.nextVspace();
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
                self.unread = try self.reader.?.readByte();
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
            return try self.reader.?.readByte();
        }
    };
}
