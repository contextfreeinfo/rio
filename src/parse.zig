const NodeKind = enum {
    comment,
    define,
    int,
    frac,
};

const Node = struct {
    kind: NodeKind,
};

pub fn Parser(comptime Reader: type) type {
    _ = Reader;
    return struct {
        //
    };
}
