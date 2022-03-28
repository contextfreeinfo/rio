pub fn Idx(comptime Index: type, comptime Value: type) type {
    return struct {
        i: Index,

        pub const Index = Index;
        pub const Value = Value;

        const Self = @This();

        pub fn of(i: usize) Self {
            // TODO Check bounds?
            return .{ .i = @intCast(Index, i) };
        }

        pub fn from(self: Self, values: []const Value) Value {
            // TODO Check bounds?
            return values[self.i];
        }

        pub fn slice(self: Self, l: usize) IdxSlice(Self) {
            return IdxSlice(Self).of(self, l);
        }

        pub fn til(self: Self, end: Self) IdxSlice(Self) {
            return self.slice(end.i - self.i);
        }
    };
}

pub fn IdxSlice(comptime IdxType: type) type {
    return IdxLenSlice(IdxType, IdxType.Index);
}

pub fn IdxLenSlice(comptime IdxType: type, comptime Len: type) type {
    return struct {
        idx: IdxType,
        len: Len,

        const Self = @This();
        const Value = IdxType.Value;

        pub fn of(idx: IdxType, len: usize) Self {
            return .{ .idx = idx, .len = @intCast(Len, len) };
        }

        pub fn from(self: Self, values: []const Value) []const Value {
            // TODO Check bounds?
            return values[self.idx.i .. self.idx.i + self.len];
        }
    };
}
