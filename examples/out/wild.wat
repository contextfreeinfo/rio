(module
  (type (;0;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;1;) (func))
  (type (;2;) (func (param i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func (;0;) (type 0)))
  (func (;1;) (type 1))
  (func (;2;) (type 1))
  (func (;3;) (type 1))
  (func (;4;) (type 2) (param i32))
  (func (;5;) (type 2) (param i32)
    i32.const 4
  )
  (func (;6;) (type 2) (param i32)
    i32.const 4
  )
  (func (;7;) (type 2) (param i32)
    i32.const 4
  )
  (func (;8;) (type 2) (param i32)
    i32.const 4
  )
  (func (;9;) (type 2) (param i32)
    i32.const 4
  )
  (memory (;0;) 1)
  (global (;0;) (mut i32) i32.const 4096)
  (export "memory" (memory 0))
)