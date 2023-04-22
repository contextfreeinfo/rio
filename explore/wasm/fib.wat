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

(global $stack-top (mut i32) (i32.const 1024))

(func $main (export "_start")
  (call $print (i32.const 1024))
  (call $print (i32.const 1032))
  (call $print-i32 (call $fib (i32.const 9)))
  (call $print (i32.const 1040))
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
  (local.set $iovec (call $stack-push (i32.const 8)))
  (local.set $nwritten (call $stack-push (i32.const 4)))
  (i32.store (local.get $iovec) (i32.add (local.get $text) (i32.const 4)))
  (i32.store
    (i32.add (local.get $iovec) (i32.const 4)) (i32.load (local.get $text))
  )
  (call $fd-write
    (i32.const 1) (local.get $iovec) (i32.const 1) (local.get $nwritten)
  )
  drop
  (call $stack-pop (i32.const 12))
)

(func $print-i32 (export "print-i32") (param $n i32)
  (local $neg i32)
  (local $text i32)
  (local $buffer i32)
  (local $cursor i32)
  ;; Need 4 bytes for size and up to 11 bytes for signed 32-bit in base 10.
  ;; Round up to 12 just nice alignment.
  (local.set $text (call $stack-push (i32.const 16)))
  (local.set $buffer (i32.add (local.get $text) (i32.const 4)))
  (local.set $cursor (local.get $buffer))
  ;; Put in a dash for negative.
  (local.tee $neg (i32.lt_s (local.get $n) (i32.const 0)))
  if
    (i32.store8 (local.get $cursor) (i32.const 0x2d))
    (local.set $cursor (i32.add (local.get $cursor) (i32.const 1)))
    (local.set $n (i32.sub (i32.const 0) (local.get $n)))
  end
  ;; Now start putting in the digits in reverse order because that's easier.
  loop $digits
    ;; Add the digit to ascii '0' 0x30.
    (i32.store8 (local.get $cursor)
      (i32.add (i32.rem_u (local.get $n) (i32.const 10)) (i32.const 0x30))
    )
    (local.set $cursor (i32.add (local.get $cursor) (i32.const 1)))
    ;; Divide by 10.
    (local.tee $n (i32.div_u (local.get $n) (i32.const 10)))
    ;; Break if done. We get at least one digit above.
    (i32.ne (i32.const 0))
    br_if $digits
  end
  ;; TODO Reverse digits!
  ;; Store size and print.
  (i32.store (local.get $text)
    (i32.sub (local.get $cursor) (local.get $buffer))
  )
  (call $print (local.get $text))
  (call $stack-pop (i32.const 16))
)

(func $stack-pop (param $n i32)
  (global.set $stack-top (i32.add (global.get $stack-top) (local.get $n)))
)

(func $stack-push (param $n i32) (result i32)
  (local $address i32)
  (global.set $stack-top
    (local.tee $address (i32.sub (global.get $stack-top) (local.get $n)))
  )
  local.get $address
)

(data $message0 (i32.const 1024) "\04\00\00\00fib(")
(data $message1 (i32.const 1032) "\04\00\00\00) = ")
(data $message2 (i32.const 1040) "\01\00\00\00\n\00\00\00")
