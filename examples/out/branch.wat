(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32 i32)))
  (type (;3;) (func (param i32) (result i32)))
  (type (;4;) (func (param i32 i32 i32 i32)))
  (type (;5;) (func (param i32 i32) (result i32)))
  (type (;6;) (func (param i32 i32)))
  (type (;7;) (func))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func (;1;) (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call 2
    i32.const 1
    i32.const 4096
    call 2
  )
  (func (;2;) (type 2) (param i32 i32)
    (local i32 i32)
    i32.const 8
    call 4
    local.set 2
    i32.const 4
    call 4
    local.set 3
    local.get 2
    local.get 1
    i32.store align=1
    local.get 2
    i32.const 4
    i32.add
    local.get 0
    i32.store
    i32.const 1
    local.get 2
    i32.const 1
    local.get 3
    call 0
    drop
    i32.const 12
    call 3
  )
  (func (;3;) (type 1) (param i32)
    global.get 0
    local.get 0
    i32.add
    global.set 0
  )
  (func (;4;) (type 3) (param i32) (result i32)
    (local i32)
    global.get 0
    local.get 0
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
  )
  (func (;5;) (type 7)
    (local i32 i32 i32)
    local.set 0
    local.get 0
    if (result i32) ;; label = @1
      i32.const 8
      i32.const 4098
    else
      local.get 0
      if (result i32) ;; label = @2
        i32.const 8
        i32.const 4107
      else
        i32.const 4
        i32.const 4116
      end
    end
    local.set 2
    local.set 1
    local.get 1
    local.get 2
    call 1
  )
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
  (export "_start" (func 5))
  (data (;0;) (i32.const 4096) "\0a\00")
  (data (;1;) (i32.const 4098) "negative\00")
  (data (;2;) (i32.const 4107) "positive\00")
  (data (;3;) (i32.const 4116) "zero\00")
)