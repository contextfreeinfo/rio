(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;2;) (func))
  (type (;3;) (func (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 1)))
  (func (;1;) (type 2)
    (local i32)
    loop  ;; label = @1
      i32.const 1024
      call 3
      local.get 0
      call 4
      i32.const 1032
      call 3
      local.get 0
      call 2
      call 4
      i32.const 1040
      call 3
      local.get 0
      i32.const 1
      i32.add
      local.tee 0
      i32.const 9
      i32.le_s
      br_if 0 (;@1;)
    end)
  (func (;2;) (type 3) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.le_s
    if (result i32)  ;; label = @1
      i32.const 0
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
    end)
  (func (;3;) (type 0) (param i32)
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
  (func (;4;) (type 0) (param i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 4
    i32.add
    local.tee 4
    local.set 1
    local.get 0
    i32.const 0
    i32.lt_s
    if  ;; label = @1
      local.get 1
      i32.const 45
      i32.store8
      local.get 1
      i32.const 1
      i32.add
      local.set 1
      i32.const 0
      local.get 0
      i32.sub
      local.set 0
    end
    local.get 0
    local.set 3
    loop  ;; label = @1
      local.get 1
      i32.const 1
      i32.add
      local.set 1
      local.get 3
      i32.const 10
      i32.div_u
      local.tee 3
      br_if 0 (;@1;)
    end
    local.get 2
    local.get 1
    local.get 4
    i32.sub
    i32.store
    loop  ;; label = @1
      local.get 1
      i32.const 1
      i32.sub
      local.tee 1
      local.get 0
      i32.const 10
      i32.rem_u
      i32.const 48
      i32.add
      i32.store8
      local.get 0
      i32.const 10
      i32.div_u
      local.tee 0
      br_if 0 (;@1;)
    end
    local.get 2
    call 3
    global.get 0
    i32.const 16
    i32.add
    global.set 0)
  (memory (;0;) 1)
  (global (;0;) (mut i32) (i32.const 1024))
  (export "memory" (memory 0))
  (export "_start" (func 1))
  (export "print-i32" (func 4))
  (data (;0;) (i32.const 1024) "\04\00\00\00fib(")
  (data (;1;) (i32.const 1032) "\04\00\00\00) = ")
  (data (;2;) (i32.const 1040) "\01\00\00\00\0a"))
