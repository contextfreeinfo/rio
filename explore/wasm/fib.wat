;; wat2wasm fib.wat && wasm2wat -v fib.wasm -o fib-out.wat 2> fib-out.txt && wasmer run fib.wasm

(import "wasi_snapshot_preview1" "fd_write" 
  (func $fd-write
    (param $fd i32) 
    (param $iovec i32)
    (param $len i32)
    (param $written i32)
    (result i32)))

(memory 1)
(export "memory" (memory 0))

(global $stacktop (mut i32) (i32.const 1024))

(func $main (export "_start")
  (call $fib (i32.const 9))
  (call $print (i32.const 1024))
  (call $print (i32.const 1032))
  (call $print (i32.const 1040))
  drop
)

(func $fib (param $n i32) (result i32)
  (local $result i32)
  (i32.le_s (local.get $n) (i32.const 0))
  if
    (local.set $result (i32.const 0))
  else
    (i32.eq (local.get $n) (i32.const 1))
    if (result i32)
      i32.const 1
    else
      (i32.add
        (call $fib (i32.sub (local.get $n) (i32.const 2)))
        (call $fib (i32.sub (local.get $n) (i32.const 1)))
      )
    end
    local.set $result
  end
  local.get $result
)

(func $print (param $text i32)
  (local $iovec i32)
  (local $nwritten i32)
  (global.set $stacktop
    (local.tee $iovec (i32.sub (global.get $stacktop) (i32.const 8)))
  )
  (global.set $stacktop
    (local.tee $nwritten (i32.sub (global.get $stacktop) (i32.const 4)))
  )
  (i32.store (local.get $iovec) (i32.add (local.get $text) (i32.const 4)))
  (i32.store
    (i32.add (local.get $iovec) (i32.const 4))
    (i32.load (local.get $text))
  )
  (call $fd-write
    (i32.const 1) (local.get $iovec) (i32.const 1) (local.get $nwritten)
  )
  drop
  (global.set $stacktop (i32.add (global.get $stacktop) (i32.const 12)))
)

(data $message0 (i32.const 1024) "\04\00\00\00fib(")
(data $message1 (i32.const 1032) "\04\00\00\00) = ")
(data $message2 (i32.const 1040) "\01\00\00\00\n\00\00\00")
