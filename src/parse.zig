const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

const NodeId = u32;
const TextId = intern.Index;

const NodeKind = enum {
    comment,
    def,
    fun,
    frac,
    leaf_int,
    leaf_ref,
};

const Node = struct {
    kind: NodeKind,
    data: union {
        kids: []NodeId,
        text: TextId,
    },
};

const Tree = struct {
    nodes: []Node,
    root: Node,
};

pub fn Parser(comptime Reader: type) type {
    _ = Reader;
    return struct {
        lexer: lex.Lexer(Reader),
        nodes: std.ArrayList(Node),
        working: std.ArrayList(NodeId),

        const Self = @This();

        pub fn init(allocator: Allocator, pool: ?*intern.Pool) !Self {
            return Self{
                .lexer = try lex.Lexer(Reader).init(allocator, pool),
                .nodes = std.ArrayList(Node).init(allocator),
                .working = std.ArrayList(NodeId).init(allocator),
            };
        }

        pub fn deinit(self: *Self) void {
            self.lexer.deinit();
            self.nodes.deinit();
            self.working.deinit();
        }

        pub fn parse(self: Self, reader: Reader) !Tree {
            self.lexer.start(reader);
            return Tree{};
        }
    };
}
