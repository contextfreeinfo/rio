(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func (param i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func (;1;) (type 1)
    i32.const 9
    call 2
    i32.const 1024
    call 3
    i32.const 1032
    call 3
    i32.const 1040
    call 3
    drop)
  (func (;2;) (type 2) (param i32) (result i32)
    (local i32)
    local.get 0
    i32.const 0
    i32.le_s
    if  ;; label = @1
      i32.const 0
      local.set 1
    else
      local.get 0
      i32.const 1
      i32.eq
      if (result i32)  ;; label = @2
        i32.const 1
      else
        local.get 0
        i32.const 2
        i32.sub
        call 2
        local.get 0
        i32.const 1
        i32.sub
        call 2
        i32.add
      end
      local.set 1
    end
    local.get 1)
  (func (;3;) (type 3) (param i32)
    (local i32 i32)
    global.get 0
    i32.const 8
    i32.sub
    local.tee 1
    global.set 0
    global.get 0
    i32.const 4
    i32.sub
    local.tee 2
    global.set 0
    local.get 1
    local.get 0
    i32.const 4
    i32.add
    i32.store
    local.get 1
    i32.const 4
    i32.add
    local.get 0
    i32.load
    i32.store
    i32.const 1
    local.get 1
    i32.const 1
    local.get 2
    call 0
    drop
    global.get 0
    i32.const 12
    i32.add
    global.set 0)
  (memory (;0;) 1)
  (global (;0;) (mut i32) (i32.const 1024))
  (export "memory" (memory 0))
  (export "_start" (func 1))
  (data (;0;) (i32.const 1024) "\04\00\00\00fib(")
  (data (;1;) (i32.const 1032) "\04\00\00\00) = ")
  (data (;2;) (i32.const 1040) "\01\00\00\00\0a\00\00\00"))
