;; wat2wasm fib.wat && wasm2wat -v fib.wasm -o fib-out.wat 2> fib-out.txt

(memory 1)

(func $fib (param $n i32) (result i32)
  (local $result i32)
  (i32.le_s (local.get $n) (i32.const 0))
  if
    (local.set $result (i32.const 0))
  else
    (i32.eq (local.get $n) (i32.const 1))
    if
      (local.set $result (i32.const 1))
    else
      (local.set $result
        (i32.add
          (call $fib (i32.sub (local.get $n) (i32.const 2)))
          (call $fib (i32.sub (local.get $n) (i32.const 1)))
        )
      )
    end
  end
  local.get $result
)
