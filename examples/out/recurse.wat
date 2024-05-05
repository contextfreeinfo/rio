(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32 i32)))
  (type (;3;) (func (param i32) (result i32)))
  (type (;4;) (func (result i32)))
  (type (;5;) (func))
  (type (;6;) (func (result i32 i32)))
  (type (;7;) (func (param i32 i32)))
  (type (;8;) (func (param i32 i32) (result i32)))
  (type (;9;) (func (param i32 i32 i32 i32)))
  (type (;10;) (func (param i32)))
  (type (;11;) (func (param i32) (result i32)))
  (type (;12;) (func (param i32) (result i32 i32)))
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
  (func (;5;) (type 5)
    (local i32)
    i32.const 3
    local.set 0
    local.get 0
    call 6
    local.get 0
    call 9
    call 1
    local.get 0
    i32.const 1
    i32.add
    call 9
    call 1
  )
  (func (;6;) (type 10) (param i32)
    i32.const 12
    i32.const 4098
    call 1
    local.get 0
    i32.const 1
    i32.gt_s
    if ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call 6
    else
      i32.const 4
      i32.const 4111
      call 1
    end
  )
  (func (;7;) (type 11) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.gt_s
    if (type 4) (result i32) ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call 8
    else
      i32.const 1
    end
  )
  (func (;8;) (type 11) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.gt_s
    if (type 4) (result i32) ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call 7
    else
      i32.const 0
    end
  )
  (func (;9;) (type 12) (param i32) (result i32 i32)
    local.get 0
    call 7
    if (type 6) (result i32 i32) ;; label = @1
      i32.const 4
      i32.const 4116
    else
      i32.const 3
      i32.const 4121
    end
  )
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
  (export "_start" (func 5))
  (data (;0;) (i32.const 4096) "\0a\00")
  (data (;1;) (i32.const 4098) "counting ...\00")
  (data (;2;) (i32.const 4111) "done\00")
  (data (;3;) (i32.const 4116) "even\00")
  (data (;4;) (i32.const 4121) "odd\00")
)