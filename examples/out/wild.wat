(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32) (result i32 i32)))
  (type (;2;) (func (param i32)))
  (type (;3;) (func (param i32 i32)))
  (type (;4;) (func (param i32) (result i32)))
  (type (;5;) (func (result i32)))
  (type (;6;) (func (param i32)))
  (type (;7;) (func))
  (type (;8;) (func (result i32 i32)))
  (type (;9;) (func (param i32 i32)))
  (type (;10;) (func (param i32 i32 i32 i32)))
  (type (;11;) (func (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func $-load2 (;1;) (type 1) (param i32) (result i32 i32)
    local.get 0
    i32.load align=1
    local.get 0
    i32.const 4
    i32.add
    i32.load align=1
  )
  (func $print (;2;) (type 3) (param i32 i32)
    local.get 0
    local.get 1
    call $-printInline
    i32.const 1
    i32.const 4096
    call $-printInline
  )
  (func $-printInline (;3;) (type 3) (param i32 i32)
    (local i32 i32)
    i32.const 8
    call $-push
    local.set 2
    i32.const 4
    call $-push
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
    call $-pop
  )
  (func $-pop (;4;) (type 2) (param i32)
    global.get 0
    local.get 0
    i32.add
    global.set 0
  )
  (func $-push (;5;) (type 4) (param i32) (result i32)
    (local i32)
    global.get 0
    local.get 0
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
  )
  (func (;6;) (type 6) (param i32))
  (func (;7;) (type 7))
  (func (;8;) (type 7))
  (func (;9;) (type 7))
  (func (;10;) (type 6) (param i32))
  (func (;11;) (type 6) (param i32))
  (func (;12;) (type 6) (param i32))
  (func (;13;) (type 6) (param i32))
  (func (;14;) (type 6) (param i32))
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
  (data (;0;) (i32.const 4096) "\0a\00")
  (data (;1;) (i32.const 4098) "no\00")
  (data (;2;) (i32.const 4101) "a single\00")
  (data (;3;) (i32.const 4110) "a couple of\00")
  (data (;4;) (i32.const 4122) "a few of\00")
  (data (;5;) (i32.const 4131) "negative\00")
  (data (;6;) (i32.const 4140) "several\00")
)