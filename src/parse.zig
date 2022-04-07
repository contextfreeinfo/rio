const idx = @import("./idx.zig");
const intern = @import("./intern.zig");
const lex = @import("./lex.zig");
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const NodeKind = enum {
    assign,
    assign_to,
    be,
    block,
    call,
    colon,
    comment,
    def,
    dot,
    end,
    escape,
    fun,
    frac,
    leaf,
    of,
    question,
    round,
    space,
    string,
};

pub const NodeId = idx.Idx(u32, Node);
pub const NodeSlice = idx.IdxSlice(NodeId);

pub const Node = struct {
    kind: NodeKind,
    data: union {
        // Both of these are pairs of u32 values.
        kids: NodeSlice,
        token: lex.Token,
    },

    const Self = @This();

    pub fn print(self: Self, writer: anytype, context: TreePrintContext) std.os.WriteError!u32 {
        var count = @as(u32, 0);
        try printIndent(writer, context.indent);
        try writer.print("{}", .{self.kind});
        switch (self.kind) {
            .leaf => {
                count += 1;
                try writer.print(": {} {s}\n", .{ self.data.token.kind, context.config.pool.get(self.data.token.text) });
            },
            else => {
                try writer.print("\n", .{});
                var nested = context;
                nested.indent += 2;
                for (self.data.kids.from(context.nodes)) |node| {
                    count += try node.print(writer, nested);
                }
                if (count > 1) {
                    try printIndent(writer, context.indent);
                    try writer.print("/{}\n", .{self.kind});
                }
            },
        }
        return count;
    }

    fn printIndent(writer: anytype, indent: u16) !void {
        var i = @as(@TypeOf(indent), 0);
        while (i < indent) : (i += 1) {
            try writer.print(" ", .{});
        }
    }
};

pub fn adjustKids(nodes: []Node, new_begin: u32, offset: u32) void {
    for (nodes) |*node| {
        switch (node.kind) {
            .leaf => {},
            else => if (node.data.kids.idx.i >= new_begin + offset) {
                node.data.kids.idx.i -= offset;
            },
        }
    }
}

pub const TreePrintConfig = struct {
    pool: intern.Pool,
};

const TreePrintContext = struct {
    config: TreePrintConfig,
    indent: u16 = 0,
    nodes: []Node,
};

pub const Tree = struct {
    nodes: []Node,

    const Self = @This();

    pub fn deinit(self: Self, allocator: Allocator) void {
        allocator.free(self.nodes);
    }

    pub fn root(self: Self) ?Node {
        return if (self.nodes.len > 0) self.nodes[self.nodes.len - 1] else null;
    }

    pub fn print(self: Self, writer: anytype, config: TreePrintConfig) !void {
        if (self.root()) |r| {
            _ = try r.print(writer, .{ .nodes = self.nodes, .config = config, .indent = 0 });
        } else {
            try writer.print("()\n", .{});
        }
    }
};

pub const ParseError = error{OutOfMemory} || std.os.ReadError;

