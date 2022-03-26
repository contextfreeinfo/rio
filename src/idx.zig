pub fn Idx(comptime Index: type, comptime Value: type) type {
    return struct {
        i: Index,

        pub const Index = Index;
        pub const Value = Value;

        pub fn of(i: usize) @This() {
            // TODO Check bounds?
            return .{.i = @intCast(Index, i)};
        }

        pub fn from(self: @This(), values: []const Value) Value {
            // TODO Check bounds?
            return values[self.i];
        }

        pub fn slice(self: @This(), values: []const Value, len: Index) []const Value {
            // TODO Check bounds?
            return values[self.i .. self.i + len];
        }
    };
}

pub fn IdxSlice(comptime IdxType: type) type {
    return struct {
        idx: IdxType,
        // TODO Option for smaller len?
        len: IdxType.Index,

        pub fn of(idx: IdxType, len: usize) @This() {
            return .{ .idx = idx, .len = IdxType.of(len).i };
        }
    };
}
