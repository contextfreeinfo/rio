(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32) (result i32 i32)))
  (type (;2;) (func (param i32)))
  (type (;3;) (func (param i32 i32)))
  (type (;4;) (func (param i32) (result i32)))
  (type (;5;) (func (param i32 i32 i32) (result i32 i32 i32)))
  (type (;6;) (func (param i32 i32) (result i32 i32)))
  (type (;7;) (func (result i32)))
  (type (;8;) (func))
  (type (;9;) (func (result i32 i32)))
  (type (;10;) (func (param i32 i32)))
  (type (;11;) (func (param i32 i32) (result i32)))
  (type (;12;) (func (param i32 i32 i32 i32)))
  (type (;13;) (func (param i32)))
  (type (;14;) (func (param i32) (result i32)))
  (type (;15;) (func (param i32) (result i32 i32)))
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
  (func $main (;8;) (type 8)
    (local i32)
    i32.const 3
    local.set 0
    local.get 0
    call $countDown
    local.get 0
    call $oddness
    call $core::print
    local.get 0
    i32.const 1
    i32.add
    call $oddness
    call $core::print
  )
  (func $countDown (;9;) (type 13) (param i32)
    i32.const 12
    i32.const 4098
    call $core::print
    local.get 0
    i32.const 1
    i32.gt_s
    if ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call $countDown
    else
      i32.const 4
      i32.const 4111
      call $core::print
    end
  )
  (func $isEven (;10;) (type 14) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.gt_s
    if (type 7) (result i32) ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call $isOdd
    else
      i32.const 1
    end
  )
  (func $isOdd (;11;) (type 14) (param i32) (result i32)
    local.get 0
    i32.const 0
    i32.gt_s
    if (type 7) (result i32) ;; label = @1
      local.get 0
      i32.const 1
      i32.sub
      call $isEven
    else
      i32.const 0
    end
  )
  (func $oddness (;12;) (type 15) (param i32) (result i32 i32)
    local.get 0
    call $isEven
    if (type 9) (result i32 i32) ;; label = @1
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
  (export "_start" (func $main))
  (data (;0;) (i32.const 4096) "\0a\00")
  (data (;1;) (i32.const 4098) "counting ...\00")
  (data (;2;) (i32.const 4111) "done\00")
  (data (;3;) (i32.const 4116) "even\00")
  (data (;4;) (i32.const 4121) "odd\00")
)
