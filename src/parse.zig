const NodeKind = enum {
    comment,
    def,
    fun,
    int,
    frac,
    ref,
};

const Node = struct {
    kind: NodeKind,
};

pub fn Parser(comptime Reader: type) type {
    _ = Reader;
    return struct {
        const Self = @This();

        pub fn init() !Self {
            return Self{};
        }

        pub fn deinit(self: *Self) void {
            _ = self;
        }
    };
}
