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
    div,
    do,
    dot,
    else_key,
    end,
    eq,
    eq_eq,
    float,
    for_key,
    ge,
    geq,
    id: String,
    int,
    le,
    leq,
    mul,
    range,
    round_begin,
    round_end,
    string,
    struct_key,
    sub,
    to,
    use,
    when_key,
};

pub fn lex(allocator: Allocator, source: String) !ArrayList(Token) {
    _ = allocator;
    _ = source;
    _ = Token.float;
    var tokens = ArrayList(Token).init(allocator);
    try tokens.append(Token.float);
    try tokens.append(Token{ .id = "hi" });
    return tokens;
}
