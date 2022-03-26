pub fn Dex(comptime Index: type, comptime Value: type) type {
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

pub fn DexSlice(comptime DexType: type) type {
    return struct {
        ptr: DexType,
        // TODO Option for smaller len?
        len: DexType.Index,

        pub fn of(ptr: DexType, len: usize) @This() {
            return .{ .ptr = ptr, .len = DexType.of(len).i };
        }
    };
}
