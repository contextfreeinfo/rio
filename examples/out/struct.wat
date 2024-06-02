(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32) (result i32 i32)))
  (type (;2;) (func (param i32)))
  (type (;3;) (func (param i32 i32)))
  (type (;4;) (func (param i32) (result i32)))
  (type (;5;) (func (param i32 i32 i32) (result i32 i32 i32)))
  (type (;6;) (func (param i32 i32) (result i32 i32)))
  (type (;7;) (func (result i32 i32)))
  (type (;8;) (func (result i32)))
  (type (;9;) (func))
  (type (;10;) (func (param i32)))
  (type (;11;) (func (param i32 i32 i32 i32)))
  (type (;12;) (func (param i32 i32)))
  (type (;13;) (func (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func $-i32.dup (;1;) (type 1) (param i32) (result i32 i32)
    local.get 0
    local.get 0
  )
  (func $core::print (;2;) (type 3) (param i32 i32)
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
  (func $-rot3 (;6;) (type 5) (param i32 i32 i32) (result i32 i32 i32)
    local.get 2
    local.get 0
    local.get 1
  )
  (func $-swap (;7;) (type 6) (param i32 i32) (result i32 i32)
    local.get 1
    local.get 0
  )
  (func $main (;8;) (type 9)
    (local i32 i32 i32)
    i32.const 5
    i32.const 4098
    local.set 1
    local.set 0
    i32.const 12
    call $-push
    call $-i32.dup
    call $-i32.dup
    local.get 0
    local.get 1
    call $-rot3
    i32.store
    i32.store offset=4
    call $-i32.dup
    i32.const 8
    i32.add
    i32.const 40
    i32.store
    local.set 2
    local.get 2
    call $describe
    i32.const 12
    call $-push
    call $-i32.dup
    i32.const 8
    i32.add
    local.get 2
    i32.const 8
    i32.add
    i32.load
    i32.store
    call $-i32.dup
    call $-i32.dup
    i32.const 3
    i32.const 4104
    call $-rot3
    i32.store
    i32.store offset=4
    call $describe
    i32.const 12
    call $-pop
    i32.const 12
    call $-push
    call $-i32.dup
    i32.const 4
    i32.const 4108
    i32.const 10
    call $build
    call $describe
    i32.const 12
    call $-pop
    i32.const 12
    call $-push
    call $-i32.dup
    i32.const 8
    i32.add
    i32.const 9
    i32.store
    call $-i32.dup
    call $-i32.dup
    i32.const 6
    i32.const 4113
    call $-rot3
    i32.store
    i32.store offset=4
    call $describe
    i32.const 12
    call $-pop
    i32.const 12
    call $-pop
  )
  (func $build (;9;) (type 11) (param i32 i32 i32 i32)
    i32.const 12
    call $-push
    call $-i32.dup
    call $-i32.dup
    local.get 1
    local.get 2
    call $-rot3
    i32.store
    i32.store offset=4
    call $-i32.dup
    i32.const 8
    i32.add
    local.get 3
    i32.store
    local.get 0
    call $-swap
    i32.const 12
    memory.copy
    i32.const 12
    call $-pop
  )
  (func $describe (;10;) (type 10) (param i32)
    i32.const 7
    i32.const 4120
    call $core::print
    local.get 0
    call $-i32.dup
    i32.load
    call $-swap
    i32.load offset=4
    call $core::print
    local.get 0
    i32.const 8
    i32.add
    i32.load
    i32.const 18
    i32.ge_s
    if (type 7) (result i32 i32) ;; label = @1
      i32.const 5
      i32.const 4128
    else
      i32.const 5
      i32.const 4134
    end
    call $core::print
    i32.const 0
    i32.const 4140
    call $core::print
  )
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
  (export "_start" (func $main))
  (data (;0;) (i32.const 4096) "\0a\00")
  (data (;1;) (i32.const 4098) "Alice\00")
  (data (;2;) (i32.const 4104) "Bob\00")
  (data (;3;) (i32.const 4108) "Carl\00")
  (data (;4;) (i32.const 4113) "Denise\00")
  (data (;5;) (i32.const 4120) "Person:\00")
  (data (;6;) (i32.const 4128) "adult\00")
  (data (;7;) (i32.const 4134) "minor\00")
  (data (;8;) (i32.const 4140) "\00")
)