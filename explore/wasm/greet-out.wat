(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func))
  (import "env" "print" (func (;0;) (type 0)))
  (func (;1;) (type 1)
    i32.const 0
    call 0
    i32.const 16
    call 0)
  (memory (;0;) 1)
  (export "memory" (memory 0))
  (export "main" (func 1))
  (data (;0;) (i32.const 0) "\09\00\00\00Hi there!")
  (data (;1;) (i32.const 16) "\0a\00\00\00Bye y'all!"))
