(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func))
  (type (;4;) (func (param i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func (;1;) (type 1) (param i32)
    i32.const 8
    call 2
    local.set 1
    i32.const 4
    call 2
    local.set 2
    local.get 1
    local.get 0
    i32.const 4
    i32.add
    i32.store align=1
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
    i32.const 12
    call 1
  )
  (func (;2;) (type 1) (param i32)
    global.get 0
    local.get 0
    i32.add
    global.set 0
  )
  (func (;3;) (type 2) (param i32) (result i32)
    global.get 0
    local.get 0
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
  )
  (func (;4;) (type 3))
  (func (;5;) (type 3))
  (func (;6;) (type 3))
  (func (;7;) (type 4) (param i32))
  (func (;8;) (type 4) (param i32))
  (func (;9;) (type 4) (param i32))
  (func (;10;) (type 4) (param i32))
  (func (;11;) (type 4) (param i32))
  (func (;12;) (type 4) (param i32))
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
)