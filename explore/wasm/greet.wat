;; wat2wasm greet.wat && wasm2wat greet.wasm -o greet-out.wat && bun run wasmini.ts greet.wasm

(import "env" "print" (func $print (param $text i32)))

(memory (export "memory") 1)

(func (export "main")
  (call $print (i32.const 0))
  (call $print (i32.const 16))
)

(data (i32.const 0) "\09\00\00\00Hi there!")
(data (i32.const 16) "\0a\00\00\00Bye y'all!")
