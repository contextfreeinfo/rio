const std = @import("std");
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;
const String = []const u8;

pub const Token = union(enum) {
    add,
    as,
    be,
    case,
    colon,
    comment: String,
    div,
    do,
    dot,
    else_key,
    end,
    eq,
    eq_eq,
    float: String,
    for_key,
    ge,
    geq,
    id: String,
    int: String,
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

pub fn lex(allocator: Allocator, reader: anytype) !void {
    _ = allocator;
    var n = @as(u32, 0);
    while (true) {
        const c = reader.readByte() catch |err| switch (err) {
            error.EndOfStream => break,
            else => |e| return e,
        };
        _ = c;
        n += 1;
    }
    std.debug.print("Read: {}\n", .{n});
    // _ = Token.float;
    // var tokens = ArrayList(Token).init(allocator);
    // try tokens.append(Token.dot);
    // // TODO Allocate string data in one buffer and tokens in another.
    // // TODO Use u32 indices into text buffer.
    // try tokens.append(Token{ .id = "hi" });
    // return tokens;
}
