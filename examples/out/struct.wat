(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32 i32)))
  (type (;3;) (func (param i32) (result i32)))
  (type (;4;) (func (result i32 i32)))
  (type (;5;) (func (result i32)))
  (type (;6;) (func))
  (type (;7;) (func (param i32 i32 i32)))
  (type (;8;) (func (param i32 i32)))
  (type (;9;) (func (param i32 i32 i32 i32)))
  (type (;10;) (func (param i32 i32) (result i32)))
  (type (;11;) (func (param i32)))
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
  (func (;5;) (type 6)
    (local i32 i32 i32)
    i32.const 5
    i32.const 4098
    local.set 1
    local.set 0
    i32.const 12
    call 4
    i32.const 12
    call 4
    local.get 0
    local.get 1
    i32.const 40
    i32.const 12
    call 3
    local.set 2
    local.get 2
    call 7
    i32.const 12
    call 4
    local.get 2
    i32.const 3
    i32.const 4104
    call 7
    i32.const 12
    call 3
    i32.const 12
    call 4
    i32.const 4
    i32.const 4108
    i32.const 10
    call 6
    call 7
    i32.const 12
    call 3
    i32.const 12
    call 4
    i32.const 9
    i32.const 6
    i32.const 4113
    call 7
    i32.const 12
    call 3
    i32.const 12
    call 3
  )
  (func (;6;) (type 7) (param i32 i32 i32)
    i32.const 12
    call 4
    local.get 0
    local.get 1
    local.get 2
    i32.const 12
    call 3
  )
  (func (;7;) (type 11) (param i32)
    i32.const 7
    i32.const 4120
    call 1
    local.get 0
    call 1
    local.get 0
    i32.const 18
    i32.ge_s
    if (type 4) (result i32 i32) ;; label = @1
      i32.const 5
      i32.const 4128
    else
      i32.const 5
      i32.const 4134
    end
    call 1
    i32.const 0
    i32.const 4140
    call 1
  )
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
  (export "_start" (func 5))
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