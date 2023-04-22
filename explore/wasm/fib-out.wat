(module
  (type (;0;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32) (result i32)
    (local i32 i32)
    i32.const 0
    local.set 1
    block  ;; label = @1
      loop  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.const 1
          i32.ge_s
          br_if 0 (;@3;)
          i32.const 0
          local.set 2
          br 2 (;@1;)
        end
        i32.const 1
        local.set 2
        local.get 0
        i32.const 1
        i32.eq
        br_if 1 (;@1;)
        local.get 0
        i32.const -2
        i32.add
        call 0
        local.get 1
        i32.add
        local.set 1
        local.get 0
        i32.const -1
        i32.add
        local.set 0
        br 0 (;@2;)
      end
    end
    local.get 2
    local.get 1
    i32.add)
  (memory (;0;) 16)
  (global (;0;) (mut i32) (i32.const 1048576))
  (export "memory" (memory 0))
  (export "fib" (func 0)))
