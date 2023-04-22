// zig run fib.zig
// zig build-exe fib.zig -target wasm32-wasi -O ReleaseSmall
// wasm-opt -O4 -all fib.wasm -o fib.opt.wasm
// wasm2wat fib.opt.wasm -o fib-out.opt.wat
// zig build-lib fib.zig -target wasm32-freestanding -dynamic
// zig build-lib fib.zig -target wasm32-freestanding -dynamic -O ReleaseSmall
// wasm2wat fib.wasm -o fib-out.wat

const print = @import("std").debug.print;

pub fn main() void {
    for ([_]u0{0} ** 10) |_, n| {
        print("fib({}) = {}\n", .{ n, fib(@intCast(i32, n)) });
    }
}

export fn fib(n: i32) i32 {
    return if (n <= 0)
        0
    else if (n == 1)
        1
    else
        fib(n - 2) + fib(n - 1);
}