pub fn Parser(comptime Reader: type) type {
    return struct {
        lexer: lex.Lexer(Reader),
        line_begins: std.ArrayList(NodeId),
        nodes: std.ArrayList(Node),
        unread: ?lex.Token,
        working: std.ArrayList(Node),

        const Self = @This();

        pub fn init(allocator: Allocator, pool: ?*intern.Pool) !Self {
            return Self{
                .lexer = try lex.Lexer(Reader).init(allocator, pool),
                .line_begins = std.ArrayList(NodeId).init(allocator),
                .nodes = std.ArrayList(Node).init(allocator),
                .unread = null,
                .working = std.ArrayList(Node).init(allocator),
            };
        }

        pub fn deinit(self: *Self) void {
            self.lexer.deinit();
            self.nodes.deinit();
            self.working.deinit();
        }

        pub fn parse(self: *Self, reader: Reader) !Tree {
            self.lexer.start(reader);
            self.nodes.clearRetainingCapacity();
            self.working.clearRetainingCapacity();
            var bonus = false;
            while (true) {
                try self.block();
                switch ((try self.peek()).kind) {
                    .eof => break,
                    else => {
                        // Some extra `end` involved.
                        bonus = true;
                        try self.advance();
                    },
                }
            }
            if (bonus) {
                // Gather multiple top levels into a bonus block.
                try self.nest(.block, NodeId.of(0));
            }
            // This one doesn't ever appear but should push down the block if any.
            try self.nest(.block, NodeId.of(0));
            // Retain the buffer on the idea we might need that much again.
            return Tree{ .nodes = (try self.nodes.clone()).items };
        }

        fn advance(self: *Self) !void {
            const token = try self.readToken();
            if (token.kind != .eof) {
                try self.working.append(.{ .kind = .leaf, .data = .{ .token = token } });
            }
        }

        fn assign(self: *Self) !void {
            // TODO Should be right associative.
            try self.infix(.assign, .op_eq, call);
        }

        fn assign_to(self: *Self) !void {
            try self.infix(.assign_to, .op_eqto, assign);
        }

        fn atom(self: *Self) !void {
            switch ((try self.peek()).kind) {
                .eof, .escape_end, .key_end, .round_end => {},
                .key_be => try self.blocker(.be),
                .key_of => try self.blocker(.of),
                // .key_for => try self.fun(),
                .round_begin => try self.round(),
                .string_begin_single, .string_begin_double => try self.string(),
                else => try self.advance(),
            }
        }

        fn block(self: *Self) !void {
            try self.block_until(check_block_end);
        }

        fn block_until(self: *Self, check_end: fn (self: *Self, kind: lex.TokenKind) ParseError!bool) !void {
            const begin = self.here();
            while (true) {
                try self.space();
                const check = (try self.peek()).kind;
                switch (check) {
                    .eof => break,
                    else => if (try check_end(self, check)) {
                        break;
                    },
                }
                try self.line();
            }
            try self.nestMaybe(.block, begin);
        }

        fn blocker(self: *Self, kind: NodeKind) !void {
            const begin = self.here();
            try self.advance();
            try self.hspace();
            switch ((try self.peek()).kind) {
                .vspace => {
                    try self.block();
                    if ((try self.peek()).kind == .key_end) {
                        try self.end();
                    }
                },
                else => try self.call(), // TODO Infix ops instead.
            }
            try self.nest(kind, begin);
        }

        fn call(self: *Self) !void {
            const begin = self.here();
            var count = @as(u32, 0);
            while (true) : (count += 1) {
                try self.colon();
                try self.hspace();
                switch ((try self.peek()).kind) {
                    .eof, .escape_end, .key_end, .op_eq, .op_eqto, .round_end, .vspace => break,
                    else => {},
                }
            }
            if (count > 0) {
                const first = begin.from(self.working.items);
                const wrap_question = first.kind == .question;
                if (wrap_question) {
                    // Bring out the wrapped kids to working space and adjust.
                    // Overwrite the question with the question's kid.
                    self.working.items[begin.i] = first.data.kids.idx.from(self.nodes.items);
                    const kid_begin = first.data.kids.idx.i;
                    const kid_count = first.data.kids.len;
                    if (kid_count > 1) {
                        // If more than one kid, insert the rest after.
                        const kids = NodeId.of(kid_begin + 1).slice(kid_count - 1);
                        try self.working.insertSlice(begin.i + 1, kids.from(self.nodes.items));
                    }
                    // Remove these kids from the tree.
                    std.mem.copy(
                        Node,
                        self.nodes.items[kid_begin .. self.nodes.items.len - kid_count],
                        self.nodes.items[kid_begin + kid_count ..],
                    );
                    self.nodes.items.len -= kid_count;
                    // Move the pointing back further in the tree, too.
                    adjustKids(self.nodes.items[kid_begin..], kid_begin, kid_count);
                    adjustKids(self.working.items[begin.i..], kid_begin, kid_count);
                }
                try self.nest(.call, begin);
                if (wrap_question) {
                    // Wrap the call in a question.
                    try self.nest(.question, begin);
                }
            }
        }

        fn check_block_end(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // TODO Get fancier?
            _ = self;
            return kind == .key_end;
        }

        fn check_escape_end(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // Nothing much fancier we can do here with nested lexing.
            _ = self;
            return kind == .escape_end;
        }

        fn check_round_end(self: *Self, kind: lex.TokenKind) ParseError!bool {
            // TODO Get fancier?
            _ = self;
            return kind == .round_end;
        }

        fn colon(self: *Self) ParseError!void {
            const begin = self.here();
            try self.question();
            // Line won't repeat colons, since they'll be consumed internally, but eh.
            try self.infixFinish(.colon, .op_colon, if (begin.is(self.lineBegin())) line else question, begin);
        }

        fn dot(self: *Self) !void {
            try self.infix(.dot, .op_dot, atom);
        }

        fn end(self: *Self) !void {
            const begin = self.here();
            try self.advance();
            try self.hspace();
            switch (lex.tokenKindCategory((try self.peek()).kind)) {
                // TODO Use for multipop???
                .id, .key => try self.advance(),
                else => {},
            }
            try self.nest(.end, begin);
        }

        fn escape(self: *Self) !void {
            const begin = self.here();
            try self.advance();
            try self.block_until(check_escape_end);
            if ((try self.peek()).kind == .escape_end) {
                try self.advance();
            }
            try self.nest(.escape, begin);
        }

        // fn fun(self: *Self) !void {
        //     const begin = self.here();
        //     try self.fun_part();
        //     try self.nest(.fun, begin);
        // }

        // fn fun_for(self: *Self) !void {
        // }

        // fn fun_part(self: *Self) !void {
        //     switch ((try self.peek()).kind) {
        //         .key_for => try self.fun_for(),
        //         else => {},
        //     }
        // }

        fn here(self: Self) NodeId {
            return NodeId.of(self.working.items.len);
        }

        fn hspace(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch ((try self.peek()).kind) {
                    .comment, .hspace => try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        // TODO This nests left, but normal assign (if nesting is allowed) should nest right. (And assign_to should nest left.)
        fn infix(self: *Self, kind: NodeKind, op: lex.TokenKind, kid: fn (self: *Self) ParseError!void) !void {
            const begin = self.here();
            try kid(self);
            try self.infixFinish(kind, op, kid, begin);
        }

        // TODO This nests left, but normal assign (if nesting is allowed) should nest right.
        fn infixFinish(self: *Self, kind: NodeKind, op: lex.TokenKind, kid: fn (self: *Self) ParseError!void, begin: NodeId) !void {
            try self.hspace();
            while (true) {
                if ((try self.peek()).kind != op) break;
                try self.advance();
                try self.space();
                switch ((try self.peek()).kind) { // or any end
                    .eof, .escape_end, .key_end, .round_end => {},
                    else => try kid(self),
                }
                try self.nest(kind, begin);
            }
        }

        fn line(self: *Self) !void {
            try self.line_begins.append(self.here());
            try self.assign_to();
            defer self.line_begins.items.len -= 1;
        }

        fn lineBegin(self: *Self) NodeId {
            return if (self.line_begins.items.len > 0) self.line_begins.items[self.line_begins.items.len - 1] else NodeId.of(0);
        }

        fn nest(self: *Self, kind: NodeKind, begin: NodeId) !void {
            // Move the working nodes into permanent.
            const nodes_begin = NodeId.of(self.nodes.items.len);
            try self.nodes.appendSlice(begin.til(NodeId.of(self.working.items.len)).from(self.working.items));
            self.working.items.len = begin.i;
            // Make a new working node.
            const parent = Node{
                .kind = kind,
                .data = .{ .kids = nodes_begin.til(NodeId.of(self.nodes.items.len)) },
            };
            try self.working.append(parent);
        }

        fn nestMaybe(self: *Self, kind: NodeKind, begin: NodeId) !void {
            if (self.here().i > begin.i) {
                try self.nest(kind, begin);
            }
        }

        fn peek(self: *Self) !lex.Token {
            if (self.unread == null) {
                self.unread = try self.lexer.next();
            }
            return self.unread.?;
        }

        fn question(self: *Self) !void { // suffix -- any others?
            const begin = self.here();
            try self.dot();
            try self.hspace();
            while (true) {
                if ((try self.peek()).kind != .op_question) break;
                try self.advance();
                // The goal here is to keep questions outside of dots.
                try self.nest(.question, begin);
                // But to let dots proceed still.
                try self.infixFinish(.dot, .op_dot, atom, begin);
            }
        }

        fn readToken(self: *Self) !lex.Token {
            if (self.unread) |unread| {
                self.unread = null;
                return unread;
            }
            return try self.lexer.next();
        }

        fn round(self: *Self) !void {
            const begin = self.here();
            try self.advance();
            try self.block_until(check_round_end);
            if ((try self.peek()).kind == .round_end) {
                try self.advance();
            }
            try self.nest(.round, begin);
        }

        fn space(self: *Self) !void {
            const begin = self.here();
            while (true) {
                switch ((try self.peek()).kind) {
                    .comment, .hspace, .vspace => try self.advance(),
                    else => break,
                }
            }
            try self.nestMaybe(.space, begin);
        }

        fn string(self: *Self) !void {
            const begin = self.here();
            try self.advance();
            while (true) {
                switch ((try self.peek()).kind) {
                    .eof, .string_end => break,
                    .escape_begin => try self.escape(),
                    else => try self.advance(),
                }
            }
            try self.advance();
            try self.nest(.string, begin);
        }
    };
}
